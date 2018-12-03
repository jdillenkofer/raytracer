#ifndef RAYTRACER_TRIANGLE_H
#define RAYTRACER_TRIANGLE_H

#include "util/vec3.h"
#include "util/color.h"

typedef struct {
    Vec3 v0, v1, v2;
    Color color;
} Triangle;

#endif //RAYTRACER_TRIANGLE_H
