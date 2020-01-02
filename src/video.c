/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

//#include "video.h"
#include "watchtogether.h"

int32
ClearVideoOutput(output_video *video)
{
    //video->pitch = 0;
    //video->pitch_sup1 = 0;
    //video->pitch_sup2 = 0;
    video->pts = 0.0;
    video->frame_duration = 0.0;
    
    RETURN(SUCCESS);
}
