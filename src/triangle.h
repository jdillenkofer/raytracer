#ifndef RAYTRACER_TRIANGLE_H
#define RAYTRACER_TRIANGLE_H

#include <stdint.h>

#include "utils/vec3.h"

typedef struct {
    uint32_t materialIndex;
    Vec3 v0, v1, v2;
} Triangle;

#endif //RAYTRACER_TRIANGLE_H
