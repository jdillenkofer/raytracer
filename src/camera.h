#ifndef RAYTRACER_CAMERA_H
#define RAYTRACER_CAMERA_H

#include "utils/vec3.h"
#include "ray.h"

typedef struct {
    Vec3 position;
    Vec3 x, y, z;
    Vec3 lookAt;
    uint32_t width, height;
    double FOV, aspectRatio;
    Vec3 renderTargetCenter;
    double renderTargetWidth, renderTargetHeight, renderTargetDistance;
} Camera;

Camera* camera_create(Vec3 position, Vec3 lookAt, uint32_t width, uint32_t height, double FOV);
Ray camera_ray_from_pixel(Camera *cam, uint32_t x, uint32_t y);
void camera_destroy(Camera* camera);

#endif //RAYTRACER_CAMERA_H
