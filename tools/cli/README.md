# Aegis CLI - Developer Workflow Tool

The Aegis CLI provides a unified interface for all developer workflow operations.

## Design Principles

1. **No Magic**: Every operation is transparent and explicit
2. **Inspectable Artifacts**: All operations produce artifacts that can be examined
3. **Explicit Operations**: No hidden behavior or implicit state

## Commands

### `aegis build`

Build the Aegis runtime and applications using CMake.

**Usage:**
```bash
aegis build                    # Release build (default)
aegis build --type Debug       # Debug build
aegis build --verbose          # Show full build output
aegis build --show-artifacts   # List all build artifacts
```

**Artifacts Produced:**
- `build/core/libaegis_core.a` - Core library
- `build/shell/aegis_shell` - Shell executable
- `build/compile_commands.json` - Compilation database
- Build logs visible in output

**Design Notes:**
- Wraps CMake with sensible defaults
- Makes build process explicit and reproducible
- All build artifacts are in `build/` directory
- No hidden configuration or state

---

### `aegis run`

Run an Aegis module using the shell runtime.

**Usage:**
```bash
aegis run app.aegis                   # Run module
aegis run app.aegis --record log.bin  # Run and record events
aegis run app.aegis --verbose         # Show detailed output
```

**Artifacts Produced:**
- Event recording file (when using `--record`)
- Runtime logs (visible in output)

**Design Notes:**
- Wraps `aegis_shell` executable
- Recording creates deterministic event log
- All runtime behavior is logged
- No hidden state or caching

---

### `aegis replay`

Replay a previously recorded session for deterministic testing.

**Usage:**
```bash
aegis replay --replay log.bin --module app.aegis
aegis replay --replay log.bin --module app.aegis --verbose
```

**Artifacts Produced:**
- Replay logs (visible in output)
- Verification that replay matches original

**Design Notes:**
- Uses recorded event stream
- Deterministic replay (same inputs → same outputs)
- Useful for debugging and testing
- Validates workload hash

---

### `aegis bench`

Run performance benchmarks with detailed metrics.

**Usage:**
```bash
aegis bench                           # Run all benchmarks
aegis bench --name frame_lifecycle    # Run specific benchmark
aegis bench --verbose                 # Show detailed output
aegis bench --show-artifacts          # Show result artifacts
```

**Artifacts Produced:**
- Benchmark results (stdout)
- Percentile metrics (P50, P90, P95, P99, P99.9)
- Timing statistics

**Design Notes:**
- Runs deterministic benchmarks
- All results printed to stdout (redirect to save)
- No hidden metrics or caching
- Reproducible performance measurements

---

## Artifact Philosophy

Every Aegis CLI operation produces **inspectable artifacts**:

1. **Build Artifacts**: 
   - Libraries, executables in `build/`
   - All files can be examined with standard tools

2. **Runtime Artifacts**:
   - Event recordings are binary logs
   - Can be replayed deterministically
   - Format is documented

3. **Benchmark Artifacts**:
   - Results printed to stdout
   - Redirect to file for storage
   - Machine-readable format

4. **No Hidden State**:
   - No caches or hidden files
   - No implicit configuration
   - All state is explicit and visible

## Integration with Existing Tools

The CLI wraps existing tools without replacing them:

- `aegis build` → wraps CMake
- `aegis run` → wraps `aegis_shell`
- `aegis replay` → wraps `aegis_shell --replay`
- `aegis bench` → wraps benchmark executables

**You can still use the underlying tools directly** if you prefer:

```bash
# These are equivalent:
aegis build --type Release
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

# These are equivalent:
aegis run app.aegis
./build/shell/aegis_shell app.aegis
```

## Building the CLI

The CLI is built as part of the normal build:

```bash
cmake -B build
cmake --build build
# Binary at: build/tools/cli/aegis
```

Or install it system-wide:

```bash
cmake -B build
cmake --build build
sudo cmake --install build
# Binary at: /usr/local/bin/aegis
```

## Examples

**Complete workflow:**

```bash
# 1. Build everything
aegis build

# 2. Run an application
aegis run myapp.aegis --record session.bin

# 3. Replay the session
aegis replay --replay session.bin --module myapp.aegis

# 4. Run benchmarks
aegis bench > benchmark_results.txt
```

**Development workflow:**

```bash
# Debug build
aegis build --type Debug --verbose

# Run with verbose output
aegis run myapp.aegis --verbose

# Run specific benchmark
aegis bench --name frame_allocator_benchmark
```

## See Also

- [Build Documentation](../../BUILD.md)
- [Architecture](../../docs/ARCHITECTURE.md)
- [Benchmark System](../../core/benchmark/README.md)
