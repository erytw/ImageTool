#include "compare.h"

int sqr(int a) noexcept { return a * a; }

double MSE(Image image1, Image image2, bool ignore_dimensions = false) {
    if (!ignore_dimensions &&
        (image1.getHeight() != image2.getHeight() || image1.getWidth() != image2.getWidth())) {
        throw std::invalid_argument("Images must be of the same size");
    }
    int width = std::min(image1.getWidth(), image2.getWidth());
    int height = std::min(image1.getHeight(), image2.getHeight());
    int dif_sum = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            dif_sum += sqr(image1.getPixel(x, y).r - image2.getPixel(x, y).r) +
                       sqr(image1.getPixel(x, y).g - image2.getPixel(x, y).g) +
                       sqr(image1.getPixel(x, y).b - image2.getPixel(x, y).b);
        }
    }
    return (double)dif_sum / (width * height * 3);
}

double psnr(double mse, int max_pixel_value) {
    if (mse == 0) {
        return 100.0;
    }
    return 10 * log10(sqr(max_pixel_value) / mse);
}
