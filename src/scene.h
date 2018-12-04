#ifndef RAYTRACER_SCENE_H
#define RAYTRACER_SCENE_H

#include <stdint.h>

#include "material.h"
#include "plane.h"
#include "sphere.h"
#include "triangle.h"

typedef struct {
    uint32_t materialCount;
    Material *materials;

    uint32_t planeCount;
    Plane *planes;

    uint32_t sphereCount;
    Sphere *spheres;

    uint32_t triangleCount;
    Triangle *triangles;
} Scene;

#endif //RAYTRACER_SCENE_H
