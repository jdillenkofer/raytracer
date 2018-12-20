#include "object.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "utils/file.h"
#include "vertextable.h"

#define DEFAULT_CAPACITY 200

static void object_skipToNextWhitespace(char* data, size_t* offset) {
    char* dataPtr = &data[*offset];
    while(*dataPtr != '\0' && (*dataPtr != '\n' && *dataPtr != ' ' && *dataPtr != '\t')) {
        dataPtr = &data[++(*offset)];
    }
}

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
static float object_getFloat(char *data, size_t *offset) {
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
    return (float) atof(buff);
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
    return (uint32_t) atoi(buff);
}

static void object_parseVertex(VertexTable* vertexTable, char* data, size_t* offset) {
    Vec3 vertex;
    // skip v
    (*offset)++;
    object_skipWhitespace(data, offset);
    vertex.x = object_getFloat(data, offset);
    object_skipWhitespace(data, offset);
    vertex.y = object_getFloat(data, offset);
    object_skipWhitespace(data, offset);
    vertex.z = object_getFloat(data, offset);
    vertextable_addVertex(vertexTable, vertex);
}

static Object* object_createObject(void) {
    Object* object = malloc(sizeof(Object));
    object->triangleCount = 0;
    object->capacity = DEFAULT_CAPACITY;
    object->triangles= malloc(sizeof(Triangle) * object->capacity);
    return object;
}

static void object_addTriangle(Object* object, Triangle triangle) {
    if (object->capacity < object->triangleCount + 1) {
        object->capacity *= 2;
        object->triangles = realloc(object->triangles, sizeof(Triangle) * object->capacity);
    }
    object->triangles[object->triangleCount++] = triangle;
}

static void object_parseFace(Object* object, VertexTable* vertexTable, char* data, size_t* offset) {
    // skip f
    (*offset)++;
    object_skipWhitespace(data, offset);
    uint32_t v0Id = object_getInt(data, offset);
    object_skipToNextWhitespace(data, offset);
    object_skipWhitespace(data, offset);
    uint32_t v1Id = object_getInt(data, offset);
    object_skipToNextWhitespace(data, offset);
    object_skipWhitespace(data, offset);
    uint32_t v2Id = object_getInt(data, offset);
    object_skipToNextWhitespace(data, offset);
    Vec3 v0 = vertextable_getVertexById(vertexTable, v0Id);
    Vec3 v1 = vertextable_getVertexById(vertexTable, v1Id);
    Vec3 v2 = vertextable_getVertexById(vertexTable, v2Id);
    Triangle triangle;
    triangle.v0 = v0;
    triangle.v1 = v1;
    triangle.v2 = v2;
    triangle.materialIndex = 0;
    object_addTriangle(object, triangle);
}

Object* object_loadFromFile(const char* filepath) {
    Object* object = object_createObject();
    VertexTable* vertexTable = vertextable_create();

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
    vertextable_destroy(vertexTable);
    return object;
}

void object_scale(Object* object, float factor) {
    for (uint32_t i = 0; i < object->triangleCount; i++) {
        Triangle* triangle = &object->triangles[i];
        triangle->v0 = vec3_mul(triangle->v0, factor);
        triangle->v1 = vec3_mul(triangle->v1, factor);
        triangle->v2 = vec3_mul(triangle->v2, factor);
    }
}

void object_translate(Object* object, Vec3 translation) {
    for (uint32_t i = 0; i < object->triangleCount; i++) {
        Triangle* triangle = &object->triangles[i];
        triangle->v0 = vec3_add(triangle->v0, translation);
        triangle->v1 = vec3_add(triangle->v1, translation);
        triangle->v2 = vec3_add(triangle->v2, translation);
    }
}

void object_materialIndex(Object* object, uint32_t materialIndex) {
    for (uint32_t i = 0; i < object->triangleCount; i++) {
        Triangle* triangle = &object->triangles[i];
        triangle->materialIndex = materialIndex;
    }
}

void object_destroy(Object* object) {
    if (object) {
        free(object->triangles);
        free(object);
    }
}
