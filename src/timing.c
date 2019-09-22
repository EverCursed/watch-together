#include "timing.h"

#ifdef _WIN32
#include "windows.h"
#endif

_Thread_local struct _timing_data __dbgtimdat = {};

static void
InitPlatformTimingData()
{
#if defined( _WIN32)
    QueryPerformanceFrequency((LARGE_INTEGER *)&__dbgtimdat.dat.freq);
#endif
}

static void
GetHighPrecisionTime(real64 *ptr)
{
#if defined( _WIN32)
    uint64 time;
    QueryPerformanceCounter((LARGE_INTEGER *)&time);
    *ptr = ((real64)time / (real64)__dbgtimdat.dat.freq) - __dbgtimdat.start_time;
#endif
}

void
StartTimer(char* name_c)
{
    if(__dbgtimdat.n == __dbgtimdat.max_marks)
        realloc(__dbgtimdat.inst, sizeof(timing_instance)*(__dbgtimdat.max_marks + MAX_EVENTS));
    
    __dbgtimdat.inst[__dbgtimdat.n].name = name_c;
    GetHighPrecisionTime(&__dbgtimdat.inst[__dbgtimdat.n].time);
    __dbgtimdat.n++;
    
}

void
EndTimer()
{
    if(__dbgtimdat.n == __dbgtimdat.max_marks)
        realloc(__dbgtimdat.inst, sizeof(timing_instance)*(__dbgtimdat.max_marks + MAX_EVENTS));
    
    __dbgtimdat.inst[__dbgtimdat.n].name = NULL;
    GetHighPrecisionTime(&__dbgtimdat.inst[__dbgtimdat.n].time);
    __dbgtimdat.n++;
}

static void
DumpTimingFrame()
{
    int32 n = 0; 
    struct _timing_queue queue = {};
    __dbgtimdat.dump = malloc(__dbgtimdat.n * DEBUG_LINE_WIDTH);
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
            real64 time_passed = __dbgtimdat.inst[i].time - queue.q[queue.n-1].time;
            for(int d = 0; d < queue.n-1; d++)
            {
                n += sprintf((__dbgtimdat.dump + n), "\t");
            }
            n += sprintf((__dbgtimdat.dump + n), "%s    END %lf\n", queue.q[queue.n-1].name, time_passed);
            queue.n--;
        }
    }
    __dbgtimdat.dump_length += n;
}

void
InitializeTimingSystem()
{
    InitPlatformTimingData(); 
    
    __dbgtimdat.inst = malloc(sizeof(timing_instance) * MAX_EVENTS);
    //__dbgtimdat.dump = malloc(MAX_EVENTS * DEBUG_LINE_WIDTH);
    __dbgtimdat.start_time = 0;
    GetHighPrecisionTime(&__dbgtimdat.start_time);
    __dbgtimdat.n = 0;
    __dbgtimdat.max_marks = MAX_EVENTS;
}

void
FinishTiming()
{
    DumpTimingFrame();
    char filename[256];
    sprintf(filename, "timing_thread_%ld.txt", (long int)PlatformGetThreadID());
    FILE *f = fopen(filename, "w");
    fwrite(__dbgtimdat.dump, sizeof(char), __dbgtimdat.dump_length, f);
    fclose(f);
    free(__dbgtimdat.inst);
    free(__dbgtimdat.dump);
}
