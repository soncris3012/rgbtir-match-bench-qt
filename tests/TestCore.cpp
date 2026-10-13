#include "core/DatasetPairer.h"
#include "core/BenchmarkEngine.h"
#include "core/FeatureMatcher.h"
#include "core/MetricCalculator.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

class TestCore final : public QObject {
    Q_OBJECT

private slots:
    void pairsLlvipLayout();
    void fallsBackToSortedOrder();
    void identityMatchesAreCorrect();
    void normalizesDifferentResolutions();
    void runsSiftAndWritesReports();
};

namespace {

void writeImage(const QString &path)
{
    QDir().mkpath(QFileInfo(path).path());
    const cv::Mat image(20, 30, CV_8UC3, cv::Scalar(0, 0, 0));
    QVERIFY(cv::imwrite(path.toStdString(), image));
}

rgbtir::MatchResult points(const std::vector<cv::Point2f> &a, const std::vector<cv::Point2f> &b)
{
    rgbtir::MatchResult result;
    result.points0 = a;
    result.points1 = b;
    result.confidence.assign(a.size(), 1.0f);
    return result;
}

} // namespace

void TestCore::pairsLlvipLayout()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    writeImage(temporary.filePath(QStringLiteral("visible/test/190001.jpg")));
    writeImage(temporary.filePath(QStringLiteral("infrared/test/190001.jpg")));

    QString visible;
    QString thermal;
    QString name;
    QVERIFY(rgbtir::DatasetPairer::detectDatasetRoot(
        temporary.path(), &visible, &thermal, &name));
    QCOMPARE(name, QStringLiteral("LLVIP"));
    QStringList warnings;
    const auto pairs = rgbtir::DatasetPairer::pairDirectories(visible, thermal, &warnings);
    QCOMPARE(pairs.size(), 1);
    QVERIFY(warnings.isEmpty());
}

void TestCore::fallsBackToSortedOrder()
{
    QTemporaryDir temporary;
    writeImage(temporary.filePath(QStringLiteral("v/a.jpg")));
    writeImage(temporary.filePath(QStringLiteral("t/x.jpg")));
    QStringList warnings;
    const auto pairs = rgbtir::DatasetPairer::pairDirectories(
        temporary.filePath(QStringLiteral("v")), temporary.filePath(QStringLiteral("t")), &warnings);
    QCOMPARE(pairs.size(), 1);
    QVERIFY(!warnings.isEmpty());
}

void TestCore::identityMatchesAreCorrect()
{
    const std::vector<cv::Point2f> p = {{10, 10}, {80, 10}, {80, 80}, {10, 80}, {50, 50}};
    const auto metrics = rgbtir::MetricCalculator::evaluate(
        points(p, p), cv::Size(100, 100), cv::Size(100, 100), 1.0, 3.0);
    QCOMPARE(metrics.precision, 1.0);
    QCOMPARE(metrics.mma1, 1.0);
    QCOMPARE(metrics.homographyInliers, 5);
    QVERIFY(metrics.homographyRmse < 1e-4);
}

void TestCore::normalizesDifferentResolutions()
{
    const std::vector<cv::Point2f> p0 = {{10, 10}, {50, 10}, {50, 50}, {10, 50}};
    const std::vector<cv::Point2f> p1 = {{20, 20}, {100, 20}, {100, 100}, {20, 100}};
    const auto metrics = rgbtir::MetricCalculator::evaluate(
        points(p0, p1), cv::Size(100, 100), cv::Size(200, 200), 0.1, 1.0);
    QCOMPARE(metrics.precision, 1.0);
}

void TestCore::runsSiftAndWritesReports()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString visiblePath = temporary.filePath(QStringLiteral("visible/0001.png"));
    const QString thermalPath = temporary.filePath(QStringLiteral("infrared/0001.png"));
    QDir().mkpath(QFileInfo(visiblePath).path());
    QDir().mkpath(QFileInfo(thermalPath).path());

    cv::Mat visible(240, 320, CV_8UC3, cv::Scalar(10, 10, 10));
    for (int y = 20; y < 220; y += 30) {
        for (int x = 20; x < 300; x += 35)
            cv::circle(visible, cv::Point(x, y), 7, cv::Scalar(220, 220, 220), -1);
    }
    cv::Mat thermal;
    cv::cvtColor(visible, thermal, cv::COLOR_BGR2GRAY);
    cv::cvtColor(thermal, thermal, cv::COLOR_GRAY2BGR);
    QVERIFY(cv::imwrite(visiblePath.toStdString(), visible));
    QVERIFY(cv::imwrite(thermalPath.toStdString(), thermal));

    rgbtir::BenchmarkConfig config;
    config.visibleDir = QFileInfo(visiblePath).path();
    config.thermalDir = QFileInfo(thermalPath).path();
    config.outputDir = temporary.filePath(QStringLiteral("runs"));
    config.methods = rgbtir::FeatureMatcherFactory::availableMethods();
    config.maxPairs = 1;
    const rgbtir::BenchmarkResult result = rgbtir::BenchmarkEngine::run(config);
    QCOMPARE(result.summary.size(), 4);
    QVERIFY(QFileInfo::exists(result.runDirectory + QStringLiteral("/summary.csv")));
    QVERIFY(QFileInfo::exists(result.runDirectory + QStringLiteral("/report.html")));
}

QTEST_APPLESS_MAIN(TestCore)
#include "TestCore.moc"

