#ifndef WT_V2
#define WT_V2


typedef struct {
    uint32 width;
    uint32 height;
    void *buffer;
} pixel_buffer;

typedef struct {
    uint32 old_x;
    uint32 old_y;
    uint32 x;
    uint32 y;
    bool32 left_button_was_pressed;
    bool32 right_button_was_pressed;
    bool32 left_button_is_pressed;
    bool32 right_button_is_pressed;
} mouse_info;

typedef struct {
    void *Data;
    uint32 Size;
    uint32 CurrentIndex;
    uint32 Frequency;
    uint16 Channels;
    uint16 BitsPerSample;
} sound_sample;

typedef struct {
    pixel_buffer Pixels;
    mouse_info Mouse;
    // TODO(Val): Remove this stuff, it's temporary
    uint32 gradient_index;
    sound_sample *SoundSample;
} program_data;


internal void
PlatformEnqueueAudio(sound_sample *SoundSample);

#endif