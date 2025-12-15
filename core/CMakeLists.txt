cmake_minimum_required(VERSION 3.20)

# Core library - deterministic engine
project(aegis_core VERSION 0.1.0 LANGUAGES CXX)

# Collect all source files
file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

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

# Ensure no OS/runtime headers are included in core
# This is enforced by the compile-time checks and architecture contract
