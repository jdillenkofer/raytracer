#ifndef RAYTRACER_VERTEXTABLE_H
#define RAYTRACER_VERTEXTABLE_H

#include "utils/vec3.h"
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    Vec3* vertices;
    uint32_t size;
    uint32_t capacity;
} VertexTable;

VertexTable* vertextable_create(void);
void vertextable_addVertex(VertexTable *vertexTable, Vec3 vertex);
Vec3 vertextable_getVertexById(VertexTable *vertexTable, uint32_t id);
void vertextable_destroy(VertexTable *vertexTable);

#endif //RAYTRACER_VERTEXTABLE_H
