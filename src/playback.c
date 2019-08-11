#include "defines.h"
#include "playback.h"

#define AUDIO_QUEUE_MARGIN 2.0

static int32
increment_video_times(playback_data *playback)
{
    //playback->current_video_frame_time = playback->next_video_frame_time;
    //playback->next_video_frame_time += video_time_base;
    
    return 0;
}

static int32
increment_audio_times(playback_data *playback, real64 duration)
{
    playback->audio_total_queued += duration;
    
    return 0;
}

static bool32
should_display(playback_data *playback, real64 video_ts)
{
    real64 n = get_playback_time(playback) + *playback->refresh_target;
    return (video_ts < n &&
            video_ts >= n + *playback->refresh_target);
}

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

static real64
get_playback_time(playback_data *playback)
{
    update_playback_time(playback);
    return playback->playback_time;
}

static bool32
should_skip(playback_data *playback, real64 video_ts)
{
    return (*playback->next_frame_time - *playback->refresh_target < video_ts);
}

static bool32
should_queue(playback_data *playback)
{
    dbg_print("should_queue(): %lf <= %lf && %lf >= %lf\n",
              playback->audio_total_queued - AUDIO_QUEUE_MARGIN,
              (get_playback_time(playback) + *playback->refresh_target),
              playback->audio_total_queued - AUDIO_QUEUE_MARGIN,
              get_playback_time(playback));
    return (playback->audio_total_queued - AUDIO_QUEUE_MARGIN <= (get_playback_time(playback) + *playback->refresh_target) &&
            playback->audio_total_queued - AUDIO_QUEUE_MARGIN >= get_playback_time(playback));
}

static real64
get_playback_current_time(playback_data *playback)
{
    return (*playback->current_frame_time - playback->playback_start - playback->aggregated_pause_time);
}

static void
start_playback(playback_data *p, real64 time)
{
    p->playback_start = time + *p->refresh_target;
    p->playback_time = -*p->refresh_target;
    
    p->current_video_frame_time = p->playback_time;
    p->next_video_frame_time = p->playback_start;// playback->next_frame_time + 1000.0*av_q2d(pdata->decoder.video_time_base);
    
    p->aggregated_pause_time = 0.0;
    p->pause_started = 0.0;
    p->pause_stopped = 0.0;
    p->audio_total_queued = 0.0;
    p->audio_total_played = 0.0;
}