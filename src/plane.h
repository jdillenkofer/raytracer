#ifndef RAYTRACER_PLANE_H
#define RAYTRACER_PLANE_H

#include <stdint.h>

#include "utils/vec3.h"

typedef struct {
    uint32_t materialIndex;
    Vec3 normal;
    double distanceFromOrigin;
} Plane;

#endif //RAYTRACER_PLANE_H
