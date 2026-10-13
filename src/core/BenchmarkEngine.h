#pragma once

#include "core/BenchmarkTypes.h"

#include <atomic>
#include <functional>

namespace rgbtir {

class BenchmarkEngine {
public:
    using ProgressCallback = std::function<void(int, int, const QString &)>;

    static BenchmarkResult run(
        const BenchmarkConfig &config,
        const ProgressCallback &progress = {},
        const std::atomic_bool *cancelled = nullptr);
};

} // namespace rgbtir

