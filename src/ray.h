#ifndef RAYTRACER_RAY_H
#define RAYTRACER_RAY_H

#include <util/vec3.h>
#include <stdint.h>

typedef struct {
    Vec3 origin;
    Vec3 direction;
} Ray;

#endif //RAYTRACER_RAY_H
