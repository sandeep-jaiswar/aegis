# Building Aegis

This document describes how to build the Aegis runtime from source.

## Prerequisites

- CMake 3.20 or higher
- C++23 compatible compiler:
  - GCC 13.0 or higher
  - Clang 18.0 or higher
- Git

## Quick Start

```bash
# Clone the repository
git clone https://github.com/sandeep-jaiswar/aegis.git
cd aegis

# Configure and build (Release mode with optimizations)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# The core library will be at: build/core/libaegis_core.a
```

## Build Configurations

### Release Build (Recommended for production)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Release builds include:
- `-O3` optimization level
- `-march=native` for architecture-specific optimizations
- `-Werror` to treat warnings as errors

### Debug Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Debug builds include debugging symbols and no optimizations.

## Compiler Selection

### Using GCC

```bash
cmake -B build -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Using Clang

```bash
cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Build Targets

Currently, the following targets are available:

- `aegis_core` - The core deterministic engine library (static library)

## Architecture Constraints

The `core/` library is built with strict architectural constraints:

- **No runtime dependencies**: The core library does not depend on any OS or runtime headers
- **No OS headers**: Platform-specific code belongs in `runtime/`, not `core/`
- **Standalone**: The core library can be built and tested independently
- **Deterministic**: All behavior must be deterministic and predictable

See [docs/CORE_FOLDER_CONTRACT.md](docs/CORE_FOLDER_CONTRACT.md) for detailed architectural guidelines.

## Continuous Integration

The project uses GitHub Actions for continuous integration. All builds:

- Must compile without warnings (enforced with `-Werror`)
- Are tested with both GCC and Clang
- Are built in both Release and Debug modes
- Must pass all tests (when tests are added)

See [.github/workflows/build.yml](.github/workflows/build.yml) for CI configuration.

## Troubleshooting

### CMake version too old

If you get an error about CMake version, update CMake:

```bash
# On Ubuntu/Debian
sudo apt update
sudo apt install cmake

# Or download from https://cmake.org/download/
```

### Compiler doesn't support C++23

Ensure you have a recent compiler version:

```bash
# Check GCC version
g++ --version

# Check Clang version
clang++ --version
```

Required minimum versions:
- GCC 13.0+
- Clang 18.0+

## Next Steps

- [Architecture Overview](docs/ARCHITECTURE.md)
- [Core Folder Contract](docs/CORE_FOLDER_CONTRACT.md)
- [Project Manifesto](MANIFESTO.md)
