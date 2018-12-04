#include "image.h"

#include <stdlib.h>

Image* image_create(int32_t width, int32_t height) {
    Image* image = malloc(sizeof(Image));
    if (image != NULL) {
        image->width = width;
        image->height = height;
        image->bufferSize = sizeof(uint32_t) * (uint32_t) abs(image->width * image->height);
        image->buffer = malloc(sizeof(uint32_t) * (uint32_t) abs(width * height));
        if (image->buffer == NULL) {
            free(image);
            image = NULL;
        }
    }
    return image;
}

void image_destroy(Image *image) {
    free(image->buffer);
    free(image);
}

bool bitmap_save_image(const char* path, Image* image) {
    BitmapFileHeader bitmapFileHeader = {0};
    bitmapFileHeader.bfType = 0x4d42; // ASCII "BM"
    bitmapFileHeader.bfSize = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + image->bufferSize;
    bitmapFileHeader.bfReserved = 0;
    bitmapFileHeader.bfOffBits = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

    BitmapInfoHeader bitmapInfoHeader = {0};
    bitmapInfoHeader.biSize = sizeof(BitmapInfoHeader);
    bitmapInfoHeader.biWidth = image->width;
    bitmapInfoHeader.biHeight = image->height;
    bitmapInfoHeader.biPlanes = 1;
    bitmapInfoHeader.biBitCount = 32;
    bitmapInfoHeader.biCompression = 0;
    bitmapInfoHeader.biSizeImage = image->bufferSize;
    bitmapInfoHeader.biXPelsPerMeter = 0;
    bitmapInfoHeader.biYPelsPerMeter = 0;
    bitmapInfoHeader.biClrUsed = 0;
    bitmapInfoHeader.biClrImportant = 0;

    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }

    fwrite(&bitmapFileHeader, sizeof(BitmapFileHeader), 1, file);
    fwrite(&bitmapInfoHeader, sizeof(BitmapInfoHeader), 1, file);
    uint32_t bytesPerRow = (uint32_t) image->width * sizeof(uint32_t);
    for (int32_t y = image->height; y > 0; y--) {
        fwrite(&image->buffer[y * image->width], bytesPerRow, 1, file);
    }
    fclose(file);

    return true;
}
