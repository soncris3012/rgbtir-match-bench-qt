#pragma once

#include "core/BenchmarkTypes.h"

namespace rgbtir {

class ReportWriter {
public:
    static QVector<SummaryRow> summarize(const QVector<PairResult> &rows);
    static void writeAll(
        const QString &runDirectory,
        const BenchmarkConfig &config,
        const QVector<PairResult> &rows,
        const QVector<SummaryRow> &summary);
};

} // namespace rgbtir

