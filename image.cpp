#include "image.h"

yuvPixel::yuvPixel(const rgbPixel &rgb) {
    y = (unsigned char)(rgbPixel::RGB_TO_Y_R * rgb.r + rgbPixel::RGB_TO_Y_G * rgb.g + rgbPixel::RGB_TO_Y_B * rgb.b);
    u = (unsigned char)(128 + rgbPixel::RGB_TO_U_R * rgb.r + rgbPixel::RGB_TO_U_G * rgb.g + rgbPixel::RGB_TO_U_B * rgb.b);
    v = (unsigned char)(128 + rgbPixel::RGB_TO_V_R * rgb.r + rgbPixel::RGB_TO_V_G * rgb.g + rgbPixel::RGB_TO_V_B * rgb.b);
}

rgbPixel::rgbPixel(const yuvPixel &yuv) {
    int y_norm = yuv.y;
    int u_norm = yuv.u - 128;
    int v_norm = yuv.v - 128;

    int r_temp = y_norm + yuvPixel::YUV_TO_R_U * u_norm + yuvPixel::YUV_TO_R_V * v_norm;
    int g_temp = y_norm + yuvPixel::YUV_TO_G_U * u_norm + yuvPixel::YUV_TO_G_V * v_norm;
    int b_temp = y_norm + yuvPixel::YUV_TO_B_U * u_norm + yuvPixel::YUV_TO_B_V * v_norm;

    r = (unsigned char)(std::min(std::max(r_temp, 0), 255));
    g = (unsigned char)(std::min(std::max(g_temp, 0), 255));
    b = (unsigned char)(std::min(std::max(b_temp, 0), 255));
}

class Image {
  public:
    int getWidth() { return width; }
    int getHeight() { return height; }
    bool isGrayScale() { return is_grayscale; }

    void loadImage(const std::string &filename, const std::string &format) {}
    void saveImage(const std::string &filename, const std::string &format) {};
    void upsample(const int coefficient);
    void downsample(const int coefficient);
    void switchGrayScale() { is_grayscale = !is_grayscale; }

  private:
    int width;
    int height;
    bool is_grayscale;
    std::vector<rgbPixel> pixels;
};
