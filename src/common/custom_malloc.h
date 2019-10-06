#ifndef CUSTOM_MALLOC_H
#define CUSTOM_MALLOC_H

#include "../defines.h"

#define custom_malloc(size) c_malloc((size), __FILE__, __LINE__)
#define custom_realloc(ptr, size) c_realloc(ptr, size)

void* c_malloc(int32, char *, int);
void* c_realloc(void *, int);
void assert_memory_bounds();

#endif