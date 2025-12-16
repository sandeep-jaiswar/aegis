#include "benchmark_window.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    aegis::benchmark::BenchmarkWindow window;
    window.show();

    return app.exec();
}
