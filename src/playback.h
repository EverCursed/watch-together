#ifndef PLAYBACK
#define PLAYBACK

typedef struct _playback_data {
    real64 frame_duration;
    
    real64 playback_start;            // platform time when playback was started
    real64 playback_time;             // how long the file has been playing for
    
    //real64 current_frame_time;        // time this frame was displayed
    //real64 next_frame_time;           // time next frame will be displayed
    
    real64 current_video_frame_time;  // time this video frame was displayed
    real64 next_video_frame_time;     // time next video frame will be displayed
    
    real64 aggregated_pause_time;     // how much time was spent paused
    
    real64 pause_started;             // time when pause started
    real64 pause_stopped;             // TODO(Val): probably not needed?
    
    real64 audio_total_queued;        // how much audio has been queued so far
    real64 audio_total_played;        // how much audio has already been played
    //real64 frame_duration;
    real64 *refresh_target;
    real64 *next_frame_time;
    real64 *current_frame_time;
} playback_data;

static int32
increment_video_times(playback_data *playback);

static int32
increment_audio_times(playback_data *playback, real64 duration);

static bool32
should_display(playback_data *playback, real64 video_ts);

static bool32
should_queue(playback_data *playback);

static real64
get_playback_current_time(playback_data *playback);

static real64
get_playback_time(playback_data *playback);

static void
start_playback(playback_data *p, real64 time);

#endif 