#include "defines.h"
#include "playback.h"

static int32
init_playback_time(playback_data *playback, real64 current_time, real64 frame_duration)
{
    playback->playback_start = current_time;
    playback->playback_time = 0.0;
    playback->frame_duration = frame_duration;
    
    playback->current_frame_time = current_time;
    playback->next_frame_time = current_time + frame_duration;
    
    playback->current_video_frame_time = -1.0;
    playback->next_video_frame_time =  playback->next_frame_time;// playback->next_frame_time + 1000.0*av_q2d(pdata->decoder.video_time_base);
    
    playback->aggregated_pause_time = 0.0;
    
    playback->pause_started = -1.0;
    playback->pause_stopped = -1.0;
    
    playback->playback_start = -1.0;
    playback->playback_time = -1.0;
    
    playback->aggregated_pause_time = 0.0;
    
    playback->audio_total_queued = 0.0;
    playback->audio_total_played = 0.0;
}

static int32
increment_video_times(playback_data *playback, real64 video_time_base)
{
    playback->current_video_frame_time = playback->next_video_frame_time;
    playback->next_video_frame_time += video_time_base;
}

static int32
increment_audio_times(playback_data *playback, real64 duration)
{
    playback->audio_total_queued += duration;
    
}

static bool32
should_display(playback_data *playback, real64 video_ts)
{
    return video_ts < get_playback_time(playback, playback->next_frame_time);
}

static bool32
should_queue(playback_data *playback)
{
    return (playback->audio_total_queued < get_playback_time(playback, playback->next_frame_time));
}

static real64
get_playback_current_time(playback_data *playback)
{
    return (playback->current_frame_time - playback->playback_start - playback->aggregated_pause_time);
}

static real64
get_playback_time(playback_data *playback, real64 time)
{
    return (time - playback->playback_start - playback->aggregated_pause_time);
}

