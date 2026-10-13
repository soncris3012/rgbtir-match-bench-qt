#pragma once

#include "core/BenchmarkTypes.h"

#include <atomic>
#include <functional>

namespace rgbtir {

class BenchmarkEngine {
public:
    using ProgressCallback = std::function<void(int, int, const QString &)>;
    using PairStartedCallback = std::function<void(const ImagePair &, const QString &, int, int)>;

    static BenchmarkResult run(
        const BenchmarkConfig &config,
        const ProgressCallback &progress = {},
        const std::atomic_bool *cancelled = nullptr,
        const PairStartedCallback &pairStarted = {});
};

} // namespace rgbtir

