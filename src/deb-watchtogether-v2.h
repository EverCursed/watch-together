#ifndef WT_V2
#define WT_V2


typedef struct _pixel_buffer {
    uint32 width;
    uint32 height;
    void *buffer;
} pixel_buffer;

typedef struct _mouse_info {
    uint32 old_x;
    uint32 old_y;
    uint32 x;
    uint32 y;
    bool32 left_button_was_pressed;
    bool32 right_button_was_pressed;
    bool32 left_button_is_pressed;
    bool32 right_button_is_pressed;
} mouse_info;

typedef struct _sound_sample {
    void *Data;
    uint32 Size;
    uint32 CurrentIndex;
    uint32 Frequency;
    uint16 Channels;
    uint16 BitsPerSample;
} sound_sample;

typedef struct _video_data {
    uint32 framerate;
    // TODO(Val): Add the relevant information
} video_data;

typedef struct _program_data {
    pixel_buffer Pixels;
    mouse_info Mouse;
    // TODO(Val): Remove this stuff, it's temporary
    uint32 gradient_index;
    sound_sample *SoundSample;
} program_data;


static void
PlatformEnqueueAudio(sound_sample *SoundSample);

static void
blit_frame(pixel_buffer buffer);

static void
set_FPS(float value);

#endif