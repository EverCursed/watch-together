/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#include "timing.h"

#include <stdio.h>

#include "../common/custom_malloc.h"

#ifdef _WIN32
#include <windows.h>
#endif

_Thread_local struct _timing_data __dbgtimdat = {};

#undef InitializeTimingSystem
#undef StartTimer
#undef EndTimer
#undef FinishTiming


internal void
InitPlatformTimingData()
{
#if defined( _WIN32)
    QueryPerformanceFrequency((LARGE_INTEGER *)&__dbgtimdat.dat.freq);
#endif
}

internal void
GetHighPrecisionTime(f64 *ptr)
{
#if defined( _WIN32)
    u64 time;
    QueryPerformanceCounter((LARGE_INTEGER *)&time);
    *ptr = ((f64)time / (f64)__dbgtimdat.dat.freq) - __dbgtimdat.start_time;
#endif
}

internal void
StartTimer(char* name_c)
{
#ifdef DEBUG
    if(__dbgtimdat.n == __dbgtimdat.max_marks)
    {
        __dbgtimdat.inst =
            custom_realloc(__dbgtimdat.inst, sizeof(timing_instance)*(__dbgtimdat.max_marks + MAX_EVENTS));
        __dbgtimdat.max_marks += MAX_EVENTS;
    }
    
    __dbgtimdat.inst[__dbgtimdat.n].name = name_c;
    GetHighPrecisionTime(&__dbgtimdat.inst[__dbgtimdat.n].time);
    __dbgtimdat.n++;
#else
    return;
#endif
}

internal void
EndTimer()
{
#ifdef DEBUG
    if(__dbgtimdat.n == __dbgtimdat.max_marks)
    {
        __dbgtimdat.inst = custom_realloc(__dbgtimdat.inst, sizeof(timing_instance)*(__dbgtimdat.max_marks + MAX_EVENTS));
        __dbgtimdat.max_marks += MAX_EVENTS;
    }
    __dbgtimdat.inst[__dbgtimdat.n].name = NULL;
    GetHighPrecisionTime(&__dbgtimdat.inst[__dbgtimdat.n].time);
    __dbgtimdat.n++;
#else
    return;
#endif
}

internal void
DumpTimingFrame()
{
#ifdef DEBUG
    i32 n = 0; 
    struct _timing_queue queue = {};
    __dbgtimdat.dump = custom_malloc(__dbgtimdat.n * DEBUG_LINE_WIDTH);
    for(int i = 0; i < __dbgtimdat.n; i++)
    {
        if(__dbgtimdat.inst[i].name != NULL)
        {
            queue.q[queue.n] = __dbgtimdat.inst[i];
            for(int d = 0; d < queue.n; d++)
            {
                n += sprintf((__dbgtimdat.dump + n), "\t");
            }
            n += sprintf((__dbgtimdat.dump + n), "%s    START\n", queue.q[queue.n].name);
            queue.n++;
        }
        else
        {
            queue.n--;
            f64 time_passed = __dbgtimdat.inst[i].time - queue.q[queue.n].time;
            for(int d = 0; d < queue.n; d++)
            {
                n += sprintf((__dbgtimdat.dump + n), "\t");
            }
            n += sprintf((__dbgtimdat.dump + n), "%s    END %lf\n", queue.q[queue.n].name, time_passed);
        }
    }
    __dbgtimdat.dump_length += n;
#else
    return;
#endif
}

internal void
InitializeTimingSystem(char *name)
{
#ifdef DEBUG
    InitPlatformTimingData(); 
    
    __dbgtimdat.inst = custom_malloc(sizeof(timing_instance) * MAX_EVENTS);
    //__dbgtimdat.dump = malloc(MAX_EVENTS * DEBUG_LINE_WIDTH);
    __dbgtimdat.start_time = 0;
    __dbgtimdat.thread_name = name;
    GetHighPrecisionTime(&__dbgtimdat.start_time);
    __dbgtimdat.n = 0;
    __dbgtimdat.max_marks = MAX_EVENTS;
#else
    return;
#endif
}

internal void
FinishTiming()
{
#ifdef DEBUG
    DumpTimingFrame();
    char filename[256];
    sprintf(filename, "debug/%s-thread.txt", __dbgtimdat.thread_name);
    FILE *f = fopen(filename, "w");
    fwrite(__dbgtimdat.dump, sizeof(char), __dbgtimdat.dump_length, f);
    fclose(f);
    free(__dbgtimdat.inst);
    free(__dbgtimdat.dump);
#else
    return;
#endif
}
