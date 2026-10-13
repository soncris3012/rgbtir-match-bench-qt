#include "core/FeatureMatcher.h"

#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>

namespace rgbtir {
namespace {

class OpenCvFeatureMatcher final : public IFeatureMatcher {
public:
    OpenCvFeatureMatcher(const QString &method, int maxKeypoints)
        : method_(method)
    {
        if (method == QStringLiteral("SIFT")
            || method == QStringLiteral("Gradient-SIFT")
            || method == QStringLiteral("Laplacian-SIFT")) {
            detector_ = cv::SIFT::create(maxKeypoints);
            norm_ = cv::NORM_L2;
            ratio_ = 0.80f;
        } else if (method == QStringLiteral("ORB")) {
            detector_ = cv::ORB::create(maxKeypoints);
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
        gray0 = representation(visible);
        gray1 = representation(thermal);

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
    cv::Mat representation(const cv::Mat &image) const
    {
        cv::Mat gray;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        if (method_ == QStringLiteral("Gradient-SIFT")) {
            cv::Mat dx;
            cv::Mat dy;
            cv::Mat magnitude;
            cv::Sobel(gray, dx, CV_32F, 1, 0, 3);
            cv::Sobel(gray, dy, CV_32F, 0, 1, 3);
            cv::magnitude(dx, dy, magnitude);
            cv::normalize(magnitude, magnitude, 0, 255, cv::NORM_MINMAX);
            magnitude.convertTo(gray, CV_8U);
            cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0.0);
        } else if (method_ == QStringLiteral("Laplacian-SIFT")) {
            cv::Mat smooth;
            cv::Mat laplacian;
            cv::GaussianBlur(gray, smooth, cv::Size(5, 5), 1.0);
            cv::Laplacian(smooth, laplacian, CV_32F, 3);
            cv::convertScaleAbs(laplacian, gray);
        }
        return gray;
    }

    QString method_;
    cv::Ptr<cv::Feature2D> detector_;
    int norm_ = cv::NORM_L2;
    float ratio_ = 0.8f;
};

} // namespace

QStringList FeatureMatcherFactory::availableMethods()
{
    return {QStringLiteral("SIFT"), QStringLiteral("Gradient-SIFT"),
            QStringLiteral("Laplacian-SIFT"), QStringLiteral("ORB")};
}

std::unique_ptr<IFeatureMatcher> FeatureMatcherFactory::create(
    const QString &method, int maxKeypoints)
{
    return std::make_unique<OpenCvFeatureMatcher>(method, maxKeypoints);
}

} // namespace rgbtir

