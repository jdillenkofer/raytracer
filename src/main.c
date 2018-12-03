#include <stdio.h>
#include "camera.h"

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    Vec3 camera_pos = { 0.0f , 0.0f, 100.0f };
    Vec3 up = { 0.0f, 1.0f, 0.0f };
    Vec3 lookAt = { 0.0f, 0.0f, 0.0f };
    Dimension renderDim = { 800, 600 };
    double hFOV = 110.0f;
    Camera* camera = camera_create(camera_pos, up, lookAt, renderDim, hFOV);

    for (int y = 0; y < renderDim.height; y++) {
        for (int x = 0; x < renderDim.width; x++) {
            Ray ray = camera_get_ray_for_pixel(camera, x, y);
        }
    }

    camera_destroy(camera);
    return 0;
}
