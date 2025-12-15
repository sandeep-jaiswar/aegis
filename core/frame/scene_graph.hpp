#pragma once

#include "core/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::frame {

// Stable node identifier that persists across frames
// Node IDs are never reused within the same session
using node_id = uint64_t;

// Special node ID values
constexpr node_id invalid_node_id = 0;
constexpr node_id root_node_id = 1;

// Node type classification for scene graph nodes
enum class node_type : uint8_t {
    container = 0, // Layout container (can have children)
    rectangle = 1, // Rectangle primitive
    text = 2,      // Text primitive
    image = 3,     // Image primitive
    custom = 255   // Application-defined node type
};

// Immutable scene node properties
// Properties are frozen once set and cannot be modified during frame
struct node_properties {
    float x{0.0F};              // X position
    float y{0.0F};              // Y position
    float width{0.0F};          // Width
    float height{0.0F};         // Height
    uint32_t color{0xFF000000}; // RGBA color (default: opaque black)

    // Additional properties can be extended here
    // All properties must be POD types for deterministic behavior
};

// Scene node - immutable representation of a renderable element
// Nodes are frozen during frame execution (build_scene to end_frame)
struct scene_node {
    node_id id{invalid_node_id};          // Stable node identifier
    node_id parent_id{invalid_node_id};   // Parent node ID (invalid_node_id = no parent)
    node_type type{node_type::container}; // Node type
    node_properties props{};              // Immutable properties

    // Child range in scene_graph's children array
    // This allows O(1) access to children without pointers
    uint32_t first_child_index{0};
    uint32_t child_count{0};

    // Flag to track if node is frozen (immutable during frame)
    bool frozen{false};

    // Check if this is a valid node
    [[nodiscard]] bool is_valid() const noexcept {
        return id != invalid_node_id;
    }

    // Check if this is the root node
    [[nodiscard]] bool is_root() const noexcept {
        return id == root_node_id;
    }

    // Check if this node has a parent
    [[nodiscard]] bool has_parent() const noexcept {
        return parent_id != invalid_node_id;
    }

    // Check if this node has children
    [[nodiscard]] bool has_children() const noexcept {
        return child_count > 0;
    }
};

// Scene graph capacity configuration
struct scene_graph_config {
    uint32_t max_nodes{1024};    // Maximum number of nodes
    uint32_t max_children{2048}; // Maximum total child relationships
};

// Scene graph construction result
enum class scene_result : uint8_t {
    success = 0,
    out_of_memory = 1,
    invalid_node_id = 2,
    node_limit_exceeded = 3,
    child_limit_exceeded = 4,
    graph_frozen = 5, // Cannot modify frozen graph
    parent_not_found = 6
};

// Retained-mode scene graph
// Provides stable, immutable scene representation per frame
//
// Acceptance criteria:
// 1. Scene graph is immutable during frame
//    - Graph is frozen during build_scene phase
//    - No modifications allowed while frozen
// 2. Node IDs are stable across frames
//    - IDs are monotonically increasing
//    - IDs are never reused in same session
// 3. No self-mutating nodes
//    - All node data is const during frame execution
//    - Properties cannot be modified while frozen
class scene_graph {
  public:
    // Create scene graph with configuration and allocator
    explicit scene_graph(const scene_graph_config& config, memory::allocator* alloc) noexcept
        : cfg(config), allocator(alloc), next_id(root_node_id + 1) {
        if (allocator == nullptr) {
            return;
        }

        // Allocate node storage
        const size_t nodes_size = sizeof(scene_node) * cfg.max_nodes;
        void* nodes_mem = allocator->allocate(nodes_size, alignof(scene_node));
        if (nodes_mem != nullptr) {
            nodes = static_cast<scene_node*>(nodes_mem);
            // Initialize all nodes to invalid
            for (uint32_t i = 0; i < cfg.max_nodes; ++i) {
                nodes[i] = scene_node{};
            }
        }

        // Allocate child ID storage
        const size_t children_size = sizeof(node_id) * cfg.max_children;
        void* children_mem = allocator->allocate(children_size, alignof(node_id));
        if (children_mem != nullptr) {
            children = static_cast<node_id*>(children_mem);
            // Initialize all child IDs to invalid
            for (uint32_t i = 0; i < cfg.max_children; ++i) {
                children[i] = invalid_node_id;
            }
        }

        // Create root node
        if (nodes != nullptr) {
            nodes[0] = scene_node{.id = root_node_id,
                                  .parent_id = invalid_node_id,
                                  .type = node_type::container,
                                  .props = {},
                                  .first_child_index = 0,
                                  .child_count = 0,
                                  .frozen = false};
            node_count = 1;
        }
    }

    ~scene_graph() noexcept {
        if (allocator != nullptr) {
            if (nodes != nullptr) {
                allocator->deallocate(nodes, sizeof(scene_node) * cfg.max_nodes);
            }
            if (children != nullptr) {
                allocator->deallocate(children, sizeof(node_id) * cfg.max_children);
            }
        }
    }

    // Disable copy and move - scene graph has unique ownership
    scene_graph(const scene_graph&) = delete;
    scene_graph& operator=(const scene_graph&) = delete;
    scene_graph(scene_graph&&) = delete;
    scene_graph& operator=(scene_graph&&) = delete;

