// Runtime Conformance Test Suite - Main
// Ticket: REF-002

#include "conformance_tests.hpp"

#include <cstdio>

using namespace aegis::runtime::conformance;

int main(int argc, char** argv) {
    printf("\n");
    printf("╔═════════════════════════════════════════════════════════════════╗\n");
    printf("║  Aegis Runtime Conformance Test Suite                          ║\n");
    printf("║  Ticket: REF-002                                                ║\n");
    printf("║                                                                 ║\n");
    printf("║  Purpose: Verify runtime implementation conformance            ║\n");
    printf("║  Tests: Determinism, Replay, Memory, Capabilities              ║\n");
    printf("╚═════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Check for test category argument
    if (argc > 1) {
        const char* category = argv[1];

        if (strcmp(category, "determinism") == 0) {
            printf("Running DETERMINISM tests only...\n\n");
            test_stats stats = run_determinism_tests();
            print_test_stats(stats);
            return (stats.failed == 0 && stats.errors == 0) ? 0 : 1;
        } else if (strcmp(category, "replay") == 0) {
            printf("Running REPLAY tests only...\n\n");
            test_stats stats = run_replay_tests();
            print_test_stats(stats);
            return (stats.failed == 0 && stats.errors == 0) ? 0 : 1;
        } else if (strcmp(category, "memory") == 0) {
            printf("Running MEMORY tests only...\n\n");
            test_stats stats = run_memory_tests();
            print_test_stats(stats);
            return (stats.failed == 0 && stats.errors == 0) ? 0 : 1;
        } else if (strcmp(category, "capability") == 0) {
            printf("Running CAPABILITY tests only...\n\n");
            test_stats stats = run_capability_tests();
            print_test_stats(stats);
            return (stats.failed == 0 && stats.errors == 0) ? 0 : 1;
        } else if (strcmp(category, "--help") == 0 || strcmp(category, "-h") == 0) {
            printf("Usage: %s [category]\n\n", argv[0]);
            printf("Categories:\n");
            printf("  determinism  - Run determinism tests only\n");
            printf("  replay       - Run event replay tests only\n");
            printf("  memory       - Run memory behavior tests only\n");
            printf("  capability   - Run capability enforcement tests only\n");
            printf("  (none)       - Run all tests\n");
            printf("\n");
            return 0;
        } else {
            printf("Unknown test category: %s\n", category);
            printf("Use --help for usage information\n");
            return 1;
        }
    }

    // Run all tests
    test_stats stats = run_conformance_tests();

    // Return 0 if all tests passed, 1 otherwise
    return (stats.failed == 0 && stats.errors == 0) ? 0 : 1;
}
