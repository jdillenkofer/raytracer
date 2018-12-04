#include <stdio.h>
#include "util/image.h"
#include "camera.h"

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    Vec3 camera_pos = { 0.0f , 0.0f, 100.0f };
    Vec3 up = { 0.0f, 1.0f, 0.0f };
    Vec3 lookAt = { 0.0f, 0.0f, 0.0f };
    Dimension renderDim = { 1280, 720 };
    double hFOV = 110.0f;
    Camera* camera = camera_create(camera_pos, up, lookAt, renderDim, hFOV);
    Image* image = image_create(renderDim.width, renderDim.height);

    for (int32_t y = 0; y < renderDim.height; y++) {
        for (int32_t x = 0; x < renderDim.width; x++) {
            Ray ray = camera_ray_from_pixel(camera, x, y);
            image->buffer[y*renderDim.width + x] = 0xFFFFFFFF;
        }
    }

    // bitmap_save_image("test.bmp", image);

    image_destroy(image);
    camera_destroy(camera);
    return 0;
}
