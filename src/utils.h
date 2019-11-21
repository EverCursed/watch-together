#ifndef WT_UTILS
#define WT_UTILS

#define ArrayCount(x) (sizeof(x)/sizeof(x[0]))

#define BYTES(x)   (x)
#define KB(x)      (1024*x)
#define MB(x)      (1024*KB(x))
#define GB(x)      (1024*MB(x))
#define TB(x)      (1024*GB(x))
#define PB(x)      (1024*TB(x))

#define BYTES_ALIGNMENT 16

#define round_up_align(v) \
((v + BYTES_ALIGNMENT -1) - (((v + BYTES_ALIGNMENT - 1) % BYTES_ALIGNMENT)))

/*
static uint32 round_up_align(uint32 val)
{
    return (val + BYTES_ALIGNMENT - 1)  - (((val + BYTES_ALIGNMENT - 1) % BYTES_ALIGNMENT));
}
*/

#define smallest(a, b) (a < b ? a : b)
#define largest(a,b) (a > b ? a : b)

#define SDL_PrintError() dbg_error("%s\n", SDL_GetError())
#define SDL_Test(ret) do { if(ret) SDL_PrintError(); } while(0)
#define FF_PrintError(ret) av_err2str(ret)
#define FF_Test(ret) do { if(ret) FF_PrintError(ret); } while(0)

#define b2str(v) (v ? "true" : "false")

#endif
