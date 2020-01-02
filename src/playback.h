/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef PLAYBACK
#define PLAYBACK

#include "defines.h"
#include "debug.h"

typedef struct _playback_data {
    bool32 started_playing;
    
    real64 playback_start;            // platform time when playback was started
    
    real64 current_video_frame_time;  // time this video frame was displayed
    real64 next_video_frame_time;     // time next video frame will be displayed
    
    real64 pause_started;             // time when pause started
    //real64 pause_stopped;             // TODO(Val): probably not needed?
    real64 aggregated_pause_time;     // how much time was spent paused
    
    real64 audio_total_queued;        // how much audio has been queued so far
    //real64 audio_total_decoded;
    // TODO(Val): Leave this to be used when a better audio library is used
    // real64 audio_total_played;        // how much audio has already been played
    
    real64 *refresh_target;
    real64 *next_frame_time;
    real64 *current_frame_time;
} playback_data;

int32
increment_video_times(playback_data *playback, real64 video_time_base);

int32
increment_audio_times(playback_data *playback, real64 duration);

bool32
should_skip(playback_data *playback, real64 video_ts);

int32
start_playback(playback_data *p, real64 time);

static inline real64
get_playback_time(playback_data *p)
{
    return (*p->current_frame_time - p->aggregated_pause_time - p->playback_start);
}

static inline real64
get_next_playback_time(playback_data *p)
{
    return get_playback_time(p) + *p->refresh_target;
}

static inline real64
get_future_playback_time(playback_data *playback)
{
    return (get_next_playback_time(playback) + *playback->refresh_target);
}

static inline bool32
should_display(playback_data *playback, real64 video_ts)
{
    return (video_ts < get_next_playback_time(playback));
}

static inline bool32
should_queue(playback_data *playback, real64 audio_ts)
{
    real64 following_time =  get_future_playback_time(playback);
    return (playback->audio_total_queued < following_time) &&
        (audio_ts < following_time);
    //return (playback->audio_total_queued < get_future_playback_time(playback));
}

static inline bool32
enough_audio(playback_data *playback)
{
    return (playback->audio_total_queued >= get_next_playback_time(playback));
}
#endif 