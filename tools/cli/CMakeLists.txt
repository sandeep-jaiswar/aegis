# Aegis CLI tool
# Minimal build configuration for the developer workflow tool

# CLI executable
add_executable(aegis main.cpp)

# No dependencies - this is a standalone tool
# It wraps CMake, shell, and benchmark executables

# Install to bin directory
install(TARGETS aegis DESTINATION bin)
