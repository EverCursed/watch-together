#ifndef WT_UTILS
#define WT_UTILS

#define ArrayCount(x) (sizeof(x)/sizeof(x[0]))


#define BYTES_ALIGNMENT 16

#define round_up_align(v) \
((v + BYTES_ALIGNMENT -1) - (((v + BYTES_ALIGNMENT - 1) % BYTES_ALIGNMENT)))

/*
static uint32 round_up_align(uint32 val)
{
    return (val + BYTES_ALIGNMENT - 1)  - (((val + BYTES_ALIGNMENT - 1) % BYTES_ALIGNMENT));
}
*/
#define MS(real) (real/1000.0)

#define smallest(a, b) (a < b ? a : b)

#endif