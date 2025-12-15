cmake_minimum_required(VERSION 3.20)

# Aegis Shell - Minimal viewer-runtime (NOT a browser)
project(aegis_shell VERSION 0.1.0 LANGUAGES CXX)

# Shell executable
add_executable(aegis_shell main.cpp)

# Set C++23 standard
target_compile_features(aegis_shell PRIVATE cxx_std_23)

# Link to runtime (which includes core)
target_link_libraries(aegis_shell PRIVATE aegis_runtime)

# Include directories
target_include_directories(aegis_shell
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(aegis_shell PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
    )
elseif(MSVC)
    target_compile_options(aegis_shell PRIVATE
        /W4
        /WX
        $<$<CONFIG:Release>:/O2>
    )
endif()

# Installation
install(TARGETS aegis_shell
    RUNTIME DESTINATION bin
)
