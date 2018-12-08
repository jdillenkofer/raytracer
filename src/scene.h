#ifndef RAYTRACER_SCENE_H
#define RAYTRACER_SCENE_H

#include <stdint.h>

#include "pointlight.h"
#include "material.h"
#include "plane.h"
#include "sphere.h"
#include "triangle.h"
#include "camera.h"

typedef struct {
    Camera* camera;

    uint32_t materialCount;
    Material *materials;

    uint32_t planeCount;
    Plane *planes;

    uint32_t sphereCount;
    Sphere *spheres;

    uint32_t triangleCount;
    Triangle *triangles;

    uint32_t pointLightCount;
    PointLight* pointLights;
} Scene;

#endif //RAYTRACER_SCENE_H
