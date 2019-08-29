#ifndef TIMING_H
#define TIMING_H

#ifdef DEBUG

#ifdef _WIN32
#include "windows.h"
#endif

#include "defines.h"
#include "platform.h"


#define DEBUG_LINE_WIDTH     256
#define MAX_EVENTS          8192
#define MAX_DEPTH             64


struct platform_timing_data {
#ifdef _WIN32
    uint64 freq;
#elif defined(__linux__)
    
#endif
};

typedef struct _timing_instance {
    char   *name;
    real64  time;
} timing_instance;

struct _timing_queue {
    timing_instance q[MAX_DEPTH];
    int32           n;
};

struct _timing_data {
    timing_instance *inst;
    char *dump;
    real64 start_time;
    int32 dump_length;
    int32 n;
    int32 max_marks;
    struct platform_timing_data dat;
};

extern _Thread_local struct _timing_data __dbgtimdat;

#if defined( _WIN32)
#define InitPlatformTimingData \
do { \
    QueryPerformanceFrequency((LARGE_INTEGER *)&__dbgtimdat.dat.freq); \
} while(0)

#define GetHighPrecisionTime(ptr) \
do { \
    uint64 time; \
    QueryPerformanceCounter((LARGE_INTEGER *)&time); \
    *ptr = ((real64)time / (real64)__dbgtimdat.dat.freq) - __dbgtimdat.start_time; \
} while(0)
#elif defined(__linux__)

#endif

#define InitializeTimingSystem \
do { \
    InitPlatformTimingData; \
    \
    __dbgtimdat.inst = malloc(sizeof(timing_instance) * MAX_EVENTS); \
    __dbgtimdat.dump = malloc(MAX_EVENTS * DEBUG_LINE_WIDTH); \
    __dbgtimdat.start_time = 0; \
    GetHighPrecisionTime(&__dbgtimdat.start_time); \
    __dbgtimdat.n = 0; \
    __dbgtimdat.max_marks = MAX_EVENTS; \
    \
} while(0)

#define StartTimer(name_c) \
do { \
    __dbgtimdat.inst[__dbgtimdat.n].name = name_c; \
    GetHighPrecisionTime(&__dbgtimdat.inst[__dbgtimdat.n].time); \
    __dbgtimdat.n++; \
} while(0)

#define EndTimer \
do { \
    __dbgtimdat.inst[__dbgtimdat.n].name = NULL; \
    GetHighPrecisionTime(&__dbgtimdat.inst[__dbgtimdat.n].time); \
    __dbgtimdat.n++; \
} while(0)


#define DumpTimingFrame \
do { \
    int32 n = 0; \
    struct _timing_queue queue = {}; \
    for(int i = 0; i < __dbgtimdat.n; i++) \
    { \
        if(__dbgtimdat.inst[i].name != NULL) \
        { \
            queue.q[queue.n] = __dbgtimdat.inst[i]; \
            for(int d = 0; d < queue.n; d++) \
            { \
                n += sprintf((__dbgtimdat.dump + n), "\t"); \
            } \
            n += sprintf((__dbgtimdat.dump + n), "%s    START\n", queue.q[queue.n].name); \
            queue.n++; \
        } \
        else \
        { \
            real64 time_passed = __dbgtimdat.inst[i].time - queue.q[queue.n-1].time; \
            for(int d = 0; d < queue.n-1; d++) \
            { \
                n += sprintf((__dbgtimdat.dump + n), "\t"); \
            } \
            n += sprintf((__dbgtimdat.dump + n), "%s    END %lf\n", queue.q[queue.n-1].name, time_passed); \
            queue.n--; \
        } \
    } \
    __dbgtimdat.dump_length += n; \
} while(0)


#define FinishTiming \
do { \
    DumpTimingFrame; \
    char filename[256]; \
    sprintf(filename, "timing_thread_%ld.txt", (long int)PlatformGetThreadID()); \
    FILE *f = fopen(filename, "w"); \
    fwrite(__dbgtimdat.dump, sizeof(char), __dbgtimdat.dump_length, f); \
    fclose(f); \
} while(0)



#else

// DEBUG is not defined, remove all timing information
#define InitializeTimingSystem   do {} while(0)
#define StartTime(name)          do {} while(0)
#define EndTime                  do {} while(0)
#define DumpTiminFrame           do {} while(0)
#define FinishTiming             do {} while(0)

#endif
#endif
