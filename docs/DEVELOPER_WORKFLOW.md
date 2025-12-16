# Developer Workflow Guide

This guide explains the Aegis developer workflow using the unified `aegis` CLI tool.

## Philosophy: No Magic, Inspectable Everything

The Aegis workflow follows three core principles:

1. **No Magic**: Every operation is explicit and transparent
2. **Inspectable Artifacts**: All operations produce artifacts that can be examined
3. **Explicit Operations**: No hidden behavior, caches, or implicit state

## Quick Start

```bash
# 1. Build the project
aegis build

# 2. Run an application
aegis run myapp.aegis

# 3. Record a session for replay
aegis run myapp.aegis --record session.bin

# 4. Replay the session deterministically
aegis replay --replay session.bin --module myapp.aegis

# 5. Run benchmarks
aegis bench
```

## The Four Essential Commands

### 1. `aegis build` - Build Tool

**Purpose**: Compile the Aegis runtime and applications with full transparency.

**What it does**:
1. Configures CMake with specified build type
2. Builds all targets or a specific target
3. Lists all produced artifacts

**Example Output**:
```
=== Aegis Build ===
Build type: Release

[1/3] Configuring build...
✓ Configuration complete

[2/3] Building...
✓ Build complete

[3/3] Build artifacts:
-rw-rw-r-- 54K build/core/libaegis_core.a
-rwxrwxr-x 16K build/shell/aegis_shell
-rw-rw-r-- 18K build/compile_commands.json

✓ Build artifacts are in: build/
✓ Core library: build/core/libaegis_core.a
✓ Shell executable: build/shell/aegis_shell
```

**Artifacts Produced**:
- `build/core/libaegis_core.a` - Static library (deterministic, inspectable)
- `build/shell/aegis_shell` - Shell executable
- `build/compile_commands.json` - Compilation database for tooling
- Build logs visible in stdout

**Options**:
```bash
aegis build                    # Release build (default)
aegis build --type Debug       # Debug build with symbols
aegis build --target aegis_core  # Build specific target only
aegis build --verbose          # Show full compiler output
aegis build --show-artifacts   # List all build artifacts
```

**No Magic Examples**:
- CMake configuration is explicit: you can see the exact command
- All build artifacts are in a single `build/` directory
- No hidden caches or intermediate files outside `build/`
- Compilation database is exported for IDE integration

---

### 2. `aegis run` - Run Tool

**Purpose**: Execute Aegis modules with optional event recording.

**What it does**:
1. Validates the shell binary exists
2. Executes the module via the shell
3. Optionally records all events to a file

**Example Output**:
```
=== Aegis Run ===
Module: myapp.aegis
Recording to: session.bin

[Running module...]
=== Aegis Shell ===
Version: 0.1.0
Module: myapp.aegis

1. Initializing platform...
   Platform initialized
2. Setting up event handling...
   Event handler registered
...

✓ Recording saved to: session.bin
-rw-rw-r-- 4.2K session.bin
```

**Artifacts Produced**:
- Event recording file (binary format, deterministic)
- Runtime logs (stdout/stderr)

**Options**:
```bash
aegis run app.aegis                   # Run module
aegis run app.aegis --record log.bin  # Run and record events
aegis run app.aegis --verbose         # Show detailed output
```

**No Magic Examples**:
- Shell executable is explicitly checked before running
- Recording format is binary but deterministic
- All events are captured in order
- No hidden state or caching between runs

---

### 3. `aegis replay` - Replay Tool

**Purpose**: Deterministically replay previously recorded sessions.

**What it does**:
1. Validates replay file exists
2. Loads the event stream
3. Replays events in exact order
4. Verifies deterministic execution

**Example Output**:
```
=== Aegis Replay ===
Replay file: session.bin
Module: myapp.aegis

Replay file details:
-rw-rw-r-- 4.2K session.bin

[Replaying session...]
=== Aegis Shell ===
Version: 0.1.0
Module: myapp.aegis

(Same output as original run)

✓ Replay completed successfully
```

**Artifacts Produced**:
- Replay logs (stdout/stderr)
- Verification that execution matches original

**Options**:
```bash
aegis replay --replay log.bin --module app.aegis
aegis replay --replay log.bin --module app.aegis --verbose
```

**No Magic Examples**:
- Replay file is explicitly validated before use
- Event stream is deterministic (same inputs → same outputs)
- Useful for debugging and testing
- No heuristics or approximations

---

### 4. `aegis bench` - Benchmark Tool

