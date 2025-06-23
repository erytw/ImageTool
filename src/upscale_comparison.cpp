#include "compare.h"
#include "image.h"
#include "upscaler.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct UpscaleResult {
    std::string method_name;
    bool is_ai;
    double mse;
    double psnr;
    double time_seconds;
    bool success;
    std::string error_message;
};

void printResults(const std::vector<UpscaleResult> &results) {
    std::cout << "\n=== UPSCALING COMPARISON RESULTS ===\n" << std::endl;
    std::cout << "Method\t\tType\t\tMSE\t\tPSNR\t\tTime(s)\t\tStatus" << std::endl;
    std::cout << "-----------------------------------------------------------------------"
              << std::endl;

    for (const auto &result : results) {
        std::cout << result.method_name << "\t\t" << (result.is_ai ? "AI" : "Algo") << "\t\t";

        if (result.success) {
            std::cout << std::fixed << std::setprecision(2) << result.mse << "\t\t" << result.psnr
                      << "\t\t" << result.time_seconds << "\t\tOK";
        } else {
            std::cout << "N/A\t\tN/A\t\tN/A\t\tFAILED: " << result.error_message;
        }
        std::cout << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0]
                  << " <input_file> <input_format> <scale_factor> [model_directory]" << std::endl;
        std::cout << "Example: " << argv[0] << " input.bmp BMP 2 ./models/" << std::endl;
        return 1;
    }

    std::string input_filename = argv[1];
    std::string input_format_name = argv[2];
    int scale_factor = std::atoi(argv[3]);
    std::string model_dir = (argc > 4) ? argv[4] : "./models/";

    if (scale_factor <= 0) {
        std::cerr << "Scale factor must be a positive integer" << std::endl;
        return 1;
    }

    if (!model_dir.empty() && model_dir.back() != '/') {
        model_dir += '/';
    }

    ImageFormat input_format;
    try {
        if (input_format_name == "BMP") {
            input_format = ImageFormat::BMP;
        } else if (input_format_name == "YUV420P") {
            input_format = ImageFormat::YUV420P;
        } else if (input_format_name == "YUV422P") {
            input_format = ImageFormat::YUV422P;
        } else if (input_format_name == "YUV444P") {
            input_format = ImageFormat::YUV444P;
        } else {
            throw std::invalid_argument("Invalid input format");
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    Image original_image;
    try {
        original_image.loadImageFromFile(input_filename, input_format);
        std::cout << "Loaded image: " << original_image.getWidth() << "x"
                  << original_image.getHeight() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error loading image: " << e.what() << std::endl;
        return 1;
    }

    Image downsampled = original_image;
    downsampled.downSample(scale_factor);
    std::cout << "Downsampled to: " << downsampled.getWidth() << "x" << downsampled.getHeight()
              << std::endl;

    std::vector<UpscaleResult> results;

    std::vector<UpscaleMethod> traditional_methods = {UpscaleMethod::BICUBIC,
                                                      UpscaleMethod::LANCZOS};

    if (scale_factor >= 2 && scale_factor <= 4) {
        traditional_methods.push_back(UpscaleMethod::BTVL1);
    }

    for (auto method : traditional_methods) {
        UpscaleResult result;
        result.method_name = UpscalerFactory::methodToString(method);
        result.is_ai = false;

        try {
            Image test_image = downsampled;
            auto upscaler = UpscalerFactory::createUpscaler(method);

            auto start = std::chrono::high_resolution_clock::now();
            upscaler->upscale(test_image, scale_factor);
            auto end = std::chrono::high_resolution_clock::now();

            result.time_seconds = std::chrono::duration<double>(end - start).count();

            result.mse = MSE(original_image, test_image, true); // ignore dimensions
            result.psnr = psnr(result.mse, 255);
            result.success = true;

            std::string output_filename = "output/output_" + result.method_name + ".bmp";
            test_image.saveImageToFile(output_filename, ImageFormat::BMP);
            std::cout << "Saved: " << output_filename << std::endl;

        } catch (const std::exception &e) {
            result.success = false;
            result.error_message = e.what();
        }

        results.push_back(result);
    }

    std::vector<std::pair<UpscaleMethod, std::string>> ai_methods = {
        {UpscaleMethod::ESPCN, "ESPCN_x" + std::to_string(scale_factor) + ".pb"},
        {UpscaleMethod::FSRCNN, "FSRCNN_x" + std::to_string(scale_factor) + ".pb"},
        {UpscaleMethod::EDSR, "EDSR_x" + std::to_string(scale_factor) + ".pb"}};

    if (scale_factor == 2 || scale_factor == 4 || scale_factor == 8) {
        ai_methods.push_back(
            {UpscaleMethod::LAPSRN, "LapSRN_x" + std::to_string(scale_factor) + ".pb"});
    }

    for (auto &[method, model_file] : ai_methods) {
        UpscaleResult result;
        result.method_name = UpscalerFactory::methodToString(method);
        result.is_ai = true;

        try {
            Image test_image = downsampled;
            std::string model_path = model_dir + model_file;
            auto upscaler = UpscalerFactory::createUpscaler(method, model_path);

            auto start = std::chrono::high_resolution_clock::now();
            upscaler->upscale(test_image, scale_factor);
            auto end = std::chrono::high_resolution_clock::now();

            result.time_seconds = std::chrono::duration<double>(end - start).count();

            result.mse = MSE(original_image, test_image, true);
            result.psnr = psnr(result.mse, 255);
            result.success = true;

            std::string output_filename = "output/output_" + result.method_name + ".bmp";
            test_image.saveImageToFile(output_filename, ImageFormat::BMP);
            std::cout << "Saved: " << output_filename << std::endl;

        } catch (const std::exception &e) {
            result.success = false;
            result.error_message = e.what();
        }

        results.push_back(result);
    }

    printResults(results);

    std::cout << "\nComparison complete! Check output_*.bmp files for visual comparison."
              << std::endl;

    return 0;
}
