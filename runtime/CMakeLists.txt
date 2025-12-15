cmake_minimum_required(VERSION 3.20)

# Runtime library - platform abstraction and OS integration
project(aegis_runtime VERSION 0.1.0 LANGUAGES CXX)

# Collect all source files
file(GLOB_RECURSE RUNTIME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Exclude demo files from library
list(FILTER RUNTIME_SOURCES EXCLUDE REGEX ".*demo.*\\.cpp$")

file(GLOB_RECURSE RUNTIME_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# Create static library
add_library(aegis_runtime STATIC ${RUNTIME_SOURCES} ${RUNTIME_HEADERS})

# Set C++23 standard
target_compile_features(aegis_runtime PUBLIC cxx_std_23)

# Link to core library
target_link_libraries(aegis_runtime PUBLIC aegis_core)

# Include directories
target_include_directories(aegis_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(aegis_runtime PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
    )
elseif(MSVC)
    target_compile_options(aegis_runtime PRIVATE
        /W4
        /WX
        $<$<CONFIG:Release>:/O2>
    )
endif()

# Optional: Build platform demo executable
option(BUILD_PLATFORM_DEMO "Build platform adapter demonstration" ON)
if(BUILD_PLATFORM_DEMO)
    add_executable(platform_demo platform/demo_platform.cpp)
    target_link_libraries(platform_demo PRIVATE aegis_runtime)
    target_compile_features(platform_demo PRIVATE cxx_std_23)
    
    # Compiler-specific flags for demo
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(platform_demo PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(platform_demo PRIVATE
            /W4
            $<$<CONFIG:Release>:/O2>
        )
    endif()
endif()
