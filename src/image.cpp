#include "image.h"
#include "bmp.h"

#include <cstdio>
#include <stdexcept>
#include <sys/wait.h>

inline unsigned char clamp(int value) {
    return static_cast<unsigned char>(std::min(std::max(value, 0), 255));
}

yuvPixel::yuvPixel(const rgbPixel &rgb) {
    y = clamp(rgbPixel::RGB_TO_Y_R * rgb.r + rgbPixel::RGB_TO_Y_G * rgb.g +
              rgbPixel::RGB_TO_Y_B * rgb.b);
    u = clamp(128 + rgbPixel::RGB_TO_U_R * rgb.r + rgbPixel::RGB_TO_U_G * rgb.g +
              rgbPixel::RGB_TO_U_B * rgb.b);
    v = clamp(128 + rgbPixel::RGB_TO_V_R * rgb.r + rgbPixel::RGB_TO_V_G * rgb.g +
              rgbPixel::RGB_TO_V_B * rgb.b);
}

rgbPixel::rgbPixel(const yuvPixel &yuv) {
    int y_norm = yuv.y;
    int u_norm = yuv.u - 128;
    int v_norm = yuv.v - 128;

    int r_temp = y_norm + yuvPixel::YUV_TO_R_U * u_norm + yuvPixel::YUV_TO_R_V * v_norm;
    int g_temp = y_norm + yuvPixel::YUV_TO_G_U * u_norm + yuvPixel::YUV_TO_G_V * v_norm;
    int b_temp = y_norm + yuvPixel::YUV_TO_B_U * u_norm + yuvPixel::YUV_TO_B_V * v_norm;

    r = clamp(r_temp);
    g = clamp(g_temp);
    b = clamp(b_temp);
}

void rgbPixel::toGrayScale() noexcept {
    unsigned char y =
        clamp(rgbPixel::RGB_TO_Y_R * r + rgbPixel::RGB_TO_Y_G * g + rgbPixel::RGB_TO_Y_B * b);
    r = g = b = y;
}

void yuvPixel::toGrayScale() noexcept { u = v = 0; }

BMPHeader::BMPHeader(int width, int height) : signature(0x4D42), reserved(0), dataOffset(54) {
    fileSize = sizeof(BMPHeader) + sizeof(BMPInfoHeader) +
               ((width * sizeof(rgbPixel) + 3) & (~3)) * height;
}

BMPInfoHeader::BMPInfoHeader(int width, int height)
    : size(40), width(width), height(height), planes(1), bitsPerPixel(24), compression(0),
      xPixelsPerM(0), yPixelsPerM(0), colorsUsed(0), importantColors(0) {
    imageSize = ((width * sizeof(rgbPixel) + 3) & (~3)) * height;
}

Image::~Image() {}

int Image::getHeight() const noexcept { return height; }

int Image::getWidth() const noexcept { return width; }

bool Image::isGrayScale() noexcept { return is_grayscale; }

rgbPixel Image::getPixel(int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        throw std::out_of_range("Pixel coordinates out of bounds");
    }
    return pixels[y * width + x];
}

void Image::loadImageFromFile(std::string filename, ImageFormat format) {
    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) throw std::runtime_error("Couldn't open file \"" + filename + "\"");
    try {
        loadImage(file, format);
    } catch (const std::exception &e) {
        fclose(file);
        throw e;
    }
    fclose(file);
}

void Image::loadImage(FILE *file, ImageFormat format) {
    if (!file) {
        throw std::runtime_error("Invalid file handle");
    }

    if (format == ImageFormat::BMP) {
        BMPHeader bmpHeader;
        BMPInfoHeader bmpInfoHeader;
        if (fread(&bmpHeader, sizeof(BMPHeader), 1, file) != 1 ||
            fread(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, file) != 1) {
            throw std::runtime_error("Failed to read BMP headers");
        }
        if (bmpHeader.signature != 0x4D42) {
            throw std::runtime_error("Invalid BMP signature");
        }

        width = bmpInfoHeader.width;
        height = bmpInfoHeader.height;
        pixels.resize(width * height);

        int rowPadding = (4 - (width * 3) % 4) % 4;

        fseek(file, bmpHeader.dataOffset, SEEK_SET);

        std::vector<unsigned char> rowBuffer(width * 3 + rowPadding);
        for (int y = height - 1; y >= 0; y--) {
            if (fread(rowBuffer.data(), 1, width * 3 + rowPadding, file) !=
                width * 3 + rowPadding) {
                throw std::runtime_error("Failed to read BMP pixel data: y=" + std::to_string(y));
            }

            for (int x = 0; x < width; x++) {
                int index = y * width + x;
                pixels[index] = rgbPixel(rowBuffer[x * 3 + 2], // R
                                         rowBuffer[x * 3 + 1], // G
                                         rowBuffer[x * 3]      // B
                );
            }
        }
    } else {
        int yPlaneWidth = width;
        int yPlaneHeight = height;
        int heightDivider, widthDivider;

        switch (format) {
        case ImageFormat::YUV420P:
            heightDivider = 2;
            widthDivider = 2;
            break;
        case ImageFormat::YUV422P:
            heightDivider = 1;
            widthDivider = 2;
            break;
        case ImageFormat::YUV444P:
            heightDivider = 1;
            widthDivider = 1;
            break;
        default:
            throw std::invalid_argument("Unsupported image format");
        }

        int uPlaneWidth = (width + widthDivider - 1) / widthDivider;
        int uPlaneHeight = (height + heightDivider - 1) / heightDivider;
        int vPlaneWidth = uPlaneWidth;
        int vPlaneHeight = uPlaneHeight;

        std::vector<unsigned char> yPlane(yPlaneWidth * yPlaneHeight);
        std::vector<unsigned char> uPlane(uPlaneWidth * uPlaneHeight);
        std::vector<unsigned char> vPlane(vPlaneWidth * vPlaneHeight);

        if (fread(yPlane.data(), sizeof(unsigned char), yPlaneWidth * yPlaneHeight, file) !=
                yPlaneWidth * yPlaneHeight ||
            fread(uPlane.data(), sizeof(unsigned char), uPlaneWidth * uPlaneHeight, file) !=
                uPlaneWidth * uPlaneHeight ||
            fread(vPlane.data(), sizeof(unsigned char), vPlaneWidth * vPlaneHeight, file) !=
                vPlaneWidth * vPlaneHeight) {
            throw std::runtime_error("Failed to read YUV data from file");
        }

        pixels.resize(width * height);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                yuvPixel yuv(yPlane[y * yPlaneWidth + x],
                             uPlane[(y / heightDivider) * uPlaneWidth + (x / widthDivider)],
                             vPlane[(y / heightDivider) * vPlaneWidth + (x / widthDivider)]);
                pixels[y * width + x] = rgbPixel(yuv);
            }
        }
    }
}

void Image::saveImageToFile(std::string filename, ImageFormat format) {
    FILE *file = fopen(filename.c_str(), "wb");
    if (!file) throw std::runtime_error("Couldn't open file \"" + filename + "\"");
    try {
        saveImage(file, format);
    } catch (const std::exception &e) {
        fclose(file);
        throw e;
    }
    fclose(file);
}

