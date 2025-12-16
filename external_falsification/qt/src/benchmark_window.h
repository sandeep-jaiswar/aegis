#pragma once

#include "workload.h"

#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <vector>

namespace aegis::benchmark {

class BenchmarkWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit BenchmarkWindow(QWidget* parent = nullptr);
    ~BenchmarkWindow() override;

  private slots:
    void run_benchmark();
    void export_results();

  private:
    void display_results(const PercentileMetrics& metrics);
    QString format_ns(uint64_t ns) const;

    QWidget* central_widget_;
    QTextEdit* results_text_;
    QPushButton* start_button_;
    QPushButton* export_button_;
    QLabel* status_label_;

    PercentileMetrics current_results_{};
    bool has_results_ = false;
};

} // namespace aegis::benchmark
