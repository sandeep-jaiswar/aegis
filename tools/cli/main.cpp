// Aegis CLI - Developer workflow tool
// Provides: build, run, replay, and bench commands
// Design principle: No magic, inspectable artifacts, explicit operations

#include <cstdio>
#include <cstdlib>
#include <cstring>

// Command enumeration
enum class command_type {
    none,
    build,
    run,
    replay,
    bench,
    help,
    version
};

// CLI configuration
struct cli_config {
    command_type command{command_type::none};
    const char* target{nullptr};
    const char* build_type{"Release"};
    const char* module_path{nullptr};
    const char* replay_path{nullptr};
    const char* record_path{nullptr};
    const char* benchmark_name{nullptr};
    bool verbose{false};
    bool show_artifacts{false};
};

// Parse command line arguments
static cli_config parse_args(int argc, char** argv) {
    cli_config config{};
    
    if (argc < 2) {
        config.command = command_type::help;
        return config;
    }
    
    // Parse command
    const char* cmd = argv[1];
    if (strcmp(cmd, "build") == 0) {
        config.command = command_type::build;
    } else if (strcmp(cmd, "run") == 0) {
        config.command = command_type::run;
    } else if (strcmp(cmd, "replay") == 0) {
        config.command = command_type::replay;
    } else if (strcmp(cmd, "bench") == 0) {
        config.command = command_type::bench;
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        config.command = command_type::help;
    } else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 || strcmp(cmd, "-v") == 0) {
        config.command = command_type::version;
    } else {
        config.command = command_type::none;
    }
    
    // Parse command-specific options
    for (int i = 2; i < argc; ++i) {
        const char* arg = argv[i];
        
        if (strcmp(arg, "--verbose") == 0) {
            config.verbose = true;
        } else if (strcmp(arg, "--show-artifacts") == 0) {
            config.show_artifacts = true;
        } else if (strcmp(arg, "--type") == 0 && i + 1 < argc) {
            config.build_type = argv[++i];
        } else if (strcmp(arg, "--target") == 0 && i + 1 < argc) {
            config.target = argv[++i];
        } else if (strcmp(arg, "--module") == 0 && i + 1 < argc) {
            config.module_path = argv[++i];
        } else if (strcmp(arg, "--replay") == 0 && i + 1 < argc) {
            config.replay_path = argv[++i];
        } else if (strcmp(arg, "--record") == 0 && i + 1 < argc) {
            config.record_path = argv[++i];
        } else if (strcmp(arg, "--name") == 0 && i + 1 < argc) {
            config.benchmark_name = argv[++i];
        } else if (arg[0] != '-' && config.module_path == nullptr) {
            // Positional argument (module path for run/replay)
            config.module_path = arg;
        }
    }
    
    return config;
}

// Print usage information
static void print_usage(const char* program_name) {
    printf("Aegis CLI - Developer workflow tool\n\n");
    printf("Usage: %s <command> [options]\n\n", program_name);
    printf("Commands:\n");
    printf("  build    Build the Aegis runtime and applications\n");
    printf("  run      Run an Aegis module\n");
    printf("  replay   Replay a recorded session\n");
    printf("  bench    Run benchmarks\n");
    printf("  help     Show this help message\n");
    printf("  version  Show version information\n\n");
    printf("Build Options:\n");
    printf("  --type <type>         Build type: Release, Debug (default: Release)\n");
    printf("  --target <target>     Specific target to build (optional)\n");
    printf("  --verbose             Show detailed build output\n");
    printf("  --show-artifacts      List all build artifacts\n\n");
    printf("Run Options:\n");
    printf("  --module <path>       Module to run (.aegis file)\n");
    printf("  --record <file>       Record events to file\n");
    printf("  --verbose             Show detailed runtime output\n\n");
    printf("Replay Options:\n");
    printf("  --replay <file>       Replay events from file\n");
    printf("  --module <path>       Module to replay with\n");
    printf("  --verbose             Show detailed replay output\n\n");
    printf("Bench Options:\n");
    printf("  --name <benchmark>    Specific benchmark to run (optional)\n");
    printf("  --verbose             Show detailed benchmark output\n");
    printf("  --show-artifacts      Show benchmark result artifacts\n\n");
    printf("Examples:\n");
    printf("  %s build                           # Build in Release mode\n", program_name);
    printf("  %s build --type Debug              # Build in Debug mode\n", program_name);
    printf("  %s run app.aegis                   # Run a module\n", program_name);
    printf("  %s run app.aegis --record log.bin  # Run and record\n", program_name);
    printf("  %s replay --replay log.bin         # Replay session\n", program_name);
    printf("  %s bench                           # Run all benchmarks\n", program_name);
    printf("  %s bench --name frame_lifecycle    # Run specific benchmark\n", program_name);
}

// Print version information
static void print_version() {
    printf("Aegis CLI v0.1.0\n");
    printf("Runtime: Aegis Core v0.1.0\n");
    printf("Build tool: CMake\n");
}

