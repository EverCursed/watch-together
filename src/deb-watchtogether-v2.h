#ifndef WT_V2
#define WT_V2

// NOTE(Val): Forward declaration, declared in watchtogether.h
typedef struct _pixel_buffer pixel_buffer;
typedef struct _mouse_info mouse_info;
typedef struct _sound_sample sound_sample;

typedef struct _video_data {
    uint32 framerate;
    // TODO(Val): Add the relevant information
} video_data;

static void
PlatformEnqueueAudio(sound_sample *SoundSample);

static void
blit_frame(pixel_buffer buffer);

static void
set_FPS(float value);

#endif