#include "core/ReportWriter.h"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <algorithm>
#include <stdexcept>

namespace rgbtir {
namespace {

QString csvCell(QString value)
{
    value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QStringLiteral("\"") + value + QStringLiteral("\"");
}

QString number(double value, int precision = 6)
{
    return QString::number(value, 'f', precision);
}

QJsonObject metricsJson(const PairMetrics &m)
{
    return {
        {QStringLiteral("matches"), m.matches},
        {QStringLiteral("correct_matches"), m.correctMatches},
        {QStringLiteral("precision_px"), m.precision},
        {QStringLiteral("mean_error_px"), m.meanError},
        {QStringLiteral("median_error_px"), m.medianError},
        {QStringLiteral("p95_error_px"), m.p95Error},
        {QStringLiteral("mma_1px"), m.mma1},
        {QStringLiteral("mma_3px"), m.mma3},
        {QStringLiteral("mma_5px"), m.mma5},
        {QStringLiteral("mma_10px"), m.mma10},
        {QStringLiteral("inlier_ratio"), m.inlierRatio},
        {QStringLiteral("homography_rmse_px"), m.homographyRmse},
        {QStringLiteral("coverage_4x4"), m.coverage},
        {QStringLiteral("runtime_ms"), m.runtimeMs},
    };
}

void openForWrite(QFile &file)
{
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        throw std::runtime_error(file.errorString().toStdString());
}

} // namespace

QVector<SummaryRow> ReportWriter::summarize(const QVector<PairResult> &rows)
{
    QHash<QString, QVector<const PairResult *>> groups;
    for (const PairResult &row : rows)
        groups[row.method].append(&row);

    QVector<SummaryRow> output;
    output.reserve(groups.size());
    for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
        SummaryRow summary;
        summary.method = it.key();
        summary.pairs = it.value().size();
        for (const PairResult *row : it.value()) {
            if (!row->error.isEmpty()) {
                ++summary.failures;
                continue;
            }
            const PairMetrics &m = row->metrics;
            summary.mean.matches += m.matches;
            summary.mean.correctMatches += m.correctMatches;
            summary.mean.precision += m.precision;
            summary.mean.meanError += m.meanError;
            summary.mean.medianError += m.medianError;
            summary.mean.p95Error += m.p95Error;
            summary.mean.mma1 += m.mma1;
            summary.mean.mma3 += m.mma3;
            summary.mean.mma5 += m.mma5;
            summary.mean.mma10 += m.mma10;
            summary.mean.inlierRatio += m.inlierRatio;
            summary.mean.homographyRmse += m.homographyRmse;
            summary.mean.coverage += m.coverage;
            summary.mean.runtimeMs += m.runtimeMs;
        }
        const int successful = std::max(1, summary.pairs - summary.failures);
        summary.mean.matches /= successful;
        summary.mean.correctMatches /= successful;
        summary.mean.precision /= successful;
        summary.mean.meanError /= successful;
        summary.mean.medianError /= successful;
        summary.mean.p95Error /= successful;
        summary.mean.mma1 /= successful;
        summary.mean.mma3 /= successful;
        summary.mean.mma5 /= successful;
        summary.mean.mma10 /= successful;
        summary.mean.inlierRatio /= successful;
        summary.mean.homographyRmse /= successful;
        summary.mean.coverage /= successful;
        summary.mean.runtimeMs /= successful;
        output.append(summary);
    }
    std::sort(output.begin(), output.end(), [](const SummaryRow &a, const SummaryRow &b) {
        return a.method < b.method;
    });
    return output;
}

