/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

Malloc wrapper header.
*/

#ifndef CUSTOM_MALLOC_H
#define CUSTOM_MALLOC_H

#include "../defines.h"
#include <immintrin.h>

#ifdef DEBUG

#define custom_malloc(size) c_malloc((size), __FILE__, __LINE__)
#define custom_realloc(ptr, size) c_realloc(ptr, size, __FILE__, __LINE__)
#define custom_free(ptr) c_free(ptr)
#define custom_malloc_aligned(size, alignment) \
c_malloc_expanded(size, alignment, __FILE__, __LINE__)
#define custom_realloc_aligned(ptr, size, alignment) \
c_realloc_expanded(ptr, size, alignment, __FILE__, __LINE__)

internal void assert_memory_bounds();

// NOTE(Val): DO NOT USE THESE DIRECTLY
internal void* c_malloc(size_t size, char *file, int line);
internal void* c_realloc(void *ptr, size_t size, char *file, int line);
internal void c_free(void *ptr);
internal void* c_malloc_expanded(size_t size, i32 alignment, char *file, int line);
internal void* c_realloc_expanded(void *ptr, size_t size, i32 alignment, char *file, int line);
// ------------------------------------

#else // DEBUG


#define custom_malloc(size) malloc(size)
#define custom_realloc(ptr, size) realloc(ptr, size)
#define custom_free(ptr) free(ptr)
#define custom_malloc_aligned(size, alignment) assert(0)

// TODO(Val): Implement this when I do arena allocations


#endif //DEBUG

#endif