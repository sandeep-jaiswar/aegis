# Module format library and demo

# Add demo executable
add_executable(demo_module demo_module.cpp)

# Link to core if needed (for now it's header-only)
# target_link_libraries(demo_module PRIVATE aegis_core)

# Set compile options
target_compile_options(demo_module PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Include directories
target_include_directories(demo_module PRIVATE ${CMAKE_SOURCE_DIR})
