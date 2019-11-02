#include <libavutil/time.h>
#include "defines.h"
#include "utils/timing.h"

#define MSEC (1000.0)
#define USEC (MSEC*1000.0)
#define NSEC (USEC*1000.0)

static int64 init_time = 0;

real64
PlatformGetTime()
{
    //StartTimer("PlatformGetTime()");
    if(!init_time)
        init_time = av_gettime_relative();
    
    real64 ret = ((real64)(av_gettime_relative() - init_time) / USEC);
    //EndTimer();
    return ret;
}

int32
PlatformSleep(real64 seconds)
{
    if(seconds < 0.0)
        seconds = 0.0;
    
    av_usleep((int32)(seconds * USEC));
    
    RETURN(SUCCESS);
}

int32
WaitUntil(real64 time, real64 permissible_buffer)
{
    real64 current_time = PlatformGetTime();
    real64 time_diff = time < current_time ? 0.0 : time - current_time;
    
    while(time_diff > permissible_buffer)
    {
        PlatformSleep(0.0);
        
        current_time = PlatformGetTime();
        time_diff = time < current_time ? 0.0 : time - current_time;
    }
    
    RETURN(SUCCESS);
}
