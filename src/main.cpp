#include "compare.h"
#include "image.h"
#include <iostream>
#include <string>

ImageFormat parseImageFormat(std::string format_name) {
    if (format_name == "YUV420P") {
        return ImageFormat::YUV420P;
    } else if (format_name == "YUV422P") {
        return ImageFormat::YUV422P;
    } else if (format_name == "YUV444P") {
        return ImageFormat::YUV444P;
    } else if (format_name == "BMP") {
        return ImageFormat::BMP;
    } else {
        throw std::invalid_argument("Invalid image format");
    }
}

int main(int argc, char *argv[]) {

    std::string input_filename, output_filename;
    std::string input_format_name, output_format_name;
    int upsample_coefficient = 0, downsample_coefficient = 0;
    bool grayscale = false, compare_results = false, compare_images = false;
    std::string compare_filename1, compare_filename2;
    int width = 0, height = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--width") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing value for --width" << std::endl;
                return 1;
            }
            width = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--height") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing value for --height" << std::endl;
                return 1;
            }
            height = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--input") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing value for --input" << std::endl;
                return 1;
            }
            input_filename = argv[i + 1];
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing value for --output" << std::endl;
                return 1;
            }
            output_filename = argv[i + 1];
        } else if (strcmp(argv[i], "--input-format") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing value for --input-format" << std::endl;
                return 1;
            }
            input_format_name = argv[i + 1];
        } else if (strcmp(argv[i], "--output-format") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing value for --output-format" << std::endl;
                return 1;
            }
            output_format_name = argv[i + 1];

        } else if (strcmp(argv[i], "--compare") == 0) {
            if (i + 2 >= argc) {
                std::cerr << "Error: Missing value for --compare" << std::endl;
                return 1;
            }
            compare_images = true;
            compare_filename1 = argv[i + 1];
            compare_filename2 = argv[i + 2];
        } else if (strcmp(argv[i], "--upsample") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing value for --upsample" << std::endl;
                return 1;
            }
            upsample_coefficient = atoi(argv[i + 1]);
            if (upsample_coefficient <= 0) {
                std::cerr << "Error: --upsample coefficient must be a positive integer"
                          << std::endl;
                return 1;
            }
        } else if (strcmp(argv[i], "--downsample") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Missing value for --downsample" << std::endl;
                return 1;
            }
            downsample_coefficient = atoi(argv[i + 1]);
            if (downsample_coefficient <= 0) {
                std::cerr << "Error: --downsample coefficient must be a positive integer"
                          << std::endl;
                return 1;
            }
        } else if (strcmp(argv[i], "--grayscale") == 0) {
            grayscale = true;
        } else if (strcmp(argv[i], "--compare-results") == 0) {
            compare_results = true;
        }
    }

    if (compare_images) {
        std::cout << "Comparing images, unrelated parameters ignored" << std::endl;
        ImageFormat format;
        try {
            format = parseImageFormat(input_format_name);
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
        Image image1, image2;
        image1.loadImageFromFile(compare_filename1, format);
        image2.loadImageFromFile(compare_filename2, format);

        try {
            double mse = MSE(image1, image2);
            std::cout << "MSE: " << mse << std::endl;
            std::cout << "PSNR: " << psnr(mse, 255) << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

    } else {
        ImageFormat input_format, output_format;
        try {
            input_format = parseImageFormat(input_format_name);
            output_format = parseImageFormat(output_format_name);
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

        if (input_format != ImageFormat::BMP && (width == 0 || height == 0)) {
            std::cerr << "Error: YUV formats require width and height provided before conversion"
                      << std::endl;
            return 1;
        }
        if (input_format == ImageFormat::BMP && (width != 0 || height != 0)) {
            if (width == 0 || height == 0) {
                std::cerr
                    << "Warning: BMP format contains width and height info, ignoring ambigous "
                       "width and height arguments"
                    << std::endl;
            }
        }

        Image image(width, height);

        try {
            image.loadImageFromFile(input_filename, parseImageFormat(input_format_name));
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

        Image start_image = image;

        if (grayscale) {
            image.switchGrayScale();
        }
        if (downsample_coefficient) {
            image.downSample(downsample_coefficient);
        }
        if (upsample_coefficient) {
            image.upSample(upsample_coefficient);
        }
        if (compare_results) {
            try {
                double mse = MSE(start_image, image);
                std::cout << "MSE: " << mse << std::endl;
                std::cout << "PSNR: " << psnr(mse, 255) << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return 1;
            }
        }
        try {
            image.saveImageToFile(output_filename, output_format);
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }
    return 0;
}
