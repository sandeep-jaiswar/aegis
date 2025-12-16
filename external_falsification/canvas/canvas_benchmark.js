// Benchmark configuration (matching APP-001 spec)
const CONFIG = {
    warmupIterations: 10,
    measuredIterations: 100,
    allocationsPerFrame: 50,
    allocationSizes: [64, 128, 256, 512, 1024],
    operationsPerFrame: 100
};

// Workload class with Canvas rendering
class CanvasWorkload {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.allocations = [];
        this.operations = [];
    }

    reset() {
        this.allocations = [];
        this.operations = [];
        // Clear canvas
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    }

    performAllocations() {
        for (let i = 0; i < CONFIG.allocationsPerFrame; i++) {
            const sizeIdx = i % CONFIG.allocationSizes.length;
            const size = CONFIG.allocationSizes[sizeIdx];
            const buffer = new ArrayBuffer(size);
            this.allocations.push(buffer);
            
            // Render allocation visually to exercise Canvas API
            const x = (i % 10) * 80;
            const y = Math.floor(i / 10) * 80;
            const rectSize = Math.sqrt(size) / 2;
            
            this.ctx.fillStyle = `hsl(${(i * 7) % 360}, 70%, 50%)`;
            this.ctx.fillRect(x, y, rectSize, rectSize);
        }
    }

    performOperations() {
        for (let i = 0; i < CONFIG.operationsPerFrame; i++) {
            const result = i * i + i;
            this.operations.push(result);
            
            // Draw operation result as a line
            if (i > 0) {
                const prevResult = this.operations[i - 1];
                const x1 = (i - 1) * (this.canvas.width / CONFIG.operationsPerFrame);
                const y1 = this.canvas.height - (prevResult % this.canvas.height);
                const x2 = i * (this.canvas.width / CONFIG.operationsPerFrame);
                const y2 = this.canvas.height - (result % this.canvas.height);
                
                this.ctx.strokeStyle = '#569cd6';
                this.ctx.lineWidth = 1;
                this.ctx.beginPath();
                this.ctx.moveTo(x1, y1);
                this.ctx.lineTo(x2, y2);
                this.ctx.stroke();
            }
        }
    }

    executeFrame() {
        this.reset();
        this.performAllocations();
        this.performOperations();
    }
}

// Calculate percentiles from sorted data
function calculatePercentiles(data) {
    const sorted = [...data].sort((a, b) => a - b);
    const n = sorted.length;
    
    const getPercentile = (p) => {
        const idx = Math.floor((n * p) / 100);
        return sorted[Math.min(idx, n - 1)];
    };

    const sum = sorted.reduce((a, b) => a + b, 0);
    const mean = sum / n;
    
    const variance = sorted.reduce((sum, val) => {
        return sum + Math.pow(val - mean, 2);
    }, 0) / n;

    return {
        min_ns: sorted[0],
        p50_ns: getPercentile(50),
        p90_ns: getPercentile(90),
        p95_ns: getPercentile(95),
        p99_ns: getPercentile(99),
        p99_9_ns: getPercentile(99.9),
        max_ns: sorted[n - 1],
        mean_ns: mean,
        variance: variance
    };
}

function formatNs(ns) {
    return `${Math.round(ns).toLocaleString()} ns`;
}

let benchmarkResults = null;

async function runBenchmark() {
    const startBtn = document.getElementById('startBtn');
    const exportBtn = document.getElementById('exportBtn');
    const resultsDiv = document.getElementById('resultsDiv');
    
    startBtn.disabled = true;
    exportBtn.disabled = true;
    resultsDiv.innerHTML = '<div class="status">Running benchmark...</div>';

    const workloadCanvas = document.getElementById('workloadCanvas');
    const workload = new CanvasWorkload(workloadCanvas);
    const timings = [];

    // Warmup phase
    for (let i = 0; i < CONFIG.warmupIterations; i++) {
        workload.executeFrame();
    }

    // Wait for stable state
    await new Promise(resolve => setTimeout(resolve, 100));

    // Measured phase
    // Note: performance.now() has microsecond precision in most browsers,
    // so nanosecond values are approximations for comparison purposes
    for (let i = 0; i < CONFIG.measuredIterations; i++) {
        const start = performance.now() * 1000000; // Convert to nanoseconds (microsecond precision)
        workload.executeFrame();
        const end = performance.now() * 1000000;
        timings.push(end - start);

        if (i % 10 === 0) {
            resultsDiv.innerHTML = 
                `<div class="status">Progress: ${i}/${CONFIG.measuredIterations} iterations...</div>`;
            await new Promise(resolve => setTimeout(resolve, 0));
        }
    }

    benchmarkResults = calculatePercentiles(timings);
    displayResults();
    visualizeResults(timings);
    
    startBtn.disabled = false;
    exportBtn.disabled = false;
}

