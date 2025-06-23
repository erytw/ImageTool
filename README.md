# Image format converter in C++

Supports BMP, YUV420P, YUV422P, YUV444P for both input and output.
Uses MSE and PSNR to compare images.
Multiple upscaling methods including traditional and AI-based techniques from opencv2.

#### Building:

```bash
# Basic build (traditional upscaling only)
clang++ src/main.cpp src/image.cpp src/compare.cpp src/upscaler.cpp -o imageTool -O3 -std=c++17

# With OpenCV
clang++ src/main.cpp src/image.cpp src/compare.cpp src/upscaler.cpp -o imageTool -O3 -std=c++17 `pkg-config --cflags --libs opencv4`

# Comparison tool (requires OpenCV)
clang++ src/upscale_comparison.cpp src/image.cpp src/compare.cpp src/upscaler.cpp -o upscale_comparison -O3 -std=c++17 `pkg-config --cflags --libs opencv4`
```

#### Usage:

```bash
./imageTool --input *filename* [--width *width*] [--height *height*] --output *filename* --input-format *format* --output-format *format* [--compare-results] [--grayscale] [--downsample *coefficient*] [--upsample *coefficient*] [--upscale-method *method*] [--scale-factor *factor*] [--model-path *path*] [--ignore-dimensions]
```

or:

```bash
./imageTool --compare *filename* *filename* --input-format *format* [--ignore-dimensions]
```

or upscaling comparison:

```bash
./upscale_comparison *input_file* *input_format* *scale_factor* [model_directory]
```

#### Advanced Upscaling Options:

- `--upscale-method`: Choose upscaling method (BICUBIC, LANCZOS, BTVL1, ESPCN, EDSR, FSRCNN, LAPSRN)
- `--scale-factor`: Upscaling factor (2, 3, 4, or 8 depending on method)
- `--model-path`: Path to AI model file (required for AI methods)

#### Examples:

```bash
./imageTool --input input.bmp --output output.bmp --input-format BMP --output-format BMP --upscale-method BICUBIC --scale-factor 2

./imageTool --input input.bmp --output output.bmp --input-format BMP --output-format BMP --upscale-method ESPCN --scale-factor 2 --model-path models/ESPCN_x2.pb

./imageTool --input input.bmp --output output.bmp --input-format BMP --output-format BMP --upscale-method EDSR --scale-factor 4 --model-path models/EDSR_x4.pb --compare-results

./upscale_comparison input.bmp BMP 2 models/
```
