#ifndef RAYTRACER_STRINGBUILDER_H
#define RAYTRACER_STRINGBUILDER_H

typedef struct {
	size_t capacity;
	size_t length;
	char* buffer;
} StringBuilder;

StringBuilder* stringbuilder_create(size_t intialCapacity);

// appends a string to the end of the internal buffer
void stringbuilder_append(StringBuilder* builder, const char* str);

// creates a copy of the internal buffer and returns it
// caller has to cleanup the memory of the returned cstr
const char* stringbuilder_cstr(StringBuilder* builder);

void stringbuilder_destroy(StringBuilder* builder);

#endif //RAYTRACER_STRINGBUILDER_H