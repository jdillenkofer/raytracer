#include "stringbuilder.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

StringBuilder* stringbuilder_create(int intialCapacity) {
	if (intialCapacity < 1) {
		intialCapacity = 1;
	}
	StringBuilder* builder = malloc(sizeof(StringBuilder));
	if (!builder) {
		return NULL;
	}
	builder->capacity = intialCapacity;
	builder->buffer = malloc(sizeof(char) * intialCapacity);
	if (!builder->buffer) {
		free(builder);
		return NULL;
	}
	builder->buffer[0] = '\0';
	builder->length = 1;
	return builder;
}

void stringbuilder_append(StringBuilder* builder, const char* str) {
	size_t addedLength = strlen(str);
	if (builder->length + addedLength > builder->capacity) {
		builder->capacity = (builder->length + addedLength) + builder->capacity;
		builder->buffer = realloc(builder->buffer, builder->capacity);
	}
	memcpy(&builder->buffer[builder->length - 1], str, addedLength + 1);
	builder->length += addedLength;
}

const char* stringbuilder_cstr(StringBuilder* builder) {
	char* retStr = malloc(sizeof(char) * builder->length);
	memcpy(retStr, builder->buffer, builder->length);
	return retStr;
}

void stringbuilder_destroy(StringBuilder* builder) {
	if (builder) {
		free(builder->buffer);
		free(builder);
	}
}