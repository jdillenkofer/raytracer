#include "object.h"

#include <stdlib.h>

#include "utils/file.h"

Object* object_loadFromFile(const char* filepath) {
    size_t fileSize;
    void* data = file_readFile(filepath, &fileSize);
    free(data);
    Object* object = malloc(sizeof(Object));
    return object;
}

void object_destroy(Object* object) {
    free(object);
}
