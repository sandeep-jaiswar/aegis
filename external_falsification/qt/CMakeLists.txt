cmake_minimum_required(VERSION 3.16)
project(aegis_qt_benchmark VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Enable automatic MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add executable
add_executable(qt_benchmark
    src/main.cpp
    src/benchmark_window.cpp
    src/benchmark_window.h
    src/workload.cpp
    src/workload.h
)

# Link Qt libraries
target_link_libraries(qt_benchmark
    Qt6::Core
    Qt6::Widgets
)

# Set output directory
set_target_properties(qt_benchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
