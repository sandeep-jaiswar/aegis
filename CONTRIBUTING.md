# Contributing to Aegis

Thank you for your interest in contributing to Aegis! This document provides guidelines for contributing to the project.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Development Workflow](#development-workflow)
3. [Code Standards](#code-standards)
4. [Testing Requirements](#testing-requirements)
5. [Documentation Requirements](#documentation-requirements)
6. [Pull Request Process](#pull-request-process)
7. [Specification Changes](#specification-changes)
8. [Architecture Contracts](#architecture-contracts)

---

## Getting Started

### Prerequisites

Before contributing, ensure you have:

- **C++23 compatible compiler**
  - GCC 13.0 or higher
  - Clang 18.0 or higher
- **CMake 3.20 or higher**
- **Git**
- **clang-format** (for code formatting)
- **clang-tidy** (optional, for static analysis)

### Initial Setup

1. **Fork the repository** on GitHub
2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/aegis.git
   cd aegis
   ```
3. **Add upstream remote**:
   ```bash
   git remote add upstream https://github.com/sandeep-jaiswar/aegis.git
   ```
4. **Build the project**:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```
5. **Run tests to verify setup**:
   ```bash
   ./build/runtime/conformance_tests/conformance_tests
   ./build/core/module/capability_test
   ```

---

## Development Workflow

### 1. Create a Feature Branch

```bash
git checkout -b feature/your-feature-name
```

**Branch Naming:**
- `feature/` - New features
- `fix/` - Bug fixes
- `docs/` - Documentation updates
- `test/` - Test additions or fixes
- `refactor/` - Code refactoring

### 2. Make Your Changes

- Keep changes focused and atomic
- Follow the [Code Standards](#code-standards)
- Add tests for new functionality
- Update documentation as needed

### 3. Format Your Code

```bash
# Format all changed files
find core -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### 4. Build and Test

```bash
# Build
cmake --build build

# Run conformance tests
./build/runtime/conformance_tests/conformance_tests

# Run capability tests
./build/core/module/capability_test

# Run relevant demos to verify behavior
./build/core/memory_demo
./build/core/state_demo
```

### 5. Commit Your Changes

```bash
git add .
git commit -m "type: brief description"
```

**Commit Message Format:**
```
type: brief description (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.

- Bullet points for multiple changes
- Reference issues with #123
```

**Commit Types:**
- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation changes
- `test:` - Test additions or fixes
- `refactor:` - Code refactoring
- `perf:` - Performance improvements
- `chore:` - Build/tooling changes

### 6. Push and Create Pull Request

```bash
git push origin feature/your-feature-name
```

Then create a Pull Request on GitHub.

---

## Code Standards

### Formatting

**Required:** All code must be formatted with `clang-format` using the project's `.clang-format` configuration.

```bash
# Format a single file
clang-format -i core/memory/arena_allocator.cpp

# Format all core files
find core -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

**CI Enforcement:** The CI pipeline will fail if code is not properly formatted.

### Compilation Standards

All code must compile with:
- ✅ `-Wall -Wextra -Wpedantic` (all warnings enabled)
- ✅ `-Werror` (warnings treated as errors)
- ✅ C++23 standard
- ✅ No RTTI (`-fno-rtti`)
- ✅ No exceptions (`-fno-exceptions`)
- ✅ IEEE 754 floating-point compliance (`-fno-fast-math`, `-ffp-contract=off`)

### Coding Conventions

**Naming:**
- Types: `PascalCase` (e.g., `ArenaAllocator`)
- Functions: `snake_case` (e.g., `allocate_frame`)
- Variables: `snake_case` (e.g., `frame_count`)
- Constants: `SCREAMING_SNAKE_CASE` (e.g., `MAX_FRAME_SIZE`)
- Private members: `snake_case_` (e.g., `buffer_size_`)

**File Organization:**
- Header files: `.hpp`
- Implementation files: `.cpp`
- One class per file (generally)
- Header guards: `#pragma once`

**Documentation:**
- All public APIs must have documentation comments
- Explain *why*, not just *what*
- Include usage examples for complex APIs

**Example:**
```cpp
/// Allocates memory from the frame allocator.
///
/// Frame allocator provides O(1) allocation with automatic reset
/// at end_frame(). Memory is valid until end of current frame.
///
/// @param size Number of bytes to allocate
/// @param alignment Required alignment (must be power of 2)
/// @return Pointer to allocated memory, or nullptr if allocation fails
///
/// Example:
///   void* ptr = frame_alloc(1024, 16);
///   // Use ptr...
///   // Automatically freed at end_frame()
void* frame_alloc(size_t size, size_t alignment) noexcept;
```

---

## Testing Requirements

### When to Add Tests

**Required:**
- All new features must have tests
- All bug fixes must have regression tests
- All API changes must have updated tests

**Test Types:**

1. **Conformance Tests** (`runtime/conformance_tests/`)
   - For runtime-level behavior
   - Implementation-agnostic
   - Test invariants from specifications

2. **Unit Tests** (planned for future)
   - For individual component testing
   - Test specific functions/classes

3. **Capability Tests** (`core/module/capability_test.cpp`)
   - For capability model behavior
   - Test capability enforcement

### Adding Conformance Tests

See `runtime/conformance_tests/README.md` for detailed instructions.

**Quick Example:**
```cpp
// In conformance_tests.hpp
test_result test_my_feature() noexcept;

// In conformance_tests.cpp
test_result test_my_feature() noexcept {
    printf("Test: My feature behavior\n");
    printf("Invariant: Specific invariant being tested\n");
    
    // Test implementation
    if (/* test passes */) {
        printf("  ✓ PASSED\n");
        return test_result::success;
    }
    
    printf("  ✗ FAILED: Reason\n");
    return test_result::failure;
}

// Add to test array
static const test_case all_tests[] = {
    // ...
    {"my_feature", "My feature description",
     "Invariant statement", test_my_feature},
};
```

### Running Tests

```bash
# Run all conformance tests
./build/runtime/conformance_tests/conformance_tests

# Run specific category
./build/runtime/conformance_tests/conformance_tests determinism

# Run capability tests
./build/core/module/capability_test
```

---

## Documentation Requirements

### What Needs Documentation

**Required:**
- All new public APIs
- All specification changes (see [Specification Changes](#specification-changes))
- All architectural decisions
- All new features

**Recommended:**
- Implementation notes for complex code
- Performance characteristics
- Usage examples

### Documentation Locations

| Type | Location |
|------|----------|
| Specifications | `docs/` |
| API Documentation | Inline comments in headers |
| Architecture | `docs/ARCHITECTURE.md` |
| Developer Workflow | `docs/DEVELOPER_WORKFLOW.md` |
| Build Instructions | `BUILD.md` |
| Examples | Demo files (`core/*/demo_*.cpp`) |

### Documentation Style

- Use Markdown for all documentation files
- Use Doxygen-style comments for inline documentation
- Include code examples where helpful
- Explain *why*, not just *what*
- Link to related specifications

---

## Pull Request Process

### Before Submitting

**Checklist:**
- [ ] Code follows formatting standards (clang-format)
- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] New tests added for new features
- [ ] Documentation updated
- [ ] Commit messages follow format
- [ ] Changes are focused and atomic

### PR Description Template

```markdown
## Description
Brief description of the changes

## Motivation
Why is this change needed?

## Changes
- List of specific changes
- Include any breaking changes

## Testing
- How was this tested?
- What tests were added?

## Documentation
- What documentation was updated?
- Link to relevant specifications

## Checklist
- [ ] Code formatted with clang-format
- [ ] Compiles without warnings
- [ ] All tests pass
- [ ] New tests added
- [ ] Documentation updated
```

### Review Process

1. **Automated Checks** - CI/CD pipeline runs:
   - Code formatting check
   - Banned API check
   - Build with GCC and Clang
   - Conformance tests
   - Performance regression check

2. **Manual Review** - Maintainers review:
   - Code quality
   - Adherence to specifications
   - Test coverage
   - Documentation completeness

3. **Approval** - At least one maintainer approval required

4. **Merge** - Maintainer merges PR

---

## Specification Changes

**⚠️ IMPORTANT:** All specifications in `docs/` are **frozen at version 1.0.0**.

### For Frozen Specifications

**Allowed Changes:**
- ✅ Clarifications that don't change semantics
- ✅ Typo fixes
- ✅ Adding examples
- ✅ Improving wording without changing meaning

**Requires New Version:**
- ❌ Adding new requirements
- ❌ Removing requirements
- ❌ Changing existing behavior
- ❌ Modifying invariants

### Proposing Specification Changes

1. **Open an issue** describing the proposed change
2. **Wait for discussion** and maintainer feedback
3. **Create RFC** (Request for Comments) document if accepted
4. **Update specification** with new version number
5. **Update implementation** to match new specification
6. **Update tests** to verify new behavior

**Specification Versioning:**
- Major version bump (2.0.0) for breaking changes
- Minor version bump (1.1.0) for backward-compatible additions
- Patch version bump (1.0.1) for clarifications/typos

---

## Architecture Contracts

### Core Folder Contract

**⚠️ NON-NEGOTIABLE RULES** (see `docs/CORE_FOLDER_CONTRACT.md`):

1. **No OS Dependencies**
   - Nothing in `core/` may include OS headers
   - No `<unistd.h>`, `<windows.h>`, `<pthread.h>`, etc.
   - Platform-specific code belongs in `runtime/`

2. **No Runtime Dependencies**
   - Core library is standalone
   - No dependency on runtime implementation
   - Runtime depends on core, not vice versa

3. **No Banned APIs**
   - No `rand()`, `random()`, `time()`, `clock_gettime()`
   - No heap allocation (`new`, `malloc`, `free`)
   - No exceptions, RTTI
   - See `.github/scripts/check_banned_apis.sh` for full list

**Enforcement:**
- Banned API checker runs in CI/CD
- Build fails if contract violated
- Architecture reviews verify compliance

### Determinism Requirements

All code in `core/` must be deterministic (see `docs/DETERMINISM.md`):

- ✅ Same inputs → byte-identical outputs
- ✅ No randomness
- ✅ No system time dependencies
- ✅ No undefined behavior
- ✅ IEEE 754 floating-point compliance

### Memory Management

All allocations must use Aegis allocators (see `docs/MEMORY_SYSTEM.md`):

- ✅ Arena allocator for bulk allocation
- ✅ Frame allocator for per-frame allocation
- ✅ Pool allocator for fixed-size objects
- ❌ No `new`, `malloc`, `delete`, `free`

---

## Communication

### Reporting Bugs

**Use GitHub Issues** with the following information:

- **Description** - Clear description of the bug
- **Steps to Reproduce** - Exact steps to reproduce
- **Expected Behavior** - What should happen
- **Actual Behavior** - What actually happens
- **Environment** - OS, compiler, version
- **Logs** - Relevant error messages or logs

### Asking Questions

**Use GitHub Discussions** for:
- General questions about Aegis
- Design discussions
- Feature requests
- Implementation questions

### Security Issues

**DO NOT** open public issues for security vulnerabilities.

Instead:
1. Email maintainers privately
2. Include detailed description
3. Wait for response before public disclosure

---

## Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inclusive environment for all contributors, regardless of background or identity.

### Expected Behavior

- ✅ Be respectful and professional
- ✅ Welcome newcomers
- ✅ Provide constructive feedback
- ✅ Focus on what's best for the project
- ✅ Accept criticism gracefully

### Unacceptable Behavior

- ❌ Harassment or discrimination
- ❌ Trolling or insulting comments
- ❌ Personal or political attacks
- ❌ Publishing others' private information

### Enforcement

Violations may result in:
- Warning
- Temporary ban
- Permanent ban

Report violations to project maintainers.

---

## Recognition

Contributors will be recognized in:
- Project README.md (Contributors section)
- Release notes for significant contributions
- Special recognition for major contributions

---

## License

By contributing to Aegis, you agree that your contributions will be licensed under the Apache License 2.0 (see LICENSE file).

All contributed code must be your original work or properly attributed with compatible licensing.

---

## Getting Help

**Resources:**
- 📚 [Documentation](docs/)
- 🏗️ [Architecture Guide](docs/ARCHITECTURE.md)
- 📋 [Specification Index](docs/SPECIFICATION_INDEX.md)
- 🔧 [Developer Workflow](docs/DEVELOPER_WORKFLOW.md)
- 💬 [GitHub Discussions](https://github.com/sandeep-jaiswar/aegis/discussions)

**Contact:**
- GitHub Issues - For bugs and feature requests
- GitHub Discussions - For questions and discussions

---

Thank you for contributing to Aegis! 🎉
