#!/bin/bash
# Quick start script for running external falsification benchmarks

set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Aegis External Falsification - Quick Start                   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check what's available
echo "Checking prerequisites..."
echo ""

# Check for Aegis build
if [ -f "../build/core/browser_comparison_demo" ]; then
    echo -e "${GREEN}✓${NC} Aegis benchmark found"
    HAS_AEGIS=1
else
    echo -e "${YELLOW}⚠${NC} Aegis benchmark not built. Run: cmake -B ../build && cmake --build ../build"
    HAS_AEGIS=0
fi

# Check for Node.js (React)
if command -v node &> /dev/null; then
    echo -e "${GREEN}✓${NC} Node.js found (version $(node --version))"
    HAS_NODE=1
else
    echo -e "${YELLOW}⚠${NC} Node.js not found (needed for React benchmark)"
    HAS_NODE=0
fi

# Check for Qt (Qt benchmark)
if command -v qmake &> /dev/null || [ -d "/usr/lib/qt6" ]; then
    echo -e "${GREEN}✓${NC} Qt found"
    HAS_QT=1
else
    echo -e "${YELLOW}⚠${NC} Qt not found (needed for Qt benchmark)"
    HAS_QT=0
fi

# Check for Python (analysis)
if command -v python3 &> /dev/null; then
    echo -e "${GREEN}✓${NC} Python 3 found"
    HAS_PYTHON=1
else
    echo -e "${RED}✗${NC} Python 3 not found (needed for analysis)"
    HAS_PYTHON=0
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Menu
while true; do
    echo "What would you like to do?"
    echo ""
    echo "  1) Run Aegis benchmark (generate aegis_results.json)"
    echo "  2) Start React benchmark server"
    echo "  3) Open Canvas benchmark (browser)"
    echo "  4) Build and run Qt benchmark"
    echo "  5) Analyze all results"
    echo "  6) Clean all result files"
    echo "  7) Exit"
    echo ""
    read -p "Enter choice [1-7]: " choice

    case $choice in
        1)
            if [ $HAS_AEGIS -eq 1 ]; then
                echo ""
                echo "Running Aegis benchmark..."
                ../build/core/browser_comparison_demo | python3 convert_aegis_output.py > aegis_results.json
                echo -e "${GREEN}✓${NC} Results saved to aegis_results.json"
            else
                echo -e "${RED}✗${NC} Aegis not built. Run: cmake -B ../build && cmake --build ../build"
            fi
            ;;
        2)
            if [ $HAS_NODE -eq 1 ]; then
                echo ""
                echo "Starting React dev server..."
                echo "Once running, open http://localhost:3000"
                echo "Run benchmark, export results, and save to this directory as react_results.json"
                cd react
                npm install
                npm run dev
                cd ..
            else
                echo -e "${RED}✗${NC} Node.js not found. Install from https://nodejs.org/"
            fi
            ;;
        3)
            echo ""
            echo "Opening Canvas benchmark..."
            if command -v xdg-open &> /dev/null; then
                xdg-open canvas/canvas_benchmark.html
            elif command -v open &> /dev/null; then
                open canvas/canvas_benchmark.html
            else
                echo "Please open canvas/canvas_benchmark.html in Chrome/Chromium"
            fi
            echo "Run benchmark, export results, and save to this directory as canvas_results.json"
            ;;
        4)
            if [ $HAS_QT -eq 1 ]; then
                echo ""
                echo "Building Qt benchmark..."
                cd qt
                mkdir -p build
                cd build
                cmake ..
                make
                echo -e "${GREEN}✓${NC} Built successfully"
                echo "Running Qt benchmark..."
                ./qt_benchmark
                cd ../..
                echo "Run benchmark in GUI, export results, and save to this directory as qt_results.json"
            else
                echo -e "${RED}✗${NC} Qt not found. Install Qt 6 development libraries"
            fi
            ;;
        5)
            if [ $HAS_PYTHON -eq 1 ]; then
                echo ""
                echo "Analyzing results..."
                python3 analyze_results.py
            else
                echo -e "${RED}✗${NC} Python 3 not found"
            fi
            ;;
        6)
            echo ""
            read -p "Are you sure you want to delete all result files? [y/N]: " confirm
            if [[ $confirm == [yY] || $confirm == [yY][eE][sS] ]]; then
                rm -f aegis_results.json react_results.json canvas_results.json qt_results.json imgui_results.json
                echo -e "${GREEN}✓${NC} Result files deleted"
            fi
            ;;
        7)
            echo "Goodbye!"
            exit 0
            ;;
        *)
            echo -e "${RED}Invalid choice${NC}"
            ;;
    esac
    
    echo ""
    echo "═══════════════════════════════════════════════════════════════"
    echo ""
done
