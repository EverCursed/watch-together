/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#include "playback.h"
//#include "watchtogether.h"

int32
increment_video_times(playback_data *playback, real64 video_time_base)
{
    playback->current_video_frame_time = playback->next_video_frame_time;
    playback->next_video_frame_time += video_time_base;
    
    RETURN(SUCCESS);
}

int32
increment_audio_times(playback_data *playback, real64 duration)
{
    playback->audio_total_queued += duration;
    
    RETURN(SUCCESS);
}
/*
static void
update_playback_time(playback_data *playback)
{
dbg_print("update_playback_time:\n"
"\tcurrent_frame_time: %lf\n"
"\taggregated_pause_time: %lf\n"
"\tplayback_start: %lf\n",
*playback->current_frame_time,
playback->aggregated_pause_time,
playback->playback_start);

playback->playback_time = 
*playback->current_frame_time -
playback->aggregated_pause_time -
playback->playback_start;
}
*/

real64
get_next_frame_time(playback_data *p)
{
    return *p->next_frame_time;
}

bool32
should_skip(playback_data *playback, real64 video_ts)
{
    return (get_next_playback_time(playback) > video_ts);
}

int32
start_playback(playback_data *p, real64 time)
{
    p->playback_start = time + *p->refresh_target;
    //p->playback_time = -*p->refresh_target;
    
    //p->current_video_frame_time = get_playback_time(p);
    //p->next_video_frame_time = p->playback_start;// playback->next_frame_time + 1000.0*av_q2d(pdata->decoder.video_time_base);
    
    p->aggregated_pause_time = 0.0;
    p->pause_started = 0.0;
    //p->pause_stopped = 0.0;
    p->audio_total_queued = 0.0;
    // p->audio_total_played = 0.0;
    
    p->started_playing = 1;
    
    RETURN(SUCCESS);
}
