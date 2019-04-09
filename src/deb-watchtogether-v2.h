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
    pixel_buffer Pixels;
    mouse_info Mouse;
    // audio_buffer Audio;
    uint32 gradient_index;
    
    /*
    uint32 gradient[][5] = {
        {
            GenColor(255, 255, 0, 0),
            GenColor(0, 0, 0, 0),
            GenColor(255, 255, 255, 0),
            GenColor(0, 0, 0, 0),
            GenColor(255, 0, 255, 0),
        },
        {
            GenColor(255, 0, 0, 0),
            GenColor(255, 255, 0, 0),
            GenColor(0, 255, 0, 0),
            GenColor(0, 255, 255, 0),
            GenColor(0, 0, 255, 0),
        },
        {
            GenColor(52, 188, 2, 0),
            GenColor(244, 126, 56, 0),
            GenColor(210, 52, 186, 0),
            GenColor(24, 71, 22, 0),
            GenColor(61, 222, 81, 0),
        },
    };*/
} game_data;


#endif