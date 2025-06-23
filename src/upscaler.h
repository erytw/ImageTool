#pragma once
#include "image.h"
#include <memory>
#include <string>
#include <vector>

// Try to detect OpenCV availability
#ifdef __has_include
#if __has_include(<opencv2/opencv.hpp>)
#define OPENCV_AVAILABLE 1
#include <opencv2/opencv.hpp>
// Try to include superres, but don't fail if not available
#if __has_include(<opencv2/superres.hpp>)
#define OPENCV_SUPERRES_AVAILABLE 1
#include <opencv2/superres.hpp>
#else
#define OPENCV_SUPERRES_AVAILABLE 0
#endif
// Try to include dnn_superres, but don't fail if not available
#if __has_include(<opencv2/dnn_superres.hpp>)
#define OPENCV_DNN_SUPERRES_AVAILABLE 1
#include <opencv2/dnn_superres.hpp>
#else
#define OPENCV_DNN_SUPERRES_AVAILABLE 0
#endif
#else
#define OPENCV_AVAILABLE 0
#define OPENCV_SUPERRES_AVAILABLE 0
#define OPENCV_DNN_SUPERRES_AVAILABLE 0
#endif
#else
#ifndef OPENCV_AVAILABLE
#define OPENCV_AVAILABLE 1
#include <opencv2/opencv.hpp>
#ifndef OPENCV_SUPERRES_AVAILABLE
#define OPENCV_SUPERRES_AVAILABLE 1
#include <opencv2/superres.hpp>
#endif
#ifndef OPENCV_DNN_SUPERRES_AVAILABLE
#define OPENCV_DNN_SUPERRES_AVAILABLE 1
#include <opencv2/dnn_superres.hpp>
#endif
#else
#define OPENCV_SUPERRES_AVAILABLE 0
#define OPENCV_DNN_SUPERRES_AVAILABLE 0
#endif
#endif

enum class UpscaleMethod {
    // Non-AI methods
    BICUBIC,
    LANCZOS,
    BTVL1,

    // AI methods
    ESPCN,
    EDSR,
    FSRCNN,
    LAPSRN
};

class BaseUpscaler {
  public:
    virtual ~BaseUpscaler() = default;
    virtual void upscale(Image &image, int scale_factor) = 0;
    virtual std::string getName() const = 0;
    virtual bool isAI() const = 0;
};

class TraditionalUpscaler : public BaseUpscaler {
  private:
    UpscaleMethod method;
#if OPENCV_AVAILABLE
    cv::Ptr<cv::superres::SuperResolution> sr_processor;
#else
    void *sr_processor; // Placeholder when OpenCV not available
#endif

  public:
    explicit TraditionalUpscaler(UpscaleMethod method);
    ~TraditionalUpscaler() override = default;

    void upscale(Image &image, int scale_factor) override;
    std::string getName() const override;
    bool isAI() const override { return false; }

  private:
#if OPENCV_AVAILABLE
    cv::Mat imageToMat(const Image &image);
    void matToImage(const cv::Mat &mat, Image &image);
#endif
};

class AIUpscaler : public BaseUpscaler {
  private:
    UpscaleMethod method;
#if OPENCV_AVAILABLE
    cv::dnn_superres::DnnSuperResImpl sr;
#endif
    bool model_loaded;
    std::string model_path;

  public:
    explicit AIUpscaler(UpscaleMethod method, const std::string &model_path = "");
    ~AIUpscaler() override = default;

    void upscale(Image &image, int scale_factor) override;
    std::string getName() const override;
    bool isAI() const override { return true; }
    bool loadModel(const std::string &path);

  private:
#if OPENCV_AVAILABLE
    cv::Mat imageToMat(const Image &image);
    void matToImage(const cv::Mat &mat, Image &image);
#endif
    std::string getModelName() const;
};

class UpscalerFactory {
  public:
    static std::unique_ptr<BaseUpscaler> createUpscaler(UpscaleMethod method,
                                                        const std::string &model_path = "");
    static std::vector<UpscaleMethod> getAvailableMethods();
    static std::string methodToString(UpscaleMethod method);
    static UpscaleMethod stringToMethod(const std::string &method_name);
};
