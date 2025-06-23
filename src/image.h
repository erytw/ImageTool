#pragma once
#include <cstdio>
#include <string>
#include <vector>

#pragma pack(push, 1)

struct yuvPixel;

struct rgbPixel {
    unsigned char b, g, r;
    static constexpr double RGB_TO_Y_R = 0.299;
    static constexpr double RGB_TO_Y_G = 0.587;
    static constexpr double RGB_TO_Y_B = 0.114;
    static constexpr double RGB_TO_U_R = -0.168736;
    static constexpr double RGB_TO_U_G = -0.331264;
    static constexpr double RGB_TO_U_B = 0.5;
    static constexpr double RGB_TO_V_R = 0.5;
    static constexpr double RGB_TO_V_G = -0.418688;
    static constexpr double RGB_TO_V_B = -0.081312;

    rgbPixel(int r = 0, int g = 0, int b = 0) : b(b), g(g), r(r) {}
    rgbPixel(const yuvPixel &yuv);
    void toGrayScale() noexcept;
};

struct yuvPixel {
    unsigned char y, u, v;
    static constexpr double YUV_TO_R_Y = 1.0;
    static constexpr double YUV_TO_R_U = 0.0;
    static constexpr double YUV_TO_R_V = 1.402;
    static constexpr double YUV_TO_G_Y = 1.0;
    static constexpr double YUV_TO_G_U = -0.344136;
    static constexpr double YUV_TO_G_V = -0.714136;
    static constexpr double YUV_TO_B_Y = 1.0;
    static constexpr double YUV_TO_B_U = 1.772;
    static constexpr double YUV_TO_B_V = 0.0;

    yuvPixel(int y, int u, int v) : y(y), u(u), v(v) {}
    yuvPixel(const rgbPixel &rgb);
    void toGrayScale() noexcept;
};

#pragma pack(pop)

enum ImageFormat { BMP = 0, YUV420P = 1, YUV422P = 2, YUV444P = 3 };

class Image {
    friend class TraditionalUpscaler;
    friend class AIUpscaler;

  public:
    Image(int _width = 0, int _height = 0)
        : width(_width), height(_height), is_grayscale(false), pixels(_width * _height) {}
    ~Image();

    int getWidth() const noexcept;
    int getHeight() const noexcept;
    bool isGrayScale() noexcept;
    rgbPixel getPixel(int x, int y);

    void loadImage(FILE *file, ImageFormat format);
    void loadImageFromFile(std::string filename, ImageFormat format);
    void saveImage(FILE *file, ImageFormat format);
    void saveImageToFile(std::string filename, ImageFormat format);

    void upSample(const int coefficient) noexcept;
    void downSample(const int coefficient) noexcept;
    void switchGrayScale() noexcept;

  private:
    int width;
    int height;
    bool is_grayscale;
    std::vector<rgbPixel> pixels;
};
