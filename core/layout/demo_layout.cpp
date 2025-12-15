// Demonstration of the one-pass layout engine
// This file is NOT part of the library, only for demonstration

#include "core/layout/layout_engine.hpp"

#include <cstdio>

namespace {

using namespace aegis::core::layout;

void print_box(const layout_box& box) {
    printf("Box ID %u: pos=(%.1f, %.1f) size=(%.1f x %.1f)\n", box.id,
           box.computed_rect.pos.x.value, box.computed_rect.pos.y.value,
           box.computed_rect.sz.width.value, box.computed_rect.sz.height.value);
}

void demo_simple_layout() {
    printf("\n=== Demo: Simple Vertical Layout ===\n");

    // Create a simple layout:
    // Root box (fills available space)
    //   - Child 1 (fixed 100x50)
    //   - Child 2 (fixed 200x75)

    layout_box boxes[3];

    // Root box - fills available space
    boxes[0].id = 1;
    boxes[0].constraints.width_constraint = size_constraint::fill;
    boxes[0].constraints.height_constraint = size_constraint::fill;
    boxes[0].layout_direction = direction::vertical;
    boxes[0].first_child_id = 2;
    boxes[0].pad = padding(dimension(10.0F)); // 10px padding

    // Child 1 - fixed size
    boxes[1].id = 2;
    boxes[1].parent_id = 1;
    boxes[1].content_size = size{dimension(100.0F), dimension(50.0F)};
    boxes[1].constraints.width_constraint = size_constraint::fixed;
    boxes[1].constraints.height_constraint = size_constraint::fixed;
    boxes[1].next_sibling_id = 3;

    // Child 2 - fixed size
    boxes[2].id = 3;
    boxes[2].parent_id = 1;
    boxes[2].content_size = size{dimension(200.0F), dimension(75.0F)};
    boxes[2].constraints.width_constraint = size_constraint::fixed;
    boxes[2].constraints.height_constraint = size_constraint::fixed;

    // Compute layout
    layout_engine engine;
    size available{dimension(800.0F), dimension(600.0F)};
    layout_result result = engine.compute_layout(boxes, 3, 1, available);

    if (result.success) {
        printf("Layout computed successfully!\n");
        printf("Processed %zu boxes\n", result.box_count);
        for (size_t i = 0; i < result.box_count; ++i) {
            print_box(boxes[i]);
        }
    } else {
        printf("Layout failed!\n");
    }
}

void demo_content_sized() {
    printf("\n=== Demo: Content-Sized Layout ===\n");

    // Create layout where parent sizes to fit children
    layout_box boxes[3];

    // Root box - sized based on content
    boxes[0].id = 1;
    boxes[0].constraints.width_constraint = size_constraint::content;
    boxes[0].constraints.height_constraint = size_constraint::content;
    boxes[0].layout_direction = direction::horizontal;
    boxes[0].first_child_id = 2;
    boxes[0].pad = padding(dimension(5.0F));

    // Child 1
    boxes[1].id = 2;
    boxes[1].parent_id = 1;
    boxes[1].content_size = size{dimension(80.0F), dimension(40.0F)};
    boxes[1].constraints.width_constraint = size_constraint::fixed;
    boxes[1].constraints.height_constraint = size_constraint::fixed;
    boxes[1].next_sibling_id = 3;

    // Child 2
    boxes[2].id = 3;
    boxes[2].parent_id = 1;
    boxes[2].content_size = size{dimension(120.0F), dimension(60.0F)};
    boxes[2].constraints.width_constraint = size_constraint::fixed;
    boxes[2].constraints.height_constraint = size_constraint::fixed;

    // Compute layout
    layout_engine engine;
    size available{dimension(1000.0F), dimension(1000.0F)};
    layout_result result = engine.compute_layout(boxes, 3, 1, available);

    if (result.success) {
        printf("Layout computed successfully!\n");
        printf("Root box sized to content: %.1f x %.1f\n", boxes[0].computed_rect.sz.width.value,
               boxes[0].computed_rect.sz.height.value);
        printf("(Expected: 210 x 70 = 80+120+10 padding, max(40,60)+10 padding)\n");
        for (size_t i = 0; i < result.box_count; ++i) {
            print_box(boxes[i]);
        }
    } else {
        printf("Layout failed!\n");
    }
}

void demo_alignment() {
    printf("\n=== Demo: Center Alignment ===\n");

    // Create layout with centered child
    layout_box boxes[2];

    // Root box
    boxes[0].id = 1;
    boxes[0].constraints.width_constraint = size_constraint::fill;
    boxes[0].constraints.height_constraint = size_constraint::fill;
    boxes[0].horizontal_align = alignment::center;
    boxes[0].vertical_align = alignment::center;
    boxes[0].first_child_id = 2;

    // Child box (should be centered)
    boxes[1].id = 2;
    boxes[1].parent_id = 1;
    boxes[1].content_size = size{dimension(100.0F), dimension(100.0F)};
    boxes[1].constraints.width_constraint = size_constraint::fixed;
    boxes[1].constraints.height_constraint = size_constraint::fixed;

    // Compute layout
    layout_engine engine;
    size available{dimension(400.0F), dimension(300.0F)};
    layout_result result = engine.compute_layout(boxes, 2, 1, available);

    if (result.success) {
        printf("Layout computed successfully!\n");
        printf("Child should be centered at (150, 100):\n");
        for (size_t i = 0; i < result.box_count; ++i) {
            print_box(boxes[i]);
        }
    } else {
        printf("Layout failed!\n");
    }
}

void demo_determinism() {
    printf("\n=== Demo: Determinism Test ===\n");

    // Create identical layouts twice and verify results are identical
    layout_box boxes1[2];
    layout_box boxes2[2];

    // Setup both identically
    for (int i = 0; i < 2; ++i) {
        layout_box* boxes = (i == 0) ? boxes1 : boxes2;

        boxes[0].id = 1;
        boxes[0].constraints.width_constraint = size_constraint::fill;
        boxes[0].constraints.height_constraint = size_constraint::content;
        boxes[0].first_child_id = 2;

        boxes[1].id = 2;
        boxes[1].parent_id = 1;
        boxes[1].content_size = size{dimension(100.0F), dimension(50.0F)};
        boxes[1].constraints.width_constraint = size_constraint::fixed;
        boxes[1].constraints.height_constraint = size_constraint::fixed;
    }

    // Compute layout for both
    layout_engine engine1;
    layout_engine engine2;
    size available{dimension(500.0F), dimension(400.0F)};

    layout_result result1 = engine1.compute_layout(boxes1, 2, 1, available);
    layout_result result2 = engine2.compute_layout(boxes2, 2, 1, available);

    if (result1.success && result2.success) {
        bool identical = true;
        for (size_t i = 0; i < 2; ++i) {
            if (boxes1[i].computed_rect.pos.x.value != boxes2[i].computed_rect.pos.x.value ||
                boxes1[i].computed_rect.pos.y.value != boxes2[i].computed_rect.pos.y.value ||
                boxes1[i].computed_rect.sz.width.value != boxes2[i].computed_rect.sz.width.value ||
                boxes1[i].computed_rect.sz.height.value !=
                    boxes2[i].computed_rect.sz.height.value) {
                identical = false;
                break;
            }
        }

        if (identical) {
            printf("✓ Determinism verified: Same inputs produced identical outputs\n");
        } else {
            printf("✗ Determinism FAILED: Outputs differ!\n");
        }
    } else {
        printf("Layout computation failed!\n");
    }
}

} // anonymous namespace

int main() {
    printf("One-Pass Layout Engine Demonstration\n");
    printf("=====================================\n");

    demo_simple_layout();
    demo_content_sized();
    demo_alignment();
    demo_determinism();

    printf("\n=== All demos completed ===\n");
    return 0;
}