    // Create a new node with stable ID
    // Returns node_id on success, invalid_node_id on failure
    [[nodiscard]] node_id create_node(node_type type, const node_properties& props) noexcept {
        if (is_frozen) {
            return invalid_node_id; // Cannot modify frozen graph
        }

        if (node_count >= cfg.max_nodes) {
            return invalid_node_id; // Node limit exceeded
        }

        if (nodes == nullptr) {
            return invalid_node_id; // Not initialized
        }

        // Allocate stable node ID (monotonically increasing)
        const node_id new_id = next_id++;

        // Find slot for new node
        nodes[node_count] = scene_node{.id = new_id,
                                       .parent_id = invalid_node_id,
                                       .type = type,
                                       .props = props,
                                       .first_child_index = 0,
                                       .child_count = 0,
                                       .frozen = false};

        node_count++;
        return new_id;
    }

    // Add child node to parent
    [[nodiscard]] scene_result add_child(node_id parent, node_id child) noexcept {
        if (is_frozen) {
            return scene_result::graph_frozen;
        }

        if (nodes == nullptr || children == nullptr) {
            return scene_result::out_of_memory;
        }

        // Find parent node
        scene_node* parent_node = find_node(parent);
        if (parent_node == nullptr) {
            return scene_result::parent_not_found;
        }

        // Find child node
        scene_node* child_node = find_node(child);
        if (child_node == nullptr) {
            return scene_result::invalid_node_id;
        }

        // Check child capacity
        if (child_index_count >= cfg.max_children) {
            return scene_result::child_limit_exceeded;
        }

        // If parent has no children yet, allocate child range
        if (parent_node->child_count == 0) {
            parent_node->first_child_index = child_index_count;
        }

        // Add child to parent's child list
        children[child_index_count] = child;
        child_index_count++;
        parent_node->child_count++;

        // Update child's parent reference
        child_node->parent_id = parent;

        return scene_result::success;
    }

    // Freeze scene graph (make immutable during frame)
    void freeze() noexcept {
        is_frozen = true;
        // Freeze all nodes
        for (uint32_t i = 0; i < node_count; ++i) {
            nodes[i].frozen = true;
        }
    }

    // Unfreeze scene graph (allow modifications for next frame)
    void unfreeze() noexcept {
        is_frozen = false;
        // Unfreeze all nodes
        for (uint32_t i = 0; i < node_count; ++i) {
            nodes[i].frozen = false;
        }
    }

    // Clear scene graph (prepare for rebuild)
    void clear() noexcept {
        if (is_frozen) {
            return; // Cannot clear frozen graph
        }

        // Reset to just root node
        node_count = 1;
        child_index_count = 0;

        // Clear root node's children
        if (nodes != nullptr) {
            nodes[0].first_child_index = 0;
            nodes[0].child_count = 0;
        }

        // Note: We do NOT reset next_id to maintain stable IDs across frames
    }

    // Get node by ID (read-only access)
    [[nodiscard]] const scene_node* get_node(node_id id) const noexcept {
        return find_node(id);
    }

    // Get root node
    [[nodiscard]] const scene_node* get_root() const noexcept {
        return (nodes != nullptr && node_count > 0) ? &nodes[0] : nullptr;
    }

    // Get child IDs for a node
    [[nodiscard]] const node_id* get_children(node_id id, uint32_t& out_count) const noexcept {
        const scene_node* node = find_node(id);
        if (node == nullptr || children == nullptr) {
            out_count = 0;
            return nullptr;
        }

        out_count = node->child_count;
        return &children[node->first_child_index];
    }

    // Query state
    [[nodiscard]] bool frozen() const noexcept {
        return is_frozen;
    }

    [[nodiscard]] uint32_t count() const noexcept {
        return node_count;
    }

    [[nodiscard]] uint32_t capacity() const noexcept {
        return cfg.max_nodes;
    }

    [[nodiscard]] bool is_valid() const noexcept {
        return nodes != nullptr && children != nullptr;
    }

  private:
    // Find node by ID (internal helper)
    [[nodiscard]] scene_node* find_node(node_id id) noexcept {
        if (nodes == nullptr) {
            return nullptr;
        }

        // Linear search - could be optimized with hash map if needed
        for (uint32_t i = 0; i < node_count; ++i) {
            if (nodes[i].id == id) {
                return &nodes[i];
            }
        }

        return nullptr;
    }

    [[nodiscard]] const scene_node* find_node(node_id id) const noexcept {
        if (nodes == nullptr) {
            return nullptr;
        }

        for (uint32_t i = 0; i < node_count; ++i) {
            if (nodes[i].id == id) {
                return &nodes[i];
            }
        }

        return nullptr;
    }

    scene_graph_config cfg;
    memory::allocator* allocator;

    // Node storage
    scene_node* nodes{nullptr};
    uint32_t node_count{0};
    node_id next_id{root_node_id + 1}; // Monotonically increasing for stable IDs

    // Child relationship storage
    node_id* children{nullptr};
    uint32_t child_index_count{0};

    // Immutability flag
    bool is_frozen{false};
};

} // namespace aegis::core::frame
