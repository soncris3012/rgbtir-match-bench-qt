#include "core/MetricCalculator.h"

#include <opencv2/calib3d.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>

namespace rgbtir {
namespace {

double percentile(std::vector<double> values, double probability)
{
    if (values.empty())
        return 0.0;
    std::sort(values.begin(), values.end());
    const double index = probability * static_cast<double>(values.size() - 1);
    const auto low = static_cast<std::size_t>(std::floor(index));
    const auto high = static_cast<std::size_t>(std::ceil(index));
    const double fraction = index - static_cast<double>(low);
    return values[low] * (1.0 - fraction) + values[high] * fraction;
}

double ratioWithin(const std::vector<double> &errors, double threshold)
{
    if (errors.empty())
        return 0.0;
    return static_cast<double>(std::count_if(errors.begin(), errors.end(),
        [threshold](double error) { return error <= threshold; })) / static_cast<double>(errors.size());
}

} // namespace

PairMetrics MetricCalculator::evaluate(
    const MatchResult &matches,
    const cv::Size &visibleSize,
    const cv::Size &thermalSize,
    double correctnessPx,
    double ransacPx)
{
    PairMetrics metrics;
    metrics.matches = static_cast<int>(matches.points0.size());
    if (matches.points0.empty() || matches.points0.size() != matches.points1.size())
        return metrics;

    const double scaleX = static_cast<double>(visibleSize.width) / thermalSize.width;
    const double scaleY = static_cast<double>(visibleSize.height) / thermalSize.height;
    std::vector<cv::Point2f> normalized1;
    std::vector<double> errors;
    normalized1.reserve(matches.points1.size());
    errors.reserve(matches.points1.size());
    std::array<bool, 16> occupied{};

    for (std::size_t i = 0; i < matches.points0.size(); ++i) {
        const cv::Point2f p1(
            static_cast<float>(matches.points1[i].x * scaleX),
            static_cast<float>(matches.points1[i].y * scaleY));
        normalized1.push_back(p1);
        errors.push_back(cv::norm(matches.points0[i] - p1));
        const int gx = std::clamp(static_cast<int>(matches.points0[i].x / visibleSize.width * 4.0), 0, 3);
        const int gy = std::clamp(static_cast<int>(matches.points0[i].y / visibleSize.height * 4.0), 0, 3);
        occupied[static_cast<std::size_t>(gy * 4 + gx)] = true;
    }

    metrics.correctMatches = static_cast<int>(std::count_if(errors.begin(), errors.end(),
        [correctnessPx](double error) { return error <= correctnessPx; }));
    metrics.precision = static_cast<double>(metrics.correctMatches) / metrics.matches;
    metrics.meanError = std::accumulate(errors.begin(), errors.end(), 0.0) / errors.size();
    metrics.medianError = percentile(errors, 0.5);
    metrics.p95Error = percentile(errors, 0.95);
    metrics.mma1 = ratioWithin(errors, 1.0);
    metrics.mma3 = ratioWithin(errors, 3.0);
    metrics.mma5 = ratioWithin(errors, 5.0);
    metrics.mma10 = ratioWithin(errors, 10.0);
    metrics.coverage = static_cast<double>(std::count(occupied.begin(), occupied.end(), true)) / occupied.size();

    if (matches.points0.size() >= 4) {
        cv::Mat inlierMask;
        const cv::Mat homography = cv::findHomography(
            matches.points0, normalized1, cv::RANSAC, ransacPx, inlierMask);
        if (!homography.empty() && !inlierMask.empty()) {
            metrics.homographyInliers = cv::countNonZero(inlierMask);
            metrics.inlierRatio = static_cast<double>(metrics.homographyInliers) / metrics.matches;
            std::vector<cv::Point2f> projected;
            cv::perspectiveTransform(matches.points0, projected, homography);
            double squaredError = 0.0;
            for (int i = 0; i < inlierMask.rows; ++i) {
                if (inlierMask.at<uchar>(i)) {
                    const cv::Point2f delta = projected.at(i) - normalized1.at(i);
                    squaredError += static_cast<double>(delta.x) * delta.x
                                  + static_cast<double>(delta.y) * delta.y;
                }
            }
            if (metrics.homographyInliers > 0)
                metrics.homographyRmse = std::sqrt(squaredError / metrics.homographyInliers);
        }
    }
    return metrics;
}

} // namespace rgbtir

