#ifndef CUSTOM_MALLOC_H
#define CUSTOM_MALLOC_H

#include "../defines.h"

#ifdef DEBUG

#define custom_malloc(size) c_malloc((size), __FILE__, __LINE__)
#define custom_realloc(ptr, size) c_realloc(ptr, size)
#define custom_free(ptr) c_free(ptr)

#else // DEBUG

#define custom_malloc(size) malloc(size)
#define custom_realloc(ptr, size) realloc(ptr, size)
#define custom_free(ptr) free(ptr)

#endif

// NOTE(Val): DO NOT USE THESE DIRECTLY
void* c_malloc(int32, char *, int);
void* c_realloc(void *, int);
void assert_memory_bounds();
void c_free(void *);
// ------------------------------------

#endif