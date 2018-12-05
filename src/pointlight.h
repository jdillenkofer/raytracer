#ifndef RAYTRACER_POINTLIGHT_H
#define RAYTRACER_POINTLIGHT_H

#include "utils/vec3.h"

typedef struct {
    Vec3 position;
    Vec3 emissionColor;
} PointLight;

#endif //RAYTRACER_POINTLIGHT_H
