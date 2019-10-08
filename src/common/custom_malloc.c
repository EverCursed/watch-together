#include <stdlib.h>
#include "../debug.h"

#define MAX_ALLOCS 256

static void* allocations[MAX_ALLOCS];
static int   sizes[MAX_ALLOCS];
static char* files[MAX_ALLOCS];
static int lines[MAX_ALLOCS];

static char* canary = "END";
static const int canary_size = 4;

void *
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

void *
c_realloc(void *ptr, size_t size)
{
    for(int i = 0; i < MAX_ALLOCS; i++)
    {
        if(allocations[i] == ptr)
        {
            allocations[i] = realloc(ptr, size);
            sizes[i] = size;
            
            dbg_success("Reallocated memory (%s:%d)\n", files[i], lines[i]);
            return allocations[i];
        }
    }
    
    return NULL;
}

void
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

void
custom_free(void *mem)
{
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
