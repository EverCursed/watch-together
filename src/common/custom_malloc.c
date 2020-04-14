/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

A wrapping for malloc to more easily detect buffer overflows.
*/


#include "custom_malloc.h"

#include "../defines.h"
#include <stdlib.h>
#include "../debug.h"

#define MAX_ALLOCS 256

static void* generated_allocations[MAX_ALLOCS];
static void* allocations[MAX_ALLOCS];
static int   sizes[MAX_ALLOCS];
static char* files[MAX_ALLOCS];
static int lines[MAX_ALLOCS];

static int  allocations_count = 0;
static char* canary = "END";
static const int canary_size = 4;

internal void *
c_malloc(size_t size, char *file, int line)
{
    assert(allocations_count != MAX_ALLOCS);
    
    void* mem;
    if(!(mem = malloc(size + canary_size))) return NULL;
    
    *((int *)(mem + size)) = *((int *)(canary));
    
    int i = allocations_count++;
    allocations[i] = mem;
    generated_allocations[i] = mem;
    sizes[i] = size;
    files[i] = file;
    lines[i] = line;
    
    return mem;
}

internal void *
c_malloc_expanded(size_t size, i32 round_up_to, char *file, int line)
{
    void *mem = c_malloc(size + round_up_to, file, line);
    
    void *mem_offset = (mem+round_up_to-1) - (((u64)mem + (round_up_to-1))%round_up_to);
    generated_allocations[allocations_count - 1] = mem_offset;
    
    return mem_offset;
}

internal void *
c_realloc(void *ptr, size_t size, char *file, int line)
{
    for(int i = 0; i < allocations_count; i++)
    {
        if(generated_allocations[i] == ptr)
        {
            generated_allocations[i] = allocations[i] = realloc(ptr, size+canary_size);
            
            sizes[i] = size;
            
            *((int *)(allocations[i] + size)) = *((int *)(canary));
            
            dbg_success("Reallocated memory (%s:%d)\n", files[i], lines[i]);
            
            files[i] = file;
            lines[i] = line;
            
            return allocations[i];
        }
    }
    
    return NULL;
}

internal void *
c_realloc_expanded(void *ptr, size_t size, i32 alignment, char *file, int line)
{
    for(int i = 0; i < allocations_count; i++)
    {
        if(size > sizes[i] && generated_allocations[i] == ptr)
        {
            void *remove = allocations[i];
            void *old_mem = generated_allocations[i];
            int old_size = sizes[i];
            sizes[i] = size;
            
            allocations[i] = malloc(size+alignment+canary_size);
            generated_allocations[i] = (void *)(((u64)allocations[i] + (alignment - 1)) & (~(alignment - 1)));
            
            memcpy(generated_allocations[i], old_mem, old_size);
            free(remove);
            
            *((int *)(allocations[i] + size)) = *((int *)(canary));
            
            dbg_success("Reallocated memory (%s:%d)\n", files[i], lines[i]);
            
            files[i] = file;
            lines[i] = line;
            
            return generated_allocations[i];
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
            allocations_count--;
        }
    }
}
