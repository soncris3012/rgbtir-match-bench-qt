#include "core/BenchmarkEngine.h"

#include "core/DatasetPairer.h"
#include "core/FeatureMatcher.h"
#include "core/MetricCalculator.h"
#include "core/ReportWriter.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <opencv2/imgcodecs.hpp>
#include <algorithm>
#include <random>
#include <stdexcept>

namespace rgbtir {

BenchmarkResult BenchmarkEngine::run(
    const BenchmarkConfig &config,
    const ProgressCallback &progress,
    const std::atomic_bool *cancelled)
{
    BenchmarkResult result;
    QVector<ImagePair> pairs = DatasetPairer::pairDirectories(
        config.visibleDir, config.thermalDir, &result.warnings);

    if (config.maxPairs > 0 && pairs.size() > config.maxPairs) {
        std::mt19937 generator(config.seed);
        std::shuffle(pairs.begin(), pairs.end(), generator);
        pairs.resize(config.maxPairs);
        std::sort(pairs.begin(), pairs.end(), [](const ImagePair &a, const ImagePair &b) { return a.id < b.id; });
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    result.runDirectory = QDir(config.outputDir).filePath(timestamp);
    if (!QDir().mkpath(result.runDirectory))
        throw std::runtime_error("Cannot create run directory");

    const int total = pairs.size() * config.methods.size();
    int current = 0;
    for (const QString &method : config.methods) {
        std::unique_ptr<IFeatureMatcher> matcher = FeatureMatcherFactory::create(method, config.maxKeypoints);
        for (const ImagePair &pair : pairs) {
            if (cancelled && cancelled->load())
                throw std::runtime_error("Đã dừng theo yêu cầu.");

            PairResult row;
            row.method = method;
            row.pairId = pair.id;
            row.visiblePath = pair.visiblePath;
            row.thermalPath = pair.thermalPath;
            try {
                const cv::Mat visible = cv::imread(pair.visiblePath.toStdString(), cv::IMREAD_COLOR);
                const cv::Mat thermal = cv::imread(pair.thermalPath.toStdString(), cv::IMREAD_COLOR);
                if (visible.empty() || thermal.empty())
                    throw std::runtime_error("Không đọc được ảnh");
                QElapsedTimer timer;
                timer.start();
                const MatchResult matches = matcher->match(visible, thermal);
                row.metrics = MetricCalculator::evaluate(
                    matches, visible.size(), thermal.size(), config.correctnessPx, config.ransacPx);
                row.metrics.runtimeMs = timer.nsecsElapsed() / 1'000'000.0;
            } catch (const std::exception &error) {
                row.error = QString::fromUtf8(error.what());
            }
            result.rows.append(row);
            ++current;
            if (progress)
                progress(current, total, QStringLiteral("%1 · %2").arg(method, pair.id));
        }
    }
    result.summary = ReportWriter::summarize(result.rows);
    ReportWriter::writeAll(result.runDirectory, config, result.rows, result.summary);
    return result;
}

} // namespace rgbtir

