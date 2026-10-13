#include "worker/BenchmarkWorker.h"

#include "core/BenchmarkEngine.h"

namespace rgbtir {

BenchmarkWorker::BenchmarkWorker(BenchmarkConfig config, QObject *parent)
    : QObject(parent), config_(std::move(config))
{
}

void BenchmarkWorker::run()
{
    try {
        const BenchmarkResult result = BenchmarkEngine::run(
            config_,
            [this](int current, int total, const QString &message) {
                emit progress(current, total, message);
            },
            &cancelled_,
            [this](const ImagePair &pair, const QString &method, int current, int total) {
                emit pairStarted(
                    pair.visiblePath, pair.thermalPath, method, current, total);
            });
        emit completed(result);
    } catch (const std::exception &error) {
        emit failed(QString::fromUtf8(error.what()));
    }
    emit finished();
}

void BenchmarkWorker::cancel()
{
    cancelled_.store(true);
}

} // namespace rgbtir

