#ifndef WT 
#define WT

typedef struct _pixel_buffer {
    void* buffer;
    uint32 width;
    uint32 height;
} pixel_buffer;

typedef struct _mouse_info {
    int32 old_x;
    int32 old_y;
    int32 x;
    int32 y;
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

typedef struct _program_data {
    pixel_buffer Pixels;
    mouse_info Mouse;
    // TODO(Val): Remove this stuff, it's temporary
    sound_sample *SoundSample;
} program_data;


#endif