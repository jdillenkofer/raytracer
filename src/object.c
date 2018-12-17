#include "object.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "utils/file.h"

static void object_skipWhitespace(char* data, size_t* offset) {
    char* dataPtr = &data[*offset];
    while(*dataPtr != '\0' && (*dataPtr == '\n' || *dataPtr == ' ' || *dataPtr == '\t')) {
        dataPtr = &data[++(*offset)];
    }
}

static bool object_skipToNextLine(char* data, size_t* offset) {
    char* dataPtr = &data[*offset];
    while(*dataPtr != '\0' && *dataPtr != '\n') {
        dataPtr = &data[++(*offset)];
    }
    bool isEndOfStr = *dataPtr == '\0';
    if (!isEndOfStr) {
        ++(*offset);
    }
    return !isEndOfStr;
}

#define LOCAL_NUM_BUFFERSIZE 255
static double object_getDouble(char *data, size_t *offset) {
    static char buff[LOCAL_NUM_BUFFERSIZE];
    size_t startOffset = *offset;
    char* dataPtr = &data[*offset];
    while(*dataPtr != '\0' &&
        ((*dataPtr >= 48 && *dataPtr <= 57) ||
        *dataPtr == '.' || *dataPtr == '-' || *dataPtr == 'e')) {
        dataPtr = &data[++(*offset)];
    }
    size_t length = *offset - startOffset;
    if (length > LOCAL_NUM_BUFFERSIZE - 1) {
        length = LOCAL_NUM_BUFFERSIZE - 1;
    }
    memcpy(buff, &data[startOffset], length);
    buff[length] = '\0';
    return atof(buff);
}

static uint32_t object_getInt(char *data, size_t *offset) {
    static char buff[LOCAL_NUM_BUFFERSIZE];
    size_t startOffset = *offset;
    char* dataPtr = &data[*offset];
    while(*dataPtr != '\0' && (*dataPtr >= 48 && *dataPtr <= 57)) {
        dataPtr = &data[++(*offset)];
    }
    size_t length = *offset - startOffset;
    if (length > LOCAL_NUM_BUFFERSIZE - 1) {
        length = LOCAL_NUM_BUFFERSIZE - 1;
    }
    memcpy(buff, &data[startOffset], length);
    buff[length] = '\0';
    return atoi(buff);
}

typedef struct {
    Vec3* vertices;
    size_t elements;
    size_t space;
} VertexTable;

static VertexTable* object_createVertexTable() {
    VertexTable* vertexTable = malloc(sizeof(VertexTable));
    vertexTable->elements = 0;
    vertexTable->space = 200;
    vertexTable->vertices = malloc(sizeof(Vec3) * vertexTable->space);
    return vertexTable;
}

static void object_addVertex(VertexTable* vertexTable, Vec3 vertex) {
    if (vertexTable->space < vertexTable->elements + 1) {
        vertexTable->space *= 2;
        vertexTable->vertices = realloc(vertexTable->vertices, sizeof(Vec3) * vertexTable->space);
    }
    vertexTable->vertices[vertexTable->elements++] = vertex;
}

static Vec3 object_getVertexById(VertexTable* vertexTable, uint32_t id) {
    assert(id > 0 && id <= vertexTable->elements);
    return vertexTable->vertices[id - 1];
}

static void object_destroyVertexTable(VertexTable* vertexTable) {
    if (vertexTable != NULL) {
        free(vertexTable->vertices);
        free(vertexTable);
    }
}

static void object_parseVertex(VertexTable* vertexTable, char* data, size_t* offset) {
    Vec3 vertex;
    // skip v
    (*offset)++;
    object_skipWhitespace(data, offset);
    vertex.x = object_getDouble(data, offset);
    object_skipWhitespace(data, offset);
    vertex.y = object_getDouble(data, offset);
    object_skipWhitespace(data, offset);
    vertex.z = object_getDouble(data, offset);
    object_addVertex(vertexTable, vertex);
}

static Object* object_createObject() {
    Object* object = malloc(sizeof(Object));
    object->triangleCount = 0;
    object->space = 200;
    object->triangles= malloc(sizeof(Triangle) * object->space);
    return object;
}

static void object_addTriangle(Object* object, Triangle triangle) {
    if (object->space < object->triangleCount + 1) {
        object->space *= 2;
        object->triangles = realloc(object->triangles, sizeof(Triangle) * object->space);
    }
    object->triangles[object->triangleCount++] = triangle;
}

static void object_parseFace(Object* object, VertexTable* vertexTable, char* data, size_t* offset) {
    // skip f
    (*offset)++;
    object_skipWhitespace(data, offset);
    uint32_t v0Id = object_getInt(data, offset);
    object_skipWhitespace(data, offset);
    uint32_t v1Id = object_getInt(data, offset);
    object_skipWhitespace(data, offset);
    uint32_t v2Id = object_getInt(data, offset);
    Vec3 v0 = object_getVertexById(vertexTable, v0Id);
    Vec3 v1 = object_getVertexById(vertexTable, v1Id);
    Vec3 v2 = object_getVertexById(vertexTable, v2Id);
    Triangle triangle;
    triangle.v0 = v0;
    triangle.v1 = v1;
    triangle.v2 = v2;
    triangle.materialIndex = 2;
    object_addTriangle(object, triangle);
}

Object* object_loadFromFile(const char* filepath) {
    Object* object = object_createObject();
    VertexTable* vertexTable = object_createVertexTable();

    size_t fileSize;
    char* data = file_readFile(filepath, &fileSize);
    size_t offset = 0;
    do {
        object_skipWhitespace(data, &offset);
        switch(data[offset]) {
            case 'v':
            {
                // vn
                if (data[offset+1] == 'n') {
                    continue;
                }
                // v
                object_parseVertex(vertexTable, data, &offset);
                break;
            }
            case 'f':
            {
                object_parseFace(object, vertexTable, data, &offset);
                break;
            }
            case '#':
            case 'g':
            default:
                break;
        }
    } while (object_skipToNextLine(data, &offset));

    free(data);
    object_destroyVertexTable(vertexTable);
    return object;
}

void object_destroy(Object* object) {
    if (object) {
        free(object->triangles);
        free(object);
    }
}
