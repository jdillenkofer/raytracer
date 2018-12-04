#ifndef RAYTRACER_IMAGE_H
#define RAYTRACER_IMAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    uint32_t width, height;
    uint32_t* buffer; // Stores the data as rgba top to bottom, left to right
    uint32_t bufferSize;
} Image;

Image* image_create(uint32_t width, uint32_t height);
void image_destroy(Image* image);

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint32_t bfReserved;
    uint32_t bfOffBits;
} BitmapFileHeader;

typedef struct {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BitmapInfoHeader;
#pragma pack(pop)

bool bitmap_save_image(const char* path, Image* image);

#endif //RAYTRACER_IMAGE_H
