#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>
#include <opencv2/core.hpp>

namespace rgbtir {

struct ImagePair {
    QString id;
    QString visiblePath;
    QString thermalPath;
};

struct BenchmarkConfig {
    QString visibleDir;
    QString thermalDir;
    QString datasetName = QStringLiteral("Auto");
    QStringList methods;
    QString outputDir;
    int maxPairs = 100;
    int maxKeypoints = 2048;
    double correctnessPx = 5.0;
    double ransacPx = 3.0;
    quint32 seed = 42;
};

struct MatchResult {
    std::vector<cv::Point2f> points0;
    std::vector<cv::Point2f> points1;
    std::vector<float> confidence;
};

struct PairMetrics {
    int matches = 0;
    int correctMatches = 0;
    int homographyInliers = 0;
    double precision = 0.0;
    double meanError = 0.0;
    double medianError = 0.0;
    double p95Error = 0.0;
    double mma1 = 0.0;
    double mma3 = 0.0;
    double mma5 = 0.0;
    double mma10 = 0.0;
    double inlierRatio = 0.0;
    double homographyRmse = 0.0;
    double coverage = 0.0;
    double runtimeMs = 0.0;
};

struct PairResult {
    QString method;
    QString pairId;
    QString visiblePath;
    QString thermalPath;
    QString error;
    PairMetrics metrics;
};

struct SummaryRow {
    QString method;
    int pairs = 0;
    int failures = 0;
    PairMetrics mean;
};

struct BenchmarkResult {
    QString runDirectory;
    QVector<PairResult> rows;
    QVector<SummaryRow> summary;
    QStringList warnings;
};

} // namespace rgbtir

Q_DECLARE_METATYPE(rgbtir::BenchmarkConfig)
Q_DECLARE_METATYPE(rgbtir::BenchmarkResult)

