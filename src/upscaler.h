#pragma once
#include "image.h"
#include <memory>
#include <string>
#include <vector>

#include <opencv2/dnn_superres.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/superres.hpp>

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
    cv::Ptr<cv::superres::SuperResolution> sr_processor;

  public:
    explicit TraditionalUpscaler(UpscaleMethod method);
    ~TraditionalUpscaler() override = default;

    void upscale(Image &image, int scale_factor) override;
    std::string getName() const override;
    bool isAI() const override { return false; }

  private:
    cv::Mat imageToMat(const Image &image);
    void matToImage(const cv::Mat &mat, Image &image);
};

class AIUpscaler : public BaseUpscaler {
  private:
    UpscaleMethod method;
    cv::dnn_superres::DnnSuperResImpl sr;
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
    cv::Mat imageToMat(const Image &image);
    void matToImage(const cv::Mat &mat, Image &image);
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