function displayResults() {
    const resultsDiv = document.getElementById('resultsDiv');
    const r = benchmarkResults;
    
    resultsDiv.innerHTML = `
        <div class="metric">
            <span class="metric-name">Min:</span>
            <span class="metric-value">${formatNs(r.min_ns)}</span>
        </div>
        <div class="metric">
            <span class="metric-name">P50 (Median):</span>
            <span class="metric-value">${formatNs(r.p50_ns)}</span>
        </div>
        <div class="metric">
            <span class="metric-name">P90:</span>
            <span class="metric-value">${formatNs(r.p90_ns)}</span>
        </div>
        <div class="metric">
            <span class="metric-name">P95:</span>
            <span class="metric-value">${formatNs(r.p95_ns)}</span>
        </div>
        <div class="metric">
            <span class="metric-name">P99:</span>
            <span class="metric-value">${formatNs(r.p99_ns)}</span>
        </div>
        <div class="metric">
            <span class="metric-name">P99.9:</span>
            <span class="metric-value">${formatNs(r.p99_9_ns)}</span>
        </div>
        <div class="metric">
            <span class="metric-name">Max:</span>
            <span class="metric-value">${formatNs(r.max_ns)}</span>
        </div>
        <div class="metric">
            <span class="metric-name">Mean:</span>
            <span class="metric-value">${formatNs(r.mean_ns)}</span>
        </div>
        <div class="metric">
            <span class="metric-name">Variance:</span>
            <span class="metric-value">${formatNs(r.variance)}</span>
        </div>
    `;
}

function visualizeResults(timings) {
    const canvas = document.getElementById('visualizationCanvas');
    const ctx = canvas.getContext('2d');
    const width = canvas.width;
    const height = canvas.height;

    ctx.fillStyle = '#1e1e1e';
    ctx.fillRect(0, 0, width, height);

    if (timings.length === 0) return;

    const min = Math.min(...timings);
    const max = Math.max(...timings);
    const range = max - min;

    // Draw timing graph
    ctx.strokeStyle = '#569cd6';
    ctx.lineWidth = 2;
    ctx.beginPath();

    timings.forEach((time, i) => {
        const x = (i / timings.length) * width;
        const y = height - ((time - min) / range) * (height - 40) - 20;
        
        if (i === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    });

    ctx.stroke();

    // Draw mean line
    const mean = timings.reduce((a, b) => a + b, 0) / timings.length;
    const meanY = height - ((mean - min) / range) * (height - 40) - 20;
    
    ctx.strokeStyle = '#4ec9b0';
    ctx.lineWidth = 1;
    ctx.setLineDash([5, 5]);
    ctx.beginPath();
    ctx.moveTo(0, meanY);
    ctx.lineTo(width, meanY);
    ctx.stroke();
    ctx.setLineDash([]);

    // Labels
    ctx.fillStyle = '#d4d4d4';
    ctx.font = '12px Consolas';
    ctx.fillText(`Max: ${formatNs(max)}`, 10, 20);
    ctx.fillText(`Min: ${formatNs(min)}`, 10, height - 10);
    ctx.fillText(`Mean: ${formatNs(mean)}`, width - 200, meanY - 5);
}

function exportResults() {
    if (!benchmarkResults) return;

    const data = {
        technology: 'Canvas',
        timestamp: new Date().toISOString(),
        config: CONFIG,
        results: benchmarkResults
    };

    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `canvas-benchmark-${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
}
