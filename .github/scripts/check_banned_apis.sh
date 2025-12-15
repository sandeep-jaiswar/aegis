#!/bin/bash
# Aegis Banned API Checker
# Enforces architectural constraints from COPILOT_INSTRUCTIONS.md
# Exit code 1 if any banned APIs are detected

set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXIT_CODE=0

echo "=== Aegis Banned API Checker ==="
echo ""

# Check for shared_ptr usage
echo "Checking for banned std::shared_ptr usage..."
if grep -rn --include="*.cpp" --include="*.hpp" "std::shared_ptr" "$REPO_ROOT/core" 2>/dev/null; then
    echo "ERROR: std::shared_ptr is banned in core/ (violates COPILOT_INSTRUCTIONS.md)"
    echo "  Use unique_ptr, raw pointers with clear ownership, or arena allocators instead"
    EXIT_CODE=1
else
    echo "✓ No std::shared_ptr usage found"
fi
echo ""

# Check for exception throws in hot paths (simplified check)
echo "Checking for exception throws in core/..."
if grep -rn --include="*.cpp" --include="*.hpp" "throw " "$REPO_ROOT/core" 2>/dev/null; then
    echo "ERROR: throw statements found in core/ (exceptions banned in hot paths)"
    echo "  Use std::expected or error codes instead"
    EXIT_CODE=1
else
    echo "✓ No throw statements found"
fi
echo ""

# Check for RTTI usage
echo "Checking for RTTI usage (dynamic_cast, typeid)..."
if grep -rn --include="*.cpp" --include="*.hpp" -E "dynamic_cast|typeid" "$REPO_ROOT/core" 2>/dev/null; then
    echo "ERROR: RTTI usage found (dynamic_cast/typeid banned in hot paths)"
    echo "  Use static polymorphism or explicit type checking"
    EXIT_CODE=1
else
    echo "✓ No RTTI usage found"
fi
echo ""

# Check for direct new/delete in hot paths
echo "Checking for direct heap allocations (new/delete)..."
if grep -rn --include="*.cpp" --include="*.hpp" -E "\bnew\b|\bdelete\b" "$REPO_ROOT/core" 2>/dev/null; then
    echo "WARNING: Direct new/delete usage found in core/"
    echo "  Prefer arena, frame, or pool allocators for deterministic performance"
    # Not failing build for this, just warning
fi
echo ""

# Check for std::function (type erasure overhead)
echo "Checking for std::function usage..."
if grep -rn --include="*.cpp" --include="*.hpp" "std::function" "$REPO_ROOT/core" 2>/dev/null; then
    echo "WARNING: std::function found - consider templates for zero-cost abstraction"
    # Not failing build for this, just warning
fi
echo ""

if [ $EXIT_CODE -eq 0 ]; then
    echo "=== All banned API checks passed ==="
else
    echo "=== Banned API checks FAILED ==="
fi

exit $EXIT_CODE
