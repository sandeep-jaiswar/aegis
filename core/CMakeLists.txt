cmake_minimum_required(VERSION 3.20)

# Core library - deterministic engine
project(aegis_core VERSION 0.1.0 LANGUAGES CXX)

# Collect all source files
file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Exclude demo files from library
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*demo.*\\.cpp$")

file(GLOB_RECURSE CORE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# Create static library
add_library(aegis_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# Set C++23 standard
target_compile_features(aegis_core PUBLIC cxx_std_23)

# Include directory
target_include_directories(aegis_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(aegis_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
    )
elseif(MSVC)
    target_compile_options(aegis_core PRIVATE
        /W4
        /WX
        $<$<CONFIG:Release>:/O2>
    )
endif()

# Enable clang-tidy for static analysis
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" ON)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}")
        set_target_properties(aegis_core PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
        message(STATUS "clang-tidy enabled for aegis_core")
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

# Optional: Build demo executable
option(BUILD_MEMORY_DEMO "Build memory allocator demonstration" ON)
if(BUILD_MEMORY_DEMO)
    add_executable(memory_demo memory/demo_allocators.cpp)
    target_link_libraries(memory_demo PRIVATE aegis_core)
    target_compile_features(memory_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(memory_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(memory_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build state demo executable
option(BUILD_STATE_DEMO "Build immutable state demonstration" ON)
if(BUILD_STATE_DEMO)
    add_executable(state_demo state/demo_state.cpp)
    target_link_libraries(state_demo PRIVATE aegis_core)
    target_compile_features(state_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(state_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(state_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build event stream demo executable
option(BUILD_EVENT_STREAM_DEMO "Build deterministic event stream demonstration" ON)
if(BUILD_EVENT_STREAM_DEMO)
    add_executable(event_stream_demo state/demo_event_stream.cpp)
    target_link_libraries(event_stream_demo PRIVATE aegis_core)
    target_compile_features(event_stream_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(event_stream_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(event_stream_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build layout demo executable
option(BUILD_LAYOUT_DEMO "Build layout engine demonstration" ON)
if(BUILD_LAYOUT_DEMO)
    add_executable(layout_demo layout/demo_layout.cpp)
    target_link_libraries(layout_demo PRIVATE aegis_core)
    target_compile_features(layout_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(layout_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(layout_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build scene graph demo executable
option(BUILD_SCENE_GRAPH_DEMO "Build scene graph demonstration" ON)
if(BUILD_SCENE_GRAPH_DEMO)
    add_executable(scene_graph_demo frame/demo_scene_graph.cpp)
    target_link_libraries(scene_graph_demo PRIVATE aegis_core)
    target_compile_features(scene_graph_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(scene_graph_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(scene_graph_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build diff engine demo executable
option(BUILD_DIFF_ENGINE_DEMO "Build structural diff engine demonstration" ON)
if(BUILD_DIFF_ENGINE_DEMO)
    add_executable(diff_engine_demo frame/demo_diff_engine.cpp)
    target_link_libraries(diff_engine_demo PRIVATE aegis_core)
    target_compile_features(diff_engine_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(diff_engine_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(diff_engine_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build GPU command demo executable
option(BUILD_GPU_COMMAND_DEMO "Build GPU command backend demonstration" ON)
if(BUILD_GPU_COMMAND_DEMO)
    add_executable(gpu_command_demo frame/demo_gpu_command.cpp)
    target_link_libraries(gpu_command_demo PRIVATE aegis_core)
    target_compile_features(gpu_command_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(gpu_command_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(gpu_command_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build data grid demo executable
option(BUILD_DATA_GRID_DEMO "Build high-density data grid demonstration" ON)
if(BUILD_DATA_GRID_DEMO)
    add_executable(data_grid_demo frame/demo_data_grid.cpp)
    target_link_libraries(data_grid_demo PRIVATE aegis_core)
    target_compile_features(data_grid_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(data_grid_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(data_grid_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build benchmark demo executable
option(BUILD_BENCHMARK_DEMO "Build deterministic benchmark harness demonstration" ON)
if(BUILD_BENCHMARK_DEMO)
    add_executable(benchmark_demo benchmark/demo_benchmark.cpp)
    target_link_libraries(benchmark_demo PRIVATE aegis_core)
    target_compile_features(benchmark_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(benchmark_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(benchmark_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build browser comparison demo executable
option(BUILD_BROWSER_COMPARISON_DEMO "Build browser comparison demonstration" ON)
if(BUILD_BROWSER_COMPARISON_DEMO)
    add_executable(browser_comparison_demo benchmark/demo_browser_comparison.cpp)
    target_link_libraries(browser_comparison_demo PRIVATE aegis_core)
    target_compile_features(browser_comparison_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(browser_comparison_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(browser_comparison_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()

# Optional: Build module format demo executable
option(BUILD_MODULE_DEMO "Build .aegis module format demonstration" ON)
if(BUILD_MODULE_DEMO)
    add_subdirectory(module)
endif()

# Ensure no OS/runtime headers are included in core
# This is enforced by the compile-time checks and architecture contract
