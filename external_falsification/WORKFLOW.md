# GitHub Actions Workflow - External Falsification

## Overview

The `external-falsification.yml` workflow automatically builds and tests all benchmark implementations on every push to validate that the external falsification framework is working correctly.

## Workflow Jobs

### 1. Aegis Baseline Benchmark (`aegis-benchmark`)
- Builds Aegis with CMake in Release mode
- Runs the browser comparison demo
- Converts output to JSON format
- Uploads results as artifact

**Output**: `aegis_results.json` with Aegis baseline metrics

### 2. React Benchmark (`react-benchmark`)
- Sets up Node.js 20
- Installs dependencies with npm
- Builds React app with Vite
- Validates build output

**Validates**: React app builds successfully and is ready to run in browser

### 3. Canvas Benchmark (`canvas-benchmark`)
- Validates HTML file structure
- Checks that all required files exist

**Validates**: Canvas benchmark is a valid standalone HTML file

### 4. Qt Benchmark (`qt-benchmark`)
- Installs Qt 6 development libraries
- Builds Qt application with CMake
- Validates binary creation

**Validates**: Qt benchmark compiles successfully

### 5. Analysis Tool (`analysis-tool`)
- Downloads Aegis results from previous job
- Creates sample React, Canvas, and Qt results
- Runs analysis tool to generate comparison
- Tests result conversion script

**Validates**: Analysis tool works correctly with all result formats

### 6. Summary (`summary`)
- Aggregates all job results
- Downloads Aegis benchmark results
- Generates GitHub Actions summary
- Displays next steps for manual testing

**Output**: Comprehensive summary in GitHub Actions UI

## Triggering the Workflow

The workflow runs automatically on:
- Push to `copilot/reimplement-app-001` branch
- Pull requests modifying `external_falsification/**`
- Manual trigger via workflow_dispatch

## Viewing Results

1. Go to the Actions tab in GitHub
2. Click on the latest "External Falsification Benchmarks" run
3. View the summary for Aegis baseline results
4. Check individual job logs for build details

## Local Testing

To test the workflow steps locally:

```bash
# Job 1: Aegis
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target browser_comparison_demo
./build/core/browser_comparison_demo | python3 external_falsification/convert_aegis_output.py > external_falsification/aegis_results.json

# Job 2: React
cd external_falsification/react
npm install
npm run build

# Job 3: Canvas
# Just verify the file exists
ls external_falsification/canvas/canvas_benchmark.html

# Job 4: Qt
cd external_falsification/qt
mkdir -p build && cd build
cmake ..
make

# Job 5: Analysis
cd external_falsification
python3 analyze_results.py
```

## Artifacts

The workflow uploads:
- `aegis-results`: JSON file with Aegis baseline benchmark results

Download artifacts from the workflow run summary page.

## Next Steps After Workflow

After the workflow succeeds:
1. Download `aegis-results` artifact
2. Run React benchmark in browser and export results
3. Run Canvas benchmark in browser and export results
4. Run Qt benchmark GUI and export results
5. Place all JSON files in `external_falsification/`
6. Run `python3 analyze_results.py` for full comparison

## Troubleshooting

**Build failures**:
- Check CMake configuration matches your local environment
- Ensure all dependencies are installed

**React build fails**:
- Verify package.json is committed
- Check Node.js version (requires 18+)

**Qt build fails**:
- Ensure Qt 6 is available in CI environment
- Check CMakeLists.txt for Qt dependencies

**Analysis tool fails**:
- Verify JSON format matches expected schema
- Check Python version (requires 3.8+)
