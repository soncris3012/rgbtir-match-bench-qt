#pragma once

#include "core/BenchmarkTypes.h"

namespace rgbtir {

class MetricCalculator {
public:
    static PairMetrics evaluate(
        const MatchResult &matches,
        const cv::Size &visibleSize,
        const cv::Size &thermalSize,
        double correctnessPx,
        double ransacPx);
};

} // namespace rgbtir

