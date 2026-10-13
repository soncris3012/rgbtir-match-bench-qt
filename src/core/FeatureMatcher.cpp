#include "core/FeatureMatcher.h"

#include <opencv2/features2d.hpp>
#include <opencv2/core/version.hpp>
#if CV_VERSION_MAJOR >= 5
#include <opencv2/xfeatures2d.hpp>
#endif
#include <opencv2/imgproc.hpp>
#include <stdexcept>

namespace rgbtir {
namespace {

class OpenCvFeatureMatcher final : public IFeatureMatcher {
public:
    OpenCvFeatureMatcher(const QString &method, int maxKeypoints)
        : method_(method)
    {
        if (method == QStringLiteral("SIFT")) {
            detector_ = cv::SIFT::create(maxKeypoints);
            norm_ = cv::NORM_L2;
            ratio_ = 0.80f;
        } else if (method == QStringLiteral("ORB")) {
            detector_ = cv::ORB::create(maxKeypoints);
            norm_ = cv::NORM_HAMMING;
            ratio_ = 0.82f;
        } else if (method == QStringLiteral("AKAZE")) {
#if CV_VERSION_MAJOR >= 5
            detector_ = cv::xfeatures2d::AKAZE::create();
#else
            detector_ = cv::AKAZE::create();
#endif
            norm_ = cv::NORM_HAMMING;
            ratio_ = 0.82f;
        } else if (method == QStringLiteral("BRISK")) {
#if CV_VERSION_MAJOR >= 5
            detector_ = cv::xfeatures2d::BRISK::create();
#else
            detector_ = cv::BRISK::create();
#endif
            norm_ = cv::NORM_HAMMING;
            ratio_ = 0.82f;
        } else {
            throw std::invalid_argument("Unsupported matcher");
        }
    }

    QString name() const override { return method_; }

    MatchResult match(const cv::Mat &visible, const cv::Mat &thermal) override
    {
        cv::Mat gray0;
        cv::Mat gray1;
        cv::cvtColor(visible, gray0, cv::COLOR_BGR2GRAY);
        cv::cvtColor(thermal, gray1, cv::COLOR_BGR2GRAY);

        std::vector<cv::KeyPoint> keypoints0;
        std::vector<cv::KeyPoint> keypoints1;
        cv::Mat descriptors0;
        cv::Mat descriptors1;
        detector_->detectAndCompute(gray0, cv::noArray(), keypoints0, descriptors0);
        detector_->detectAndCompute(gray1, cv::noArray(), keypoints1, descriptors1);

        MatchResult result;
        if (descriptors0.empty() || descriptors1.empty() || descriptors1.rows < 2)
            return result;

        std::vector<std::vector<cv::DMatch>> candidates;
        cv::BFMatcher(norm_).knnMatch(descriptors0, descriptors1, candidates, 2);
        for (const auto &candidate : candidates) {
            if (candidate.size() < 2 || candidate[0].distance >= ratio_ * candidate[1].distance)
                continue;
            const cv::DMatch &match = candidate[0];
            result.points0.push_back(keypoints0.at(match.queryIdx).pt);
            result.points1.push_back(keypoints1.at(match.trainIdx).pt);
            result.confidence.push_back(1.0f / (1.0f + match.distance));
        }
        return result;
    }

private:
    QString method_;
    cv::Ptr<cv::Feature2D> detector_;
    int norm_ = cv::NORM_L2;
    float ratio_ = 0.8f;
};

} // namespace

QStringList FeatureMatcherFactory::availableMethods()
{
    return {QStringLiteral("SIFT"), QStringLiteral("ORB"),
            QStringLiteral("AKAZE"), QStringLiteral("BRISK")};
}

std::unique_ptr<IFeatureMatcher> FeatureMatcherFactory::create(
    const QString &method, int maxKeypoints)
{
    return std::make_unique<OpenCvFeatureMatcher>(method, maxKeypoints);
}

} // namespace rgbtir

