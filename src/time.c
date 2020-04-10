/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#include <libavutil/time.h>
#include "defines.h"
#include "utils/timing.h"

#define MSEC (1000.0)
#define USEC (MSEC*1000.0)
#define NSEC (USEC*1000.0)

internal real64
PlatformGetTime()
{
    //StartTimer("PlatformGetTime()");
    real64 ret = ((real64)(av_gettime_relative()) / USEC);
    //EndTimer();
    return ret;
}

internal int32
PlatformSleep(real64 seconds)
{
    if(seconds < 0.0)
        seconds = 0.0;
    
    av_usleep((int32)(seconds * USEC));
    
    RETURN(SUCCESS);
}

internal int32
WaitUntil(real64 time, real64 permissible_buffer)
{
    real64 current_time = PlatformGetTime();
    real64 time_goal = time - permissible_buffer;
    real64 time_diff = time_goal < current_time ? 0.0 : (time_goal - current_time);
    
    while(time_diff > permissible_buffer)
    {
        PlatformSleep(time_diff);
        
        current_time = PlatformGetTime();
        time_diff = time_goal < current_time ? 0.0 : (time_goal - current_time);
    }
    
    RETURN(SUCCESS);
}