// Execute build command
static int execute_build(const cli_config& config) {
    printf("=== Aegis Build ===\n");
    printf("Build type: %s\n", config.build_type);
    
    // Step 1: Configure with CMake
    printf("\n[1/3] Configuring build...\n");
    char configure_cmd[512];
    snprintf(configure_cmd, sizeof(configure_cmd),
             "cmake -B build -DCMAKE_BUILD_TYPE=%s%s",
             config.build_type,
             config.verbose ? "" : " 2>&1 | grep -E '(Configuring|Generating|Build files)'");
    
    if (config.verbose) {
        printf("Command: %s\n", configure_cmd);
    }
    
    int result = system(configure_cmd);
    if (result != 0) {
        fprintf(stderr, "Error: CMake configuration failed\n");
        return 1;
    }
    printf("✓ Configuration complete\n");
    
    // Step 2: Build
    printf("\n[2/3] Building...\n");
    char build_cmd[512];
    if (config.target != nullptr) {
        snprintf(build_cmd, sizeof(build_cmd),
                 "cmake --build build --target %s%s",
                 config.target,
                 config.verbose ? "" : " 2>&1 | grep -E '(Building|Linking|\\[.*%\\])'");
    } else {
        snprintf(build_cmd, sizeof(build_cmd),
                 "cmake --build build%s",
                 config.verbose ? "" : " 2>&1 | grep -E '(Building|Linking|\\[.*%\\])'");
    }
    
    if (config.verbose) {
        printf("Command: %s\n", build_cmd);
    }
    
    result = system(build_cmd);
    if (result != 0) {
        fprintf(stderr, "Error: Build failed\n");
        return 1;
    }
    printf("✓ Build complete\n");
    
    // Step 3: List artifacts
    printf("\n[3/3] Build artifacts:\n");
    int ret = system("find build -type f \\( -name '*.a' -o -name '*.so' -o -name 'aegis_shell' -o -name 'compile_commands.json' \\) -exec ls -lh {} \\;");
    (void)ret; // Intentionally ignore - this is informational output only
    
    if (config.show_artifacts) {
        printf("\nDetailed artifact listing:\n");
        ret = system("find build -type f ! -path '*/CMakeFiles/*' ! -name '*.o' -exec ls -lh {} \\;");
        (void)ret; // Intentionally ignore - this is informational output only
    }
    
    printf("\n✓ Build artifacts are in: build/\n");
    printf("✓ Core library: build/core/libaegis_core.a\n");
    printf("✓ Shell executable: build/shell/aegis_shell\n");
    
    return 0;
}

// Execute run command
static int execute_run(const cli_config& config) {
    printf("=== Aegis Run ===\n");
    
    if (config.module_path == nullptr) {
        fprintf(stderr, "Error: No module specified\n");
        fprintf(stderr, "Use: aegis run <module.aegis> or aegis run --module <module.aegis>\n");
        return 1;
    }
    
    printf("Module: %s\n", config.module_path);
    
    // Check if shell is built
    const char* shell_path = "build/shell/aegis_shell";
    if (system("test -f build/shell/aegis_shell") != 0) {
        fprintf(stderr, "Error: Shell not found. Run 'aegis build' first.\n");
        return 1;
    }
    
    // Build command
    char run_cmd[1024];
    if (config.record_path != nullptr) {
        printf("Recording to: %s\n", config.record_path);
        snprintf(run_cmd, sizeof(run_cmd), "%s %s --record %s%s",
                 shell_path, config.module_path, config.record_path,
                 config.verbose ? "" : " 2>&1");
    } else {
        snprintf(run_cmd, sizeof(run_cmd), "%s %s%s",
                 shell_path, config.module_path,
                 config.verbose ? "" : " 2>&1");
    }
    
    printf("\n[Running module...]\n");
    if (config.verbose) {
        printf("Command: %s\n", run_cmd);
    }
    
    int result = system(run_cmd);
    
    if (config.record_path != nullptr && result == 0) {
        printf("\n✓ Recording saved to: %s\n", config.record_path);
        // Show recording details
        char stat_cmd[512];
        snprintf(stat_cmd, sizeof(stat_cmd), "ls -lh %s 2>/dev/null || echo '(Recording file not created)'", config.record_path);
        int ret = system(stat_cmd);
        (void)ret; // Intentionally ignore - this is informational output only
    }
    
    return result;
}