void ReportWriter::writeAll(
    const QString &runDirectory,
    const BenchmarkConfig &config,
    const QVector<PairResult> &rows,
    const QVector<SummaryRow> &summary)
{
    QFile pairFile(runDirectory + QStringLiteral("/per_pair.csv"));
    openForWrite(pairFile);
    QTextStream pairOut(&pairFile);
    pairOut << "method,pair_id,visible,thermal,error,matches,correct_matches,precision_px,mean_error_px,median_error_px,p95_error_px,mma_1px,mma_3px,mma_5px,mma_10px,inlier_ratio,homography_rmse_px,coverage_4x4,runtime_ms\n";
    for (const PairResult &row : rows) {
        const PairMetrics &m = row.metrics;
        pairOut << csvCell(row.method) << ',' << csvCell(row.pairId) << ','
                << csvCell(row.visiblePath) << ',' << csvCell(row.thermalPath) << ','
                << csvCell(row.error) << ',' << m.matches << ',' << m.correctMatches << ','
                << number(m.precision) << ',' << number(m.meanError) << ',' << number(m.medianError) << ','
                << number(m.p95Error) << ',' << number(m.mma1) << ',' << number(m.mma3) << ','
                << number(m.mma5) << ',' << number(m.mma10) << ',' << number(m.inlierRatio) << ','
                << number(m.homographyRmse) << ',' << number(m.coverage) << ',' << number(m.runtimeMs) << '\n';
    }

    QFile summaryFile(runDirectory + QStringLiteral("/summary.csv"));
    openForWrite(summaryFile);
    QTextStream summaryOut(&summaryFile);
    summaryOut << "method,pairs,failures,matches,precision_px,mma_3px,inlier_ratio,median_error_px,coverage_4x4,runtime_ms\n";
    for (const SummaryRow &row : summary) {
        summaryOut << csvCell(row.method) << ',' << row.pairs << ',' << row.failures << ','
                   << row.mean.matches << ',' << number(row.mean.precision) << ',' << number(row.mean.mma3) << ','
                   << number(row.mean.inlierRatio) << ',' << number(row.mean.medianError) << ','
                   << number(row.mean.coverage) << ',' << number(row.mean.runtimeMs) << '\n';
    }

    QJsonObject configJson {
        {QStringLiteral("visible_dir"), config.visibleDir},
        {QStringLiteral("thermal_dir"), config.thermalDir},
        {QStringLiteral("dataset"), config.datasetName},
        {QStringLiteral("methods"), QJsonArray::fromStringList(config.methods)},
        {QStringLiteral("max_pairs"), config.maxPairs},
        {QStringLiteral("correctness_px"), config.correctnessPx},
        {QStringLiteral("ransac_px"), config.ransacPx},
        {QStringLiteral("max_keypoints"), config.maxKeypoints},
    };
    QJsonArray pairArray;
    for (const PairResult &row : rows) {
        QJsonObject item = metricsJson(row.metrics);
        item.insert(QStringLiteral("method"), row.method);
        item.insert(QStringLiteral("pair_id"), row.pairId);
        item.insert(QStringLiteral("visible"), row.visiblePath);
        item.insert(QStringLiteral("thermal"), row.thermalPath);
        item.insert(QStringLiteral("error"), row.error);
        pairArray.append(item);
    }
    QJsonArray summaryArray;
    for (const SummaryRow &row : summary) {
        QJsonObject item = metricsJson(row.mean);
        item.insert(QStringLiteral("method"), row.method);
        item.insert(QStringLiteral("pairs"), row.pairs);
        item.insert(QStringLiteral("failures"), row.failures);
        summaryArray.append(item);
    }
    QFile jsonFile(runDirectory + QStringLiteral("/results.json"));
    openForWrite(jsonFile);
    jsonFile.write(QJsonDocument(QJsonObject {
        {QStringLiteral("config"), configJson},
        {QStringLiteral("summary"), summaryArray},
        {QStringLiteral("pairs"), pairArray},
    }).toJson(QJsonDocument::Indented));

    QFile htmlFile(runDirectory + QStringLiteral("/report.html"));
    openForWrite(htmlFile);
    QTextStream html(&htmlFile);
    html << "<!doctype html><html lang='vi'><meta charset='utf-8'><title>RGB–TIR Match Bench</title>"
            "<style>body{font:15px system-ui;max-width:1100px;margin:40px auto;padding:0 20px;color:#111}"
            "h1{font-size:28px}table{border-collapse:collapse;width:100%}th,td{border-bottom:1px solid #ddd;padding:10px;text-align:right}"
            "th:first-child,td:first-child{text-align:left}</style><h1>RGB–TIR Match Bench · C++/Qt</h1>"
         << "<p>Ngưỡng đúng: " << number(config.correctnessPx, 1) << " px. Dữ liệu: "
         << config.datasetName.toHtmlEscaped() << ".</p><table><tr><th>Method</th><th>Pairs</th><th>Precision</th>"
            "<th>MMA@3</th><th>Inlier ratio</th><th>Median px</th><th>Coverage</th><th>ms/pair</th></tr>";
    for (const SummaryRow &row : summary) {
        html << "<tr><td>" << row.method.toHtmlEscaped() << "</td><td>" << row.pairs << "</td><td>"
             << number(row.mean.precision * 100.0, 2) << "%</td><td>" << number(row.mean.mma3 * 100.0, 2)
             << "%</td><td>" << number(row.mean.inlierRatio * 100.0, 2) << "%</td><td>"
             << number(row.mean.medianError, 2) << "</td><td>" << number(row.mean.coverage * 100.0, 2)
             << "%</td><td>" << number(row.mean.runtimeMs, 2) << "</td></tr>";
    }
    html << "</table><p><small>Metric 2D trên cặp ảnh đã căn chỉnh; không phải RMSE 3D của bài báo.</small></p></html>";
}

} // namespace rgbtir

