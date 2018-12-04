#ifndef RAYTRACER_CAMERA_H
#define RAYTRACER_CAMERA_H

#include "utils/vec3.h"
#include "utils/dimension.h"
#include "ray.h"

typedef struct {
    Vec3 position;
    Vec3 x, y, z;
    Vec3 up;
    Vec3 lookAt;
    Dimension renderDim;
    double hFOV, distanceToRenderTarget;
    Vec3 cPosition, lPosition;
} Camera;

Camera* camera_create(Vec3 position, Vec3 up, Vec3 lookAt, Dimension renderDim, double hFOV);
Ray camera_ray_from_pixel(Camera *cam, uint32_t x, uint32_t y);
void camera_destroy(Camera* camera);

#endif //RAYTRACER_CAMERA_H
