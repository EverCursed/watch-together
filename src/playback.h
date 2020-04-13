/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef PLAYBACK_H
#define PLAYBACK_H

#include "defines.h"
#include "debug.h"

typedef struct _playback_data {
    b32 started_playing;
    
    f64 playback_start;            // platform time when playback was started
    
    f64 current_video_frame_time;  // time this video frame was displayed
    f64 next_video_frame_time;     // time next video frame will be displayed
    
    f64 pause_started;             // time when pause started
    //f64 pause_stopped;             // TODO(Val): probably not needed?
    f64 aggregated_pause_time;     // how much time was spent paused
    
    f64 audio_total_queued;        // how much audio has been queued so far
    //f64 audio_total_decoded;
    // TODO(Val): Leave this to be used when a better audio library is used
    // f64 audio_total_played;        // how much audio has already been played
    
    f64 *refresh_target;
    f64 *next_frame_time;
    f64 *current_frame_time;
} playback_data;

internal i32
increment_video_times(playback_data *playback, f64 video_time_base);

internal i32
increment_audio_times(playback_data *playback, f64 duration);

internal b32
should_skip(playback_data *playback, f64 video_ts);

internal i32
start_playback(playback_data *p, f64 time);

internal void
playback_start_pause(playback_data *p)
{
    p->pause_started = *p->current_frame_time;
}

internal void
playback_end_pause(playback_data *p)
{
    p->aggregated_pause_time += (*p->current_frame_time - p->pause_started);
}

internal inline f64
get_playback_time(playback_data *p)
{
    return (*p->current_frame_time - p->aggregated_pause_time - p->playback_start);
}

internal inline f64
get_next_playback_time(playback_data *p)
{
    return get_playback_time(p) + *p->refresh_target;
}

internal inline f64
get_future_playback_time(playback_data *playback)
{
    return (get_next_playback_time(playback) + *playback->refresh_target);
}

internal inline b32
should_display(playback_data *playback, f64 video_ts)
{
    f32 playback_time = get_next_playback_time(playback);
    return (video_ts < playback_time);
}

internal inline b32
should_queue(playback_data *playback, f64 audio_ts)
{
    f64 following_time =  get_future_playback_time(playback);
    return (playback->audio_total_queued < following_time) &&
        (audio_ts < following_time);
    //return (playback->audio_total_queued < get_future_playback_time(playback));
}

internal inline b32
enough_audio(playback_data *playback)
{
    return (playback->audio_total_queued >= get_next_playback_time(playback));
}

#endif // PLAYBACK_H
