#pragma once
#include "image.h"

double MSE(Image image1, Image image2, bool ignore_dimensions);
double psnr(double mse, int max_pixel_value);
