#ifndef RAYTRACER_CAMERA_H
#define RAYTRACER_CAMERA_H

#include "util/vec3.h"

typedef struct {
    Vec3 position;
    Vec3 up;
    Vec3 lookAt;
    Vec3 n, u, v;
    double renderWidth, renderHeight;
} Camera;

Camera* camera_create(Vec3 position, Vec3 up, Vec3 lookAt, double renderWidth, double renderHeight);
static void camera_calcEyeCoordinateSystem(Camera *camera);
void camera_destroy(Camera* camera);

#endif //RAYTRACER_CAMERA_H
