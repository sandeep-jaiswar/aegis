# aegis
Aegis is a C++-native, GPU-first application runtime for performance‑critical systems

## Quick Start

```bash
# Build the project
aegis build

# Run a module
aegis run app.aegis

# Run benchmarks
aegis bench
```

See [Developer Workflow Guide](docs/DEVELOPER_WORKFLOW.md) for detailed usage.

## Specifications

Aegis v1.0.0 specifications are frozen and production-ready. Start here:

- **[Specification Index](docs/SPECIFICATION_INDEX.md)** - Complete guide to all specifications
- **[Core Runtime Spec](docs/SPEC_CORE_V1.md)** - Frame lifecycle, memory, events, layout
- **[Determinism Contract](docs/DETERMINISM.md)** - Replay guarantees and verification
- **[Memory Safety Spec](docs/MEMORY_SYSTEM.md)** - Allocators, lifetimes, forbidden patterns
- **[Architecture](docs/ARCHITECTURE.md)** - System design and principles
- **[Core Folder Contract](docs/CORE_FOLDER_CONTRACT.md)** - Dependency boundaries

## Benchmarks

Reproducible performance benchmarks and browser comparisons:

- **[Benchmark Corpus](benchmarks/README.md)** - Standard workload corpus and validation
- **[Browser vs Native Comparison](benchmarks/BROWSER_NATIVE_COMPARISON.md)** - Performance comparison report
- **[Workload Specifications](benchmarks/corpus/CORPUS_SPECIFICATION.md)** - Canonical benchmark workloads

**Key Guarantees:**
- ✅ Same inputs → byte-identical outputs (determinism)
- ✅ 7-phase frame execution (begin → apply → update → layout → build → diff → end)
- ✅ O(1) memory allocation with arena/frame/pool allocators
- ✅ No undefined behavior in normal operation
- ✅ Full replay capability for debugging and verification
