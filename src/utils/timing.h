/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef TIMING_H
#define TIMING_H

#include "../defines.h"

#define DEBUG_LINE_WIDTH     256
#define MAX_EVENTS         65536
#define MAX_DEPTH             64

extern _Thread_local struct _timing_data __dbgtimdat;

struct platform_timing_data {
#ifdef _WIN32
    uint64 freq;
#elif defined(__linux__)
    
#endif
};

typedef struct _timing_instance {
    char   *name;
    f64 time;
} timing_instance;

struct _timing_queue {
    timing_instance q[MAX_DEPTH];
    int32           n;
};

struct _timing_data {
    timing_instance *inst;
    char *dump;
    char *thread_name;
    f64 start_time;
    int32 dump_length;
    int32 n;
    int32 max_marks;
    struct platform_timing_data dat;
};


#ifdef DEBUG

#ifdef _WIN32
#include "windows.h"
#endif

internal void InitializeTimingSystem(char* name);
internal void StartTimer(char* name_c);
internal void EndTimer();
internal void FinishTiming();

#else

// DEBUG is not defined, remove all timing information
#define InitializeTimingSystem(name)   do {} while(0)
#define StartTimer(name)           do {} while(0)
#define EndTimer()                 do {} while(0)
#define FinishTiming()             do {} while(0)

#endif // DEBUG
#endif 