#ifndef RAYTRACER_SPHERE_H
#define RAYTRACER_SPHERE_H

#include <stdint.h>

#include "utils/vec3.h"

typedef struct {
    uint32_t materialIndex;
    Vec3 position;
    float radius;
} Sphere;

#endif //RAYTRACER_SPHERE_H
