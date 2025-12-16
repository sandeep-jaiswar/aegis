import React, { useState, useCallback, useRef } from 'react';
import './App.css';

// Benchmark configuration (matching APP-001 spec)
const CONFIG = {
  warmupIterations: 10,
  measuredIterations: 100,
  allocationsPerFrame: 50,
  allocationSizes: [64, 128, 256, 512, 1024],
  operationsPerFrame: 100
};

// Workload class matching C++ implementation
class ReactWorkload {
  constructor() {
    this.allocations = [];
    this.operations = [];
  }

  reset() {
    this.allocations = [];
    this.operations = [];
  }

  performAllocations() {
    for (let i = 0; i < CONFIG.allocationsPerFrame; i++) {
      const sizeIdx = i % CONFIG.allocationSizes.length;
      const size = CONFIG.allocationSizes[sizeIdx];
      const buffer = new ArrayBuffer(size);
      this.allocations.push(buffer);
    }
  }

  performOperations() {
    for (let i = 0; i < CONFIG.operationsPerFrame; i++) {
      const result = i * i + i;
      this.operations.push(result);
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

function App() {
  const [running, setRunning] = useState(false);
  const [results, setResults] = useState(null);
  const [progress, setProgress] = useState(0);
  const canvasRef = useRef(null);

  const runBenchmark = useCallback(async () => {
    setRunning(true);
    setProgress(0);
    setResults(null);

    const workload = new ReactWorkload();
    const timings = [];

    // Warmup phase
    for (let i = 0; i < CONFIG.warmupIterations; i++) {
      workload.executeFrame();
    }

    // Wait for stable state
    await new Promise(resolve => setTimeout(resolve, 100));

    // Measured phase
    for (let i = 0; i < CONFIG.measuredIterations; i++) {
      const start = performance.now() * 1000000; // Convert to nanoseconds
      workload.executeFrame();
      const end = performance.now() * 1000000;
      timings.push(end - start);

      if (i % 10 === 0) {
        setProgress(Math.floor((i / CONFIG.measuredIterations) * 100));
        await new Promise(resolve => setTimeout(resolve, 0));
      }
    }

    const benchResults = calculatePercentiles(timings);
    setResults(benchResults);
    setProgress(100);
    setRunning(false);

    // Visualize results
    visualizeResults(timings);
  }, []);

  const visualizeResults = (timings) => {
    const canvas = canvasRef.current;
    if (!canvas) return;

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
  };

  const exportResults = () => {
    if (!results) return;

    const data = {
      technology: 'React',
      timestamp: new Date().toISOString(),
      config: CONFIG,
      results: results
    };

    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `react-benchmark-${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="app">
      <div className="header">
        <h1>⚛️ APP-001 React Benchmark</h1>
        <div className="subtitle">External Falsification - Chromium + React</div>
      </div>

      <div className="warning">
        <strong>⚠️ Note:</strong> This benchmark implements APP-001 using React to validate 
        Aegis's variance claims. For accurate results, close other tabs and applications.
      </div>

      <div className="controls">
        <button onClick={runBenchmark} disabled={running}>
          {running ? `Running... ${progress}%` : 'Start Benchmark'}
        </button>
        <button onClick={exportResults} disabled={!results || running}>
          Export Results
        </button>
      </div>

      <div className="results">
        <h2>React Implementation Results</h2>
        {!results && !running && (
          <div className="status">Click "Start Benchmark" to begin...</div>
        )}
        {running && (
          <div className="status">Running benchmark... {progress}%</div>
        )}
        {results && (
          <div className="metrics">
            <div className="metric">
              <span className="metric-name">Min:</span>
              <span className="metric-value">{formatNs(results.min_ns)}</span>
            </div>
            <div className="metric">
              <span className="metric-name">P50 (Median):</span>
              <span className="metric-value">{formatNs(results.p50_ns)}</span>
            </div>
            <div className="metric">
              <span className="metric-name">P90:</span>
              <span className="metric-value">{formatNs(results.p90_ns)}</span>
            </div>
            <div className="metric">
              <span className="metric-name">P95:</span>
              <span className="metric-value">{formatNs(results.p95_ns)}</span>
            </div>
            <div className="metric">
              <span className="metric-name">P99:</span>
              <span className="metric-value">{formatNs(results.p99_ns)}</span>
            </div>
            <div className="metric">
              <span className="metric-name">P99.9:</span>
              <span className="metric-value">{formatNs(results.p99_9_ns)}</span>
            </div>
            <div className="metric">
              <span className="metric-name">Max:</span>
              <span className="metric-value">{formatNs(results.max_ns)}</span>
            </div>
            <div className="metric">
              <span className="metric-name">Mean:</span>
              <span className="metric-value">{formatNs(results.mean_ns)}</span>
            </div>
            <div className="metric">
              <span className="metric-name">Variance:</span>
              <span className="metric-value">{formatNs(results.variance)}</span>
            </div>
          </div>
        )}
      </div>

      <div className="workload-area">
        <h2>Frame Time Visualization</h2>
        <canvas ref={canvasRef} width="800" height="400"></canvas>
      </div>

      <div className="info">
        <h2>About This Benchmark</h2>
        <p>
          This implementation uses <strong>React</strong> with its virtual DOM to execute 
          the APP-001 workload. React adds overhead through:
        </p>
        <ul>
          <li>Virtual DOM reconciliation</li>
          <li>Component lifecycle management</li>
          <li>React's internal scheduling (Fiber)</li>
          <li>Garbage collection from object allocation</li>
        </ul>
        <p>
          Compare these results with the Aegis baseline to see the impact of framework 
          overhead on variance and predictability.
        </p>
      </div>
    </div>
  );
}

export default App;