void Image::saveImage(FILE *file, ImageFormat format) {
    if (format == ImageFormat::BMP) {
        BMPHeader bmpHeader(width, height);
        BMPInfoHeader bmpInfoHeader(width, height);
        if (fwrite(&bmpHeader, sizeof(BMPHeader), 1, file) != 1 ||
            fwrite(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, file) != 1)
            throw std::runtime_error("Couldn't write to file");

        int padding_size = (4 - (width * sizeof(rgbPixel) & 0b11)) & 0b11;
        std::vector<rgbPixel> rowBuffer(width);
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                rowBuffer[x] = pixels[y * width + x];
                if (is_grayscale) rowBuffer[x].toGrayScale();
            }
            if (fwrite(rowBuffer.data(), sizeof(rgbPixel), width, file) != width ||
                fwrite("\0\0\0", 1, padding_size, file) != padding_size)
                throw std::runtime_error("Couldn't write to file");
        }
        return;
    }
    if (format != YUV420P && format != YUV422P && format != YUV444P) {
        throw std::invalid_argument("Unsupported image format");
    }

    int vertical_step = format == ImageFormat::YUV420P ? 2 : 1;
    int horizontal_step = format == ImageFormat::YUV444P ? 1 : 2;

    std::vector<unsigned char> yPlane(width * height);
    std::vector<unsigned char> uPlane((width / horizontal_step) * (height / vertical_step));
    std::vector<unsigned char> vPlane((width / horizontal_step) * (height / vertical_step));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            rgbPixel pixel = rgbPixel(pixels[y * width + x]);
            if (is_grayscale) pixel.toGrayScale();
            yuvPixel yuvpixel = yuvPixel(pixel);
            yPlane[y * width + x] = yuvpixel.y;
        }
    }

    for (int y = 0; y < height; y += vertical_step) {
        for (int x = 0; x < width; x += horizontal_step) {
            rgbPixel pixel = rgbPixel(pixels[y * width + x]);
            if (is_grayscale) pixel.toGrayScale();
            yuvPixel yuvpixel = yuvPixel(pixel);
            uPlane[(y / vertical_step) * (width / horizontal_step) + (x / horizontal_step)] =
                yuvpixel.u;
            vPlane[(y / vertical_step) * (width / horizontal_step) + (x / horizontal_step)] =
                yuvpixel.v;
        }
    }

    if (fwrite(yPlane.data(), 1, yPlane.size(), file) != yPlane.size() ||
        fwrite(uPlane.data(), 1, uPlane.size(), file) != uPlane.size() ||
        fwrite(vPlane.data(), 1, vPlane.size(), file) != vPlane.size())
        throw std::runtime_error("Couldn't write to file");
}

void Image::downSample(const int coefficient) noexcept {
    int newWidth = width / coefficient;
    int newHeight = height / coefficient;
    std::vector<rgbPixel> new_pixels(newWidth * newHeight);

    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            int r_sum = 0;
            int g_sum = 0;
            int b_sum = 0;

            for (int y_delta = 0; y_delta < coefficient; ++y_delta) {
                for (int x_delta = 0; x_delta < coefficient; ++x_delta) {
                    rgbPixel pixel =
                        pixels[(y * coefficient + y_delta) * width + (x * coefficient + x_delta)];
                    r_sum += pixel.r;
                    g_sum += pixel.g;
                    b_sum += pixel.b;
                }
            }

            new_pixels[y * newWidth + x] =
                rgbPixel(r_sum / (coefficient * coefficient), g_sum / (coefficient * coefficient),
                         b_sum / (coefficient * coefficient));
        }
    }

    pixels = new_pixels;
    width = newWidth;
    height = newHeight;
}

void Image::upSample(const int coefficient) noexcept {
    int new_width = width * coefficient;
    int new_height = height * coefficient;
    std::vector<rgbPixel> new_pixels(new_width * new_height);

    for (int y = 0; y < new_height; ++y) {
        for (int x = 0; x < new_width; ++x) {
            float x_ratio = (float)x / coefficient;
            float y_ratio = (float)y / coefficient;

            int x_base = (int)x_ratio;
            int y_base = (int)y_ratio;

            float x_diff = x_ratio - x_base;
            float y_diff = y_ratio - y_base;

            rgbPixel p1 = pixels[y_base * width + x_base];
            rgbPixel p2 = pixels[y_base * width + std::min(x_base + 1, width - 1)];
            rgbPixel p3 = pixels[std::min(y_base + 1, height - 1) * width + x_base];
            rgbPixel p4 =
                pixels[std::min(y_base + 1, height - 1) * width + std::min(x_base + 1, width - 1)];

            float r = (1 - x_diff) * (1 - y_diff) * p1.r + x_diff * (1 - y_diff) * p2.r +
                      (1 - x_diff) * y_diff * p3.r + x_diff * y_diff * p4.r;

            float g = (1 - x_diff) * (1 - y_diff) * p1.g + x_diff * (1 - y_diff) * p2.g +
                      (1 - x_diff) * y_diff * p3.g + x_diff * y_diff * p4.g;

            float b = (1 - x_diff) * (1 - y_diff) * p1.b + x_diff * (1 - y_diff) * p2.b +
                      (1 - x_diff) * y_diff * p3.b + x_diff * y_diff * p4.b;

            new_pixels[y * new_width + x] = rgbPixel(r, g, b);
        }
    }

    pixels = new_pixels;
    width = new_width;
    height = new_height;
}

void Image::switchGrayScale() noexcept { is_grayscale = !is_grayscale; }