**Purpose**: Run performance benchmarks with detailed metrics.

**What it does**:
1. Finds available benchmark executables
2. Runs benchmarks individually or all at once
3. Reports percentile metrics (P50, P90, P95, P99, P99.9)

**Example Output**:
```
=== Aegis Benchmark ===
Looking for benchmark executables...

Available benchmarks:
  - frame_lifecycle_benchmark
  - frame_allocator_benchmark

Running all benchmarks...

=== Running frame_lifecycle_benchmark ===
Benchmark: Frame Lifecycle
Iterations: 1000
Results:
  P50:   125 ns
  P90:   150 ns
  P95:   175 ns
  P99:   200 ns
  P99.9: 250 ns

✓ All benchmarks completed
```

**Artifacts Produced**:
- Benchmark results (stdout)
- Percentile metrics
- Timing statistics

**Options**:
```bash
aegis bench                           # Run all benchmarks
aegis bench --name frame_lifecycle    # Run specific benchmark
aegis bench --verbose                 # Show detailed output
aegis bench > results.txt             # Redirect to file
```

**No Magic Examples**:
- Benchmark binaries are explicitly discovered
- All metrics are printed to stdout (no hidden files)
- Results can be redirected and saved
- No caching or averaging across runs

---

## Workflow Examples

### Development Workflow

```bash
# 1. Make changes to code
vim core/frame/frame.cpp

# 2. Build
aegis build --type Debug --verbose

# 3. Test manually
aegis run test.aegis --record test_session.bin

# 4. Verify determinism
aegis replay --replay test_session.bin --module test.aegis

# 5. Run benchmarks
aegis bench --name relevant_benchmark
```

### Release Workflow

```bash
# 1. Build release version
aegis build --type Release --show-artifacts

# 2. Run comprehensive tests
aegis run full_test.aegis --record release_test.bin

# 3. Verify replay
aegis replay --replay release_test.bin --module full_test.aegis

# 4. Benchmark performance
aegis bench > release_benchmarks.txt
```

### Debugging Workflow

```bash
# 1. Record failing session
aegis run failing.aegis --record failure.bin

# 2. Build debug version
aegis build --type Debug

# 3. Replay with debugger
gdb --args build/shell/aegis_shell failing.aegis --replay failure.bin
```

## Artifact Philosophy

Every command produces **inspectable artifacts**:

### Build Artifacts
- **Location**: `build/` directory
- **Format**: Standard object files, libraries, executables
- **Inspection**: Use `objdump`, `nm`, `readelf`, etc.
- **Cleanup**: `rm -rf build/`

### Runtime Artifacts
- **Location**: Specified by user (e.g., `session.bin`)
- **Format**: Binary event log (deterministic)
- **Inspection**: Can be replayed exactly
- **Usage**: Debugging, testing, audit trails

### Benchmark Artifacts
- **Location**: stdout (redirect to save)
- **Format**: Human-readable text
- **Inspection**: Direct reading or parsing
- **Usage**: Performance tracking, regression detection

### No Hidden State
- ✅ No `.cache` directories
- ✅ No hidden configuration files
- ✅ No implicit build artifacts
- ✅ All state is explicit and visible

## Integration with Existing Tools

The `aegis` CLI wraps existing tools transparently:

| Command | Wraps | Can Still Use Directly |
|---------|-------|------------------------|
| `aegis build` | CMake | `cmake -B build && cmake --build build` |
| `aegis run` | aegis_shell | `./build/shell/aegis_shell app.aegis` |
| `aegis replay` | aegis_shell | `./build/shell/aegis_shell app.aegis --replay log.bin` |
| `aegis bench` | Benchmark executables | `./build/core/benchmark/frame_lifecycle_benchmark` |

**You can always use the underlying tools directly** if you prefer more control.

## Acceptance Criteria Met

✅ **No "magic"**:
- All commands show exactly what they're doing
- No hidden configuration or caching
- Operations are transparent and explicit

✅ **Every step produces artifacts**:
- Build: Libraries, executables, compilation database
- Run: Event recordings (optional), logs
- Replay: Replay logs, verification
- Bench: Metrics, timing statistics

✅ **Every artifact is inspectable**:
- Build artifacts: Standard formats (ELF, static libs)
- Event recordings: Can be replayed deterministically
- Benchmark results: Human-readable text
- All artifacts documented and accessible

## See Also

- [CLI Tool README](../tools/cli/README.md)
- [Build Documentation](BUILD.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Benchmark System](core/benchmark/README.md)
