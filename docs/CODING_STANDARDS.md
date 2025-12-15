# Coding Standards Enforcement

This document describes the automated tooling in place to enforce the coding standards defined in [COPILOT_INSTRUCTIONS.md](../.github/COPILOT_INSTRUCTIONS.md) and [ARCHITECTURE.md](ARCHITECTURE.md).

## Overview

Aegis enforces strict coding standards to maintain deterministic performance, correctness, and architectural discipline. The enforcement is automated through:

1. **Compiler Flags** - Build-time enforcement via GCC/Clang/MSVC
2. **Static Analysis** - clang-tidy for code quality
3. **Code Formatting** - clang-format for consistent style
4. **Banned API Checks** - Custom script to prevent prohibited patterns
5. **CI Integration** - Automated checks on every commit

## 1. Compiler Flags

### Banned Language Features (Build Failures)

The following features are **disabled globally** via compiler flags:

- **RTTI** (`-fno-rtti` / `/GR-`)
  - `dynamic_cast` and `typeid` will fail to compile
  - Rationale: RTTI adds overhead in hot paths

- **Exceptions** (`-fno-exceptions` / `/EHsc-`)
  - `throw` statements will fail to compile
  - Rationale: Exceptions are non-deterministic in hot paths
  - Alternative: Use `std::expected` or error codes

### Warning Policy

- All warnings are treated as errors (`-Werror` / `/WX`)
- Maximum warning levels enabled (`-Wall -Wextra -Wpedantic` / `/W4`)

## 2. Static Analysis (clang-tidy)

clang-tidy is automatically run during builds to catch:

- Bug-prone patterns
- Performance issues
- Concurrency problems
- Modern C++ violations
- Code guideline violations

Configuration: [.clang-tidy](../.clang-tidy)

### Running clang-tidy manually

```bash
# Enable/disable clang-tidy during build
cmake -B build -DENABLE_CLANG_TIDY=ON
cmake --build build

# Run on specific files
clang-tidy core/your_file.cpp
```

## 3. Code Formatting (clang-format)

All C++ code must follow the project's formatting rules.

Configuration: [.clang-format](../.clang-format)

### Running clang-format

```bash
# Format a file in-place
clang-format -i core/your_file.cpp

# Format all core files
find core -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check formatting (CI mode)
clang-format --dry-run --Werror core/your_file.cpp
```

### Style Highlights

- **Indentation**: 4 spaces (no tabs)
- **Line Length**: 100 characters
- **Braces**: Attached style (same line)
- **Naming**: `lower_case` for everything (no CamelCase)
- **Pointers**: Left-aligned (`int* ptr`, not `int *ptr`)

## 4. Banned API Checker

A custom script enforces architectural constraints by scanning source code for prohibited patterns.

Script: [.github/scripts/check_banned_apis.sh](../.github/scripts/check_banned_apis.sh)

### Banned APIs (Build Failures)

The following will **fail CI** if found in `core/`:

1. **`std::shared_ptr`**
   - Violation of explicit ownership principle
   - Use: `std::unique_ptr`, raw pointers with clear ownership, or arena allocators

2. **`throw` statements**
   - Already caught by `-fno-exceptions`, but double-checked
   - Use: `std::expected` or error codes

3. **RTTI** (`dynamic_cast`, `typeid`)
   - Already caught by `-fno-rtti`, but double-checked
   - Use: Static polymorphism or explicit type tracking

### Warnings (Non-Blocking)

The following generate warnings but don't fail builds:

1. **`new` / `delete`**
   - Prefer arena, frame, or pool allocators
   - Rationale: Deterministic memory management

2. **`std::function`**
   - Consider templates for zero-cost abstraction
   - Rationale: Type erasure has overhead

### Running the checker manually

```bash
.github/scripts/check_banned_apis.sh
```

## 5. CI Integration

GitHub Actions automatically runs all checks on every push and pull request.

Workflow: [.github/workflows/build.yml](../.github/workflows/build.yml)

### CI Jobs

1. **format-check**: Verifies all code is properly formatted
2. **banned-api-check**: Runs banned API checker
3. **build**: Compiles with clang-tidy enabled

All jobs must pass before merging.

### Local Pre-Commit Checks

Before committing, run these checks locally:

```bash
# 1. Format code
find core -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# 2. Check banned APIs
.github/scripts/check_banned_apis.sh

# 3. Build with clang-tidy
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_CLANG_TIDY=ON
cmake --build build
```

## Rationale

These enforcement mechanisms ensure:

- **Deterministic Performance**: No hidden allocations, no unpredictable control flow
- **Explicit Ownership**: Clear memory management without shared ownership
- **Cache Efficiency**: Data-oriented patterns over object-oriented abstractions
- **Measurable Behavior**: Code that can be profiled and reasoned about

## Exemptions

If you need to use a banned API:

1. **DO NOT** use it in `core/` - Move the code to `runtime/` or `app/`
2. Document why the architectural constraint doesn't apply
3. Get explicit approval in code review

## Troubleshooting

### Build fails with RTTI/exception errors

**Symptom**: `error: cannot use 'throw' with exceptions disabled`

**Solution**: Replace `throw` with `std::expected` or error codes.

### clang-tidy fails but code compiles

**Symptom**: clang-tidy reports errors but GCC/Clang compiles successfully

**Solution**: Fix the clang-tidy warnings - they catch subtle bugs and style issues.

### Format check fails in CI

**Symptom**: CI fails on "Code Formatting Check"

**Solution**:
```bash
find core -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
git add .
git commit --amend --no-edit
git push --force-with-lease
```

## References

- [COPILOT_INSTRUCTIONS.md](../.github/COPILOT_INSTRUCTIONS.md) - Core architectural principles
- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture and design rules
- [CORE_FOLDER_CONTRACT.md](CORE_FOLDER_CONTRACT.md) - Dependency boundaries

---

**Remember**: These restrictions exist to make the system **correct**, **fast**, and **explainable**. They are non-negotiable in `core/`.
