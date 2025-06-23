#include "upscaler.h"
#include <stdexcept>

TraditionalUpscaler::TraditionalUpscaler(UpscaleMethod method) : method(method) {
    if (method == UpscaleMethod::BTVL1) {
        try {
            sr_processor = cv::superres::createSuperResolution_BTVL1();
        } catch (const cv::Exception &e) {
            throw std::runtime_error("Failed to create BTVL1 processor: " + std::string(e.what()));
        }
    }
}

void TraditionalUpscaler::upscale(Image &image, int scale_factor) {
    cv::Mat input_mat = imageToMat(image);
    cv::Mat output_mat;

    switch (method) {
    case UpscaleMethod::BICUBIC: {
        cv::Size new_size(image.getWidth() * scale_factor, image.getHeight() * scale_factor);
        cv::resize(input_mat, output_mat, new_size, 0, 0, cv::INTER_CUBIC);
        break;
    }
    case UpscaleMethod::LANCZOS: {
        cv::Size new_size(image.getWidth() * scale_factor, image.getHeight() * scale_factor);
        cv::resize(input_mat, output_mat, new_size, 0, 0, cv::INTER_LANCZOS4);
        break;
    }
    case UpscaleMethod::BTVL1: {
        cv::Size new_size(image.getWidth() * scale_factor, image.getHeight() * scale_factor);
        cv::resize(input_mat, output_mat, new_size, 0, 0, cv::INTER_LANCZOS4);
        break;
    }
    default:
        throw std::invalid_argument("Unsupported traditional upscale method");
    }

    if (output_mat.empty()) {
        throw std::runtime_error("Upscaling failed - output is empty");
    }

    matToImage(output_mat, image);
}

std::string TraditionalUpscaler::getName() const {
    switch (method) {
    case UpscaleMethod::BICUBIC:
        return "Bicubic";
    case UpscaleMethod::LANCZOS:
        return "Lanczos";
    case UpscaleMethod::BTVL1:
        return "BTVL1";
    default:
        return "Unknown";
    }
}

cv::Mat TraditionalUpscaler::imageToMat(const Image &image) {
    cv::Mat mat(image.getHeight(), image.getWidth(), CV_8UC3);

    for (int y = 0; y < image.getHeight(); ++y) {
        for (int x = 0; x < image.getWidth(); ++x) {
            rgbPixel pixel = const_cast<Image &>(image).getPixel(x, y);
            cv::Vec3b &mat_pixel = mat.at<cv::Vec3b>(y, x);
            mat_pixel[0] = pixel.b;
            mat_pixel[1] = pixel.g;
            mat_pixel[2] = pixel.r;
        }
    }

    return mat;
}

void TraditionalUpscaler::matToImage(const cv::Mat &mat, Image &image) {
    Image new_image(mat.cols, mat.rows);

    for (int y = 0; y < mat.rows; ++y) {
        for (int x = 0; x < mat.cols; ++x) {
            const cv::Vec3b &mat_pixel = mat.at<cv::Vec3b>(y, x);
            new_image.pixels[y * mat.cols + x] = rgbPixel(mat_pixel[2], mat_pixel[1], mat_pixel[0]);
        }
    }

    image = std::move(new_image);
}

AIUpscaler::AIUpscaler(UpscaleMethod method, const std::string &model_path)
    : method(method), model_loaded(false), model_path(model_path) {
    if (!model_path.empty()) {
        model_loaded = loadModel(model_path);
    }
}

bool AIUpscaler::loadModel(const std::string &path) {
    try {
        sr.readModel(path);
        model_loaded = true;
        model_path = path;
        return true;
    } catch (const cv::Exception &e) {
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        model_loaded = false;
        return false;
    }
}

void AIUpscaler::upscale(Image &image, int scale_factor) {
    if (!model_loaded) {
        throw std::runtime_error("AI model not loaded. Please load a model first.");
    }

    cv::Mat input_mat = imageToMat(image);
    cv::Mat output_mat;

    try {
        sr.setModel(getModelName(), scale_factor);
        sr.upsample(input_mat, output_mat);
    } catch (const cv::Exception &e) {
        throw std::runtime_error("AI upscaling failed: " + std::string(e.what()));
    }

    if (output_mat.empty()) {
        throw std::runtime_error("AI upscaling failed - output is empty");
    }

    matToImage(output_mat, image);
}

