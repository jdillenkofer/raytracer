#ifndef RAYTRACER_SCENE_H
#define RAYTRACER_SCENE_H

#include <stdint.h>

#include "pointlight.h"
#include "material.h"
#include "plane.h"
#include "sphere.h"
#include "triangle.h"
#include "camera.h"
#include "object.h"

typedef struct {
    Camera* camera;

    uint32_t materialCapacity;
    uint32_t materialCount;
    Material *materials;

    uint32_t planeCapacity;
    uint32_t planeCount;
    Plane *planes;

    uint32_t sphereCapacity;
    uint32_t sphereCount;
    Sphere *spheres;

    uint32_t triangleCapacity;
    uint32_t triangleCount;
    Triangle *triangles;

    uint32_t pointLightCapacity;
    uint32_t pointLightCount;
    PointLight* pointLights;
} Scene;

Scene* scene_create(void);
Scene* scene_init(uint32_t width, uint32_t height);
// adds the material to the scene and returns the materialId
uint32_t scene_addMaterial(Scene* scene, Material material);
void scene_addPlane(Scene* scene, Plane plane);
void scene_addSphere(Scene* scene, Sphere sphere);
void scene_addTriangle(Scene* scene, Triangle triangle);
void scene_addObject(Scene* scene, Object object);
void scene_addPointLight(Scene* scene, PointLight pointLight);
void scene_shrinkToFit(Scene *scene);
void scene_destroy(Scene* scene);

#endif //RAYTRACER_SCENE_H
