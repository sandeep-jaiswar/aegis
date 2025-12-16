# Runtime Conformance Test Suite (REF-002)
# Implementation-agnostic tests for runtime conformance

cmake_minimum_required(VERSION 3.20)

# Conformance test suite executable
add_executable(conformance_tests
    conformance_tests.cpp
    main.cpp
)

target_link_libraries(conformance_tests
    PRIVATE
        aegis_core
)

# Include directories - point to project root so we can use "core/..." paths
target_include_directories(conformance_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

# Ensure tests can find core headers
target_compile_features(conformance_tests PRIVATE cxx_std_23)
