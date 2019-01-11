#ifndef RAYTRACER_STRINGBUILDER_H
#define RAYTRACER_STRINGBUILDER_H

typedef struct {
	int capacity;
	int length;
	char* buffer;
} StringBuilder;

StringBuilder* stringbuilder_create(int intialCapacity);

// appends a string to the end of the internal buffer
void stringbuilder_append(StringBuilder* builder, const char* str);

// creates a copy of the internal buffer and returns it
// caller has to cleanup the memory of the returned cstr
const char* stringbuilder_cstr(StringBuilder* builder);

void stringbuilder_destroy(StringBuilder* builder);

#endif //RAYTRACER_STRINGBUILDER_H