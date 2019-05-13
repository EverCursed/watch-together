#ifndef WT_UTILS
#define WT_UTILS

#define ArrayCount(x) (sizeof(x)/sizeof(x[0]))


#define BYTES_ALIGNMENT 16

uint32 round_up_align(uint32 val)
{
    register uint32 temp = (val + (BYTES_ALIGNMENT-1));
    return temp - (temp % BYTES_ALIGNMENT);
    //return val + (BYTES_ALIGNMENT - (val % BYTES_ALIGNMENT));
}

#endif