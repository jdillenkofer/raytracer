#include "file.h"

#include <stdlib.h>

void* file_readFile(const char* filepath, size_t* fileSize)
{
    FILE* file;
    void* data;
    size_t size;
    int err;

    file = fopen(filepath, "rb+");

    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    rewind(file);
    data = malloc(size);
    fread(data, size, 1, file);
    err = ferror(file);
    if (err != 0) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *fileSize = size;
    return data;
}
