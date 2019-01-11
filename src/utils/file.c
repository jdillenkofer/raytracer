#include "file.h"

#include <stdlib.h>

const char* file_readFile(const char* filepath, size_t* fileSize)
{
    FILE* file;
    char* data;
    size_t size;
    int err;

    file = fopen(filepath, "rb+");

    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    rewind(file);
    data = malloc(size + 1);
    fread(data, size, 1, file);
	data[size] = '\0';
    err = ferror(file);
    if (err != 0) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *fileSize = size + 1;
    return data;
}
