cmake_minimum_required(VERSION 3.20)

# Aegis - C++-native, GPU-first application runtime
project(aegis VERSION 0.1.0 LANGUAGES CXX)

# Set C++23 standard globally
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Global compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
    
    # Release-specific optimizations
    add_compile_options(
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
    )
elseif(MSVC)
    add_compile_options(
        /W4
        /WX
    )
    
    add_compile_options(
        $<$<CONFIG:Release>:/O2>
    )
endif()

# Build core library (standalone, no runtime/OS dependencies)
add_subdirectory(core)

# Runtime, app, tools, tests can be added later
# add_subdirectory(runtime)
# add_subdirectory(app)
# add_subdirectory(tools)
# add_subdirectory(tests)
