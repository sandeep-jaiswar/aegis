#include "benchmark_window.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QApplication>
#include <QElapsedTimer>
#include <QThread>
#include <chrono>

namespace aegis::benchmark {

BenchmarkWindow::BenchmarkWindow(QWidget* parent)
    : QMainWindow(parent) {
    
    setWindowTitle("APP-001 Qt Benchmark - External Falsification");
    resize(800, 600);

    // Create central widget and layout
    central_widget_ = new QWidget(this);
    auto* layout = new QVBoxLayout(central_widget_);

    // Header
    auto* header = new QLabel("<h1>🔧 APP-001 Qt Benchmark</h1>"
                               "<p><i>External Falsification - Native Qt</i></p>");
    header->setTextFormat(Qt::RichText);
    layout->addWidget(header);

    // Warning
    auto* warning = new QLabel(
        "<div style='background-color: #ffeecc; padding: 10px; border-left: 4px solid #ff9900;'>"
        "<strong>⚠️ Note:</strong> This benchmark implements APP-001 using Qt to validate "
        "Aegis's variance claims. For accurate results, close other applications."
        "</div>"
    );
    warning->setTextFormat(Qt::RichText);
    warning->setWordWrap(true);
    layout->addWidget(warning);

    // Buttons
    auto* button_layout = new QHBoxLayout();
    start_button_ = new QPushButton("Start Benchmark");
    export_button_ = new QPushButton("Export Results");
    export_button_->setEnabled(false);
    
    connect(start_button_, &QPushButton::clicked, this, &BenchmarkWindow::run_benchmark);
    connect(export_button_, &QPushButton::clicked, this, &BenchmarkWindow::export_results);
    
    button_layout->addWidget(start_button_);
    button_layout->addWidget(export_button_);
    button_layout->addStretch();
    layout->addLayout(button_layout);

    // Status
    status_label_ = new QLabel("Click 'Start Benchmark' to begin...");
    layout->addWidget(status_label_);

    // Results display
    results_text_ = new QTextEdit();
    results_text_->setReadOnly(true);
    results_text_->setFont(QFont("Courier New", 10));
    layout->addWidget(results_text_);

    // Info
    auto* info = new QLabel(
        "<h3>About This Benchmark</h3>"
        "<p>This implementation uses <strong>Qt</strong> with native widgets to execute "
        "the APP-001 workload. Qt adds overhead through:</p>"
        "<ul>"
        "<li>Qt event loop and signal/slot mechanism</li>"
        "<li>QObject memory management</li>"
        "<li>Native widget rendering pipeline</li>"
        "<li>Platform-specific windowing system</li>"
        "</ul>"
        "<p>Compare these results with the Aegis baseline to see the impact of Qt framework "
        "overhead on variance and predictability.</p>"
    );
    info->setTextFormat(Qt::RichText);
    info->setWordWrap(true);
    layout->addWidget(info);

    setCentralWidget(central_widget_);
}

BenchmarkWindow::~BenchmarkWindow() = default;

void BenchmarkWindow::run_benchmark() {
    start_button_->setEnabled(false);
    export_button_->setEnabled(false);
    status_label_->setText("Running benchmark...");
    results_text_->clear();
    
    QApplication::processEvents();

    QtWorkload workload;
    std::vector<uint64_t> timings;
    timings.reserve(Config::measured_iterations);

    // Warmup phase
    for (size_t i = 0; i < Config::warmup_iterations; ++i) {
        workload.execute_frame();
    }

    // Wait for stable state
    QThread::msleep(100);

    // Measured phase
    for (size_t i = 0; i < Config::measured_iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        workload.execute_frame();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        timings.push_back(duration.count());

        if (i % 10 == 0) {
            status_label_->setText(
                QString("Progress: %1/%2 iterations...").arg(i).arg(Config::measured_iterations)
            );
            QApplication::processEvents();
        }
    }

    current_results_ = calculate_percentiles(timings);
    has_results_ = true;

    display_results(current_results_);
    status_label_->setText("Benchmark complete!");
    
    start_button_->setEnabled(true);
    export_button_->setEnabled(true);
}

void BenchmarkWindow::display_results(const PercentileMetrics& metrics) {
    QString text;
    QTextStream stream(&text);

    stream << "=== Qt Implementation Results ===\n\n";
    stream << QString("Min:          %1\n").arg(format_ns(metrics.min_ns));
    stream << QString("P50 (Median): %1\n").arg(format_ns(metrics.p50_ns));
    stream << QString("P90:          %1\n").arg(format_ns(metrics.p90_ns));
    stream << QString("P95:          %1\n").arg(format_ns(metrics.p95_ns));
    stream << QString("P99:          %1\n").arg(format_ns(metrics.p99_ns));
    stream << QString("P99.9:        %1\n").arg(format_ns(metrics.p99_9_ns));
    stream << QString("Max:          %1\n").arg(format_ns(metrics.max_ns));
    stream << QString("Mean:         %1\n").arg(format_ns(metrics.mean_ns));
    stream << QString("Variance:     %1 ns²\n").arg(static_cast<uint64_t>(metrics.variance));

    results_text_->setPlainText(text);
}

QString BenchmarkWindow::format_ns(uint64_t ns) const {
    return QString("%1 ns").arg(ns);
}

void BenchmarkWindow::export_results() {
    if (!has_results_) {
        return;
    }

    QString filename = QFileDialog::getSaveFileName(
        this,
        "Export Results",
        QString("qt-benchmark-%1.json").arg(QDateTime::currentMSecsSinceEpoch()),
        "JSON Files (*.json)"
    );

    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to open file for writing");
        return;
    }

    QTextStream out(&file);
    out << "{\n";
    out << "  \"technology\": \"Qt\",\n";
    out << "  \"timestamp\": \"" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\",\n";
    out << "  \"config\": {\n";
    out << "    \"warmupIterations\": " << Config::warmup_iterations << ",\n";
    out << "    \"measuredIterations\": " << Config::measured_iterations << ",\n";
    out << "    \"allocationsPerFrame\": " << Config::allocations_per_frame << ",\n";
    out << "    \"operationsPerFrame\": " << Config::operations_per_frame << "\n";
    out << "  },\n";
    out << "  \"results\": {\n";
    out << "    \"min_ns\": " << current_results_.min_ns << ",\n";
    out << "    \"p50_ns\": " << current_results_.p50_ns << ",\n";
    out << "    \"p90_ns\": " << current_results_.p90_ns << ",\n";
    out << "    \"p95_ns\": " << current_results_.p95_ns << ",\n";
    out << "    \"p99_ns\": " << current_results_.p99_ns << ",\n";
    out << "    \"p99_9_ns\": " << current_results_.p99_9_ns << ",\n";
    out << "    \"max_ns\": " << current_results_.max_ns << ",\n";
    out << "    \"mean_ns\": " << current_results_.mean_ns << ",\n";
    out << "    \"variance\": " << static_cast<uint64_t>(current_results_.variance) << "\n";
    out << "  }\n";
    out << "}\n";

    file.close();
    QMessageBox::information(this, "Success", "Results exported successfully!");
}

} // namespace aegis::benchmark

#include "benchmark_window.moc"
