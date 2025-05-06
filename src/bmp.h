#include <cstdint>

#pragma pack(push, 1)

struct BMPHeader {
    uint16_t signature;
    uint32_t fileSize;
    uint32_t reserved;
    uint32_t dataOffset;
    BMPHeader() = default;
    BMPHeader(int width, int height);
};

struct BMPInfoHeader {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xPixelsPerM;
    int32_t yPixelsPerM;
    uint32_t colorsUsed;
    uint32_t importantColors;
    BMPInfoHeader() = default;
    BMPInfoHeader(int width, int height);
};

#pragma pack(pop)
