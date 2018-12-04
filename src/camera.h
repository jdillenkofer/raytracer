#ifndef RAYTRACER_CAMERA_H
#define RAYTRACER_CAMERA_H

#include "util/vec3.h"
#include "util/dimension.h"
#include "ray.h"

typedef struct {
    Vec3 position;
    Vec3 up;
    Vec3 lookAt;
    Vec3 n, u, v;
    Dimension renderDim;
    double hFOV, distanceToRenderTarget;
    Vec3 cPosition, lPosition;
} Camera;

Camera* camera_create(Vec3 position, Vec3 up, Vec3 lookAt, Dimension renderDim, double hFOV);
Ray camera_ray_from_pixel(Camera *cam, int32_t x, int32_t y);
void camera_destroy(Camera* camera);

#endif //RAYTRACER_CAMERA_H