// Execute replay command
static int execute_replay(const cli_config& config) {
    printf("=== Aegis Replay ===\n");
    
    if (config.replay_path == nullptr) {
        fprintf(stderr, "Error: No replay file specified\n");
        fprintf(stderr, "Use: aegis replay --replay <file.bin>\n");
        return 1;
    }
    
    if (config.module_path == nullptr) {
        fprintf(stderr, "Error: No module specified\n");
        fprintf(stderr, "Use: aegis replay --replay <file.bin> --module <module.aegis>\n");
        return 1;
    }
    
    printf("Replay file: %s\n", config.replay_path);
    printf("Module: %s\n", config.module_path);
    
    // Check if shell is built
    const char* shell_path = "build/shell/aegis_shell";
    if (system("test -f build/shell/aegis_shell") != 0) {
        fprintf(stderr, "Error: Shell not found. Run 'aegis build' first.\n");
        return 1;
    }
    
    // Check if replay file exists
    char check_cmd[512];
    snprintf(check_cmd, sizeof(check_cmd), "test -f %s", config.replay_path);
    if (system(check_cmd) != 0) {
        fprintf(stderr, "Error: Replay file not found: %s\n", config.replay_path);
        return 1;
    }
    
    // Show replay file info
    printf("\nReplay file details:\n");
    char stat_cmd[512];
    snprintf(stat_cmd, sizeof(stat_cmd), "ls -lh %s", config.replay_path);
    int ret = system(stat_cmd);
    (void)ret; // Intentionally ignore - this is informational output only
    
    // Build command
    char replay_cmd[1024];
    snprintf(replay_cmd, sizeof(replay_cmd), "%s %s --replay %s%s",
             shell_path, config.module_path, config.replay_path,
             config.verbose ? "" : " 2>&1");
    
    printf("\n[Replaying session...]\n");
    if (config.verbose) {
        printf("Command: %s\n", replay_cmd);
    }
    
    int result = system(replay_cmd);
    
    if (result == 0) {
        printf("\n✓ Replay completed successfully\n");
    } else {
        fprintf(stderr, "\n✗ Replay failed\n");
    }
    
    return result;
}

// Execute bench command
static int execute_bench(const cli_config& config) {
    printf("=== Aegis Benchmark ===\n");
    
    // Check if benchmark binaries exist
    printf("Looking for benchmark executables...\n");
    
    // List available benchmarks
    printf("\nAvailable benchmarks:\n");
    int ret = system("find build/core/benchmark -type f -executable 2>/dev/null | while read -r bench; do echo \"  - $(basename \"$bench\")\"; done || echo '  (No benchmarks found - build first)'");
    (void)ret; // Intentionally ignore - this is informational output only
    
    // If specific benchmark requested, run it
    if (config.benchmark_name != nullptr) {
        printf("\nRunning benchmark: %s\n", config.benchmark_name);
        
        // Use larger buffers to avoid truncation warnings
        char bench_path[4096];
        const int path_len = snprintf(bench_path, sizeof(bench_path), "build/core/benchmark/%s", config.benchmark_name);
        if (path_len < 0 || static_cast<size_t>(path_len) >= sizeof(bench_path)) {
            fprintf(stderr, "Error: Benchmark name too long\n");
            return 1;
        }
        
        char check_cmd[4096];
        const int check_len = snprintf(check_cmd, sizeof(check_cmd), "test -x %s", bench_path);
        if (check_len < 0 || static_cast<size_t>(check_len) >= sizeof(check_cmd)) {
            fprintf(stderr, "Error: Command too long\n");
            return 1;
        }
        
        if (system(check_cmd) != 0) {
            fprintf(stderr, "Error: Benchmark not found or not executable: %s\n", bench_path);
            fprintf(stderr, "Run 'aegis build' to build benchmarks.\n");
            return 1;
        }
        
        char bench_cmd[4096];
        const int cmd_len = snprintf(bench_cmd, sizeof(bench_cmd), "%s%s",
                 bench_path,
                 config.verbose ? "" : " 2>&1");
        if (cmd_len < 0 || static_cast<size_t>(cmd_len) >= sizeof(bench_cmd)) {
            fprintf(stderr, "Error: Command too long\n");
            return 1;
        }
        
        if (config.verbose) {
            printf("Command: %s\n", bench_cmd);
        }
        
        printf("\n[Running benchmark...]\n");
        int result = system(bench_cmd);
        
        if (result == 0) {
            printf("\n✓ Benchmark completed\n");
        }
        
        return result;
    }
    
    // Run all benchmarks
    printf("\nRunning all benchmarks...\n");
    const char* bench_cmd = "find build/core/benchmark -type f -executable 2>/dev/null | while read -r bench; do echo \"\\n=== Running $(basename \"$bench\") ===\"; \"$bench\" 2>&1; done";
    
    if (config.verbose) {
        printf("Command: %s\n", bench_cmd);
    }
    
    int result = system(bench_cmd);
    
    if (config.show_artifacts) {
        printf("\n\nBenchmark artifacts:\n");
        printf("(Benchmark results are printed to stdout - redirect to save)\n");
        printf("Example: aegis bench > benchmark_results.txt\n");
    }
    
    printf("\n✓ All benchmarks completed\n");
    
    return result;
}

int main(int argc, char** argv) {
    cli_config config = parse_args(argc, argv);
    
    switch (config.command) {
        case command_type::help:
            print_usage(argv[0]);
            return 0;
            
        case command_type::version:
            print_version();
            return 0;
            
        case command_type::build:
            return execute_build(config);
            
        case command_type::run:
            return execute_run(config);
            
        case command_type::replay:
            return execute_replay(config);
            
        case command_type::bench:
            return execute_bench(config);
            
        case command_type::none:
        default:
            fprintf(stderr, "Error: Unknown command\n\n");
            print_usage(argv[0]);
            return 1;
    }
}
