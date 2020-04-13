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

internal f64
PlatformGetTime()
{
    //StartTimer("PlatformGetTime()");
    f64 ret = ((f64)(av_gettime_relative()) / USEC);
    //EndTimer();
    return ret;
}

internal i32
PlatformSleep(f64 seconds)
{
    if(seconds < 0.0)
        seconds = 0.0;
    
    av_usleep((i32)(seconds * USEC));
    
    RETURN(SUCCESS);
}

internal i32
WaitUntil(f64 time, f64 permissible_buffer)
{
    f64 current_time = PlatformGetTime();
    f64 time_goal = time - permissible_buffer;
    f64 time_diff = time_goal < current_time ? 0.0 : (time_goal - current_time);
    
    while(time_diff > permissible_buffer)
    {
        PlatformSleep(time_diff);
        
        current_time = PlatformGetTime();
        time_diff = time_goal < current_time ? 0.0 : (time_goal - current_time);
    }
    
    RETURN(SUCCESS);
}