std::string AIUpscaler::getName() const {
    switch (method) {
    case UpscaleMethod::ESPCN:
        return "ESPCN";
    case UpscaleMethod::EDSR:
        return "EDSR";
    case UpscaleMethod::FSRCNN:
        return "FSRCNN";
    case UpscaleMethod::LAPSRN:
        return "LAPSRN";
    default:
        return "Unknown AI";
    }
}

std::string AIUpscaler::getModelName() const {
    switch (method) {
    case UpscaleMethod::ESPCN:
        return "espcn";
    case UpscaleMethod::EDSR:
        return "edsr";
    case UpscaleMethod::FSRCNN:
        return "fsrcnn";
    case UpscaleMethod::LAPSRN:
        return "lapsrn";
    default:
        return "";
    }
}

cv::Mat AIUpscaler::imageToMat(const Image &image) {
    cv::Mat mat(image.getHeight(), image.getWidth(), CV_8UC3);

    for (int y = 0; y < image.getHeight(); ++y) {
        for (int x = 0; x < image.getWidth(); ++x) {
            rgbPixel pixel = const_cast<Image &>(image).getPixel(x, y);
            cv::Vec3b &mat_pixel = mat.at<cv::Vec3b>(y, x);
            mat_pixel[0] = pixel.b;
            mat_pixel[1] = pixel.g;
            mat_pixel[2] = pixel.r;
        }
    }

    return mat;
}

void AIUpscaler::matToImage(const cv::Mat &mat, Image &image) {
    Image new_image(mat.cols, mat.rows);

    for (int y = 0; y < mat.rows; ++y) {
        for (int x = 0; x < mat.cols; ++x) {
            const cv::Vec3b &mat_pixel = mat.at<cv::Vec3b>(y, x);
            new_image.pixels[y * mat.cols + x] = rgbPixel(mat_pixel[2], mat_pixel[1], mat_pixel[0]);
        }
    }

    image = std::move(new_image);
}

std::unique_ptr<BaseUpscaler> UpscalerFactory::createUpscaler(UpscaleMethod method,
                                                              const std::string &model_path) {
    switch (method) {
    case UpscaleMethod::BICUBIC:
    case UpscaleMethod::LANCZOS:
    case UpscaleMethod::BTVL1:
        return std::make_unique<TraditionalUpscaler>(method);

    case UpscaleMethod::ESPCN:
    case UpscaleMethod::EDSR:
    case UpscaleMethod::FSRCNN:
    case UpscaleMethod::LAPSRN:
        return std::make_unique<AIUpscaler>(method, model_path);

    default:
        throw std::invalid_argument("Unknown upscale method");
    }
}

std::vector<UpscaleMethod> UpscalerFactory::getAvailableMethods() {
    return {UpscaleMethod::BICUBIC, UpscaleMethod::LANCZOS, UpscaleMethod::BTVL1,
            UpscaleMethod::ESPCN,   UpscaleMethod::EDSR,    UpscaleMethod::FSRCNN,
            UpscaleMethod::LAPSRN};
}

std::string UpscalerFactory::methodToString(UpscaleMethod method) {
    switch (method) {
    case UpscaleMethod::BICUBIC:
        return "BICUBIC";
    case UpscaleMethod::LANCZOS:
        return "LANCZOS";
    case UpscaleMethod::BTVL1:
        return "BTVL1";
    case UpscaleMethod::ESPCN:
        return "ESPCN";
    case UpscaleMethod::EDSR:
        return "EDSR";
    case UpscaleMethod::FSRCNN:
        return "FSRCNN";
    case UpscaleMethod::LAPSRN:
        return "LAPSRN";
    default:
        return "UNKNOWN";
    }
}

UpscaleMethod UpscalerFactory::stringToMethod(const std::string &method_name) {
    if (method_name == "BICUBIC") return UpscaleMethod::BICUBIC;
    if (method_name == "LANCZOS") return UpscaleMethod::LANCZOS;
    if (method_name == "BTVL1") return UpscaleMethod::BTVL1;
    if (method_name == "ESPCN") return UpscaleMethod::ESPCN;
    if (method_name == "EDSR") return UpscaleMethod::EDSR;
    if (method_name == "FSRCNN") return UpscaleMethod::FSRCNN;
    if (method_name == "LAPSRN") return UpscaleMethod::LAPSRN;

    throw std::invalid_argument("Unknown method name: " + method_name);
}
