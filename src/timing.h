#ifndef TIMING_H
#define TIMING_H

#ifdef DEBUG
#ifdef _WIN32
#include "windows.h"
#endif

#include "defines.h"
#include "platform.h"


#define DEBUG_LINE_WIDTH     256
#define MAX_EVENTS         8192
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

void StartTimer(char* name_c);
void EndTimer();
void InitializeTimingSystem();
void FinishTiming();


#else

// DEBUG is not defined, remove all timing information
#define InitializeTimingSystem   do {} while(0)
#define StartTimer(name)         do {} while(0)
#define EndTimer()                 do {} while(0)
#define DumpTimingFrame()          do {} while(0)
#define FinishTiming()             do {} while(0)

#endif
#endif 