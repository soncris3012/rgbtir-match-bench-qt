#pragma once

#include "core/BenchmarkTypes.h"

#include <QObject>
#include <atomic>

namespace rgbtir {

class BenchmarkWorker final : public QObject {
    Q_OBJECT

public:
    explicit BenchmarkWorker(BenchmarkConfig config, QObject *parent = nullptr);

public slots:
    void run();
    void cancel();

signals:
    void progress(int current, int total, const QString &message);
    void pairStarted(
        const QString &visiblePath,
        const QString &thermalPath,
        const QString &method,
        int current,
        int total);
    void completed(const rgbtir::BenchmarkResult &result);
    void failed(const QString &message);
    void finished();

private:
    BenchmarkConfig config_;
    std::atomic_bool cancelled_{false};
};

} // namespace rgbtir

