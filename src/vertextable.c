#include "vertextable.h"

VertexTable* vertextable_create() {
    VertexTable* vertexTable = malloc(sizeof(VertexTable));
    vertexTable->size = 0;
    vertexTable->capacity = 200;
    vertexTable->vertices = malloc(sizeof(Vec3) * vertexTable->capacity);
    return vertexTable;
}

void vertextable_addVertex(VertexTable *vertexTable, Vec3 vertex) {
    if (vertexTable->capacity < vertexTable->size + 1) {
        vertexTable->capacity *= 2;
        vertexTable->vertices = realloc(vertexTable->vertices, sizeof(Vec3) * vertexTable->capacity);
    }
    vertexTable->vertices[vertexTable->size++] = vertex;
}

Vec3 vertextable_getVertexById(VertexTable *vertexTable, uint32_t id) {
    assert(id > 0 && id <= vertexTable->size);
    return vertexTable->vertices[id - 1];
}

void vertextable_destroy(VertexTable *vertexTable) {
    if (vertexTable != NULL) {
        free(vertexTable->vertices);
        free(vertexTable);
    }
}
