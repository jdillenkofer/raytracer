#ifndef RAYTRACER_OBJECT_H
#define RAYTRACER_OBJECT_H

#include <stdint.h>

#include "triangle.h"

typedef struct {
    uint32_t capacity;
    uint32_t triangleCount;
    Triangle* triangles;
} Object;

Object* object_loadFromFile(const char* filepath);
void object_scale(Object* object, float factor);
void object_translate(Object* object, Vec3 translation);
void object_materialIndex(Object* object, uint32_t materialIndex);
void object_destroy(Object* object);

#endif //RAYTRACER_OBJECT_H
