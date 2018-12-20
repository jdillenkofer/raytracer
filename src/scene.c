#include "scene.h"

#include <stdlib.h>

#define DEFAULT_CAPACITY 200

Scene* scene_create(void) {
    Scene* scene = malloc(sizeof(Scene));
    scene->materialCapacity = DEFAULT_CAPACITY;
    scene->materialCount = 0;
    scene->materials = malloc(sizeof(Material) * scene->materialCapacity);

    scene->planeCapacity = DEFAULT_CAPACITY;
    scene->planeCount = 0;
    scene->planes = malloc(sizeof(Plane) * scene->planeCapacity);

    scene->sphereCapacity = DEFAULT_CAPACITY;
    scene->sphereCount = 0;
    scene->spheres = malloc(sizeof(Sphere) * scene->sphereCapacity);

    scene->triangleCapacity = DEFAULT_CAPACITY;
    scene->triangleCount = 0;
    scene->triangles = malloc(sizeof(Triangle) * scene->triangleCapacity);

    scene->pointLightCapacity = DEFAULT_CAPACITY;
    scene->pointLightCount = 0;
    scene->pointLights = malloc(sizeof(PointLight) * scene->pointLightCapacity);

    return scene;
}

uint32_t scene_addMaterial(Scene* scene, Material material) {
    if (scene->materialCapacity < scene->materialCount + 1) {
        scene->materialCapacity *= 2;
        scene->materials = realloc(scene->materials, sizeof(Material) * scene->materialCapacity);
    }
    uint32_t materialId = scene->materialCount++;
    scene->materials[materialId] = material;
    return materialId;
}

void scene_addPlane(Scene* scene, Plane plane) {
    if (scene->planeCapacity < scene->planeCount + 1) {
        scene->planeCapacity *= 2;
        scene->planes = realloc(scene->planes, sizeof(Plane) * scene->planeCapacity);
    }
    scene->planes[scene->planeCount++] = plane;
}

void scene_addSphere(Scene* scene, Sphere sphere) {
    if (scene->sphereCapacity < scene->sphereCount + 1) {
        scene->sphereCapacity *= 2;
        scene->spheres = realloc(scene->spheres, sizeof(Sphere) * scene->sphereCapacity);
    }
    scene->spheres[scene->sphereCount++] = sphere;
}

void scene_addTriangle(Scene* scene, Triangle triangle) {
    if (scene->triangleCapacity < scene->triangleCount + 1) {
        scene->triangleCapacity *= 2;
        scene->triangles = realloc(scene->triangles, sizeof(Triangle) * scene->triangleCapacity);
    }
    scene->triangles[scene->triangleCount++] = triangle;
}

void scene_addObject(Scene* scene, Object object) {
    for (uint32_t i = 0; i < object.triangleCount; i++) {
        Triangle triangle = object.triangles[i];
        scene_addTriangle(scene, triangle);
    }
}

void scene_addPointLight(Scene* scene, PointLight pointLight) {
    if (scene->pointLightCapacity < scene->pointLightCount + 1) {
        scene->pointLightCapacity *= 2;
        scene->pointLights = realloc(scene->pointLights, sizeof(PointLight) * scene->triangleCapacity);
    }
    scene->pointLights[scene->pointLightCount++] = pointLight;
}

void scene_shrinkToFit(Scene *scene) {
    if (scene->materialCapacity > scene->materialCount) {
        scene->materials = realloc(scene->materials, sizeof(Material) * scene->materialCount);
        scene->materialCapacity = scene->materialCount;
    }
    if (scene->planeCapacity > scene->planeCount) {
        scene->planes = realloc(scene->planes, sizeof(Plane) * scene->planeCount);
        scene->planeCapacity = scene->planeCount;
    }
    if (scene->sphereCapacity > scene->sphereCount) {
        scene->spheres = realloc(scene->spheres, sizeof(Sphere) * scene->sphereCount);
        scene->sphereCapacity = scene->sphereCount;
    }
    if (scene->triangleCapacity > scene->triangleCount) {
        scene->triangles = realloc(scene->triangles, sizeof(Triangle) * scene->triangleCount);
        scene->triangleCapacity = scene->triangleCount;
    }
    if (scene->pointLightCapacity > scene->pointLightCount) {
        scene->pointLights = realloc(scene->pointLights, sizeof(PointLight) * scene->pointLightCount);
        scene->pointLightCapacity = scene->pointLightCount;
    }
}

void scene_destroy(Scene* scene) {
    if (scene) {
        camera_destroy(scene->camera);
        free(scene->materials);
        free(scene->planes);
        free(scene->spheres);
        free(scene->triangles);
        free(scene->pointLights);
        free(scene);
    }
}
