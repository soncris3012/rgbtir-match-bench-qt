#pragma once

#include "core/BenchmarkTypes.h"

#include <memory>

namespace rgbtir {

class IFeatureMatcher {
public:
    virtual ~IFeatureMatcher() = default;
    virtual QString name() const = 0;
    virtual MatchResult match(const cv::Mat &visible, const cv::Mat &thermal) = 0;
};

class FeatureMatcherFactory {
public:
    static QStringList availableMethods();
    static std::unique_ptr<IFeatureMatcher> create(const QString &method, int maxKeypoints);
};

} // namespace rgbtir

