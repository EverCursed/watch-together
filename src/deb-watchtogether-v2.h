#ifndef WT_V2
#define WT_V2


typedef struct {
    uint32 width;
    uint32 height;
    void *buffer;
} pixel_buffer;

typedef struct {
    pixel_buffer Pixels;
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