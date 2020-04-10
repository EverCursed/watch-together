/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

A wrapping for malloc to more easily detect buffer overflows.
*/

#include "custom_malloc.h"

#include <stdlib.h>
#include "../debug.h"

#define MAX_ALLOCS 256

static void* allocations[MAX_ALLOCS];
static int   sizes[MAX_ALLOCS];
static char* files[MAX_ALLOCS];
static int lines[MAX_ALLOCS];

static char* canary = "END";
static const int canary_size = 4;

internal void *
c_malloc(size_t size, char *file, int line)
{
    void* mem = malloc(size + canary_size);
    *((int *)(mem + size)) = *((int *)(canary));
    
    for(int i = 0; i < MAX_ALLOCS; i++)
    {
        if(allocations[i] == NULL)
        {
            allocations[i] = mem;
            sizes[i] = size;
            files[i] = file;
            lines[i] = line;
            
            return mem;
        }
    }
    
    return NULL;
}

internal void *
c_malloc_expanded(size_t size, int round_up_to, char *file, int line)
{
    
    int size_rounded = (size + (round_up_to-1)) & ~(round_up_to-1);
    return c_malloc(size_rounded, file, line);
}

internal void *
c_realloc(void *ptr, size_t size)
{
    for(int i = 0; i < MAX_ALLOCS; i++)
    {
        if(allocations[i] == ptr)
        {
            allocations[i] = realloc(ptr, size+canary_size);
            sizes[i] = size;
            
            *((int *)(allocations[i] + size)) = *((int *)(canary));
            
            dbg_success("Reallocated memory (%s:%d)\n", files[i], lines[i]);
            
            return allocations[i];
        }
    }
    
    return NULL;
}

internal void
assert_memory_bounds()
{
    for(int i = 0; i < MAX_ALLOCS; i++)
    {
        if(allocations[i] != NULL &&
           (*((int *)(allocations[i] + sizes[i])) != *((int *)(canary))))
        {
            dbg_error("Memory constraints violated (%s:%d)\n", files[i], lines[i]);
        }
    }
}

internal void
c_free(void *mem)
{
    if(unlikely(!mem)) return;
    
    for(int i = 0; i < MAX_ALLOCS; i++)
    {
        if(allocations[i] == mem)
        {
            sizes[i] = lines[i] = 0;
            allocations[i] = files[i] = NULL;
            free(mem);
        }
    }
}
