#define GenColor(R,G,B,A) ((R << 16) | (G << 8) | (B) | (A << 24))

// TODO(Val): ffmpeg decoding/encoding
// TODO(Val): NAT-T implementation, see how it works
// TODO(Val): Encryption

struct pixel {
    union {
        struct __attribute__ ((packed)){
            uint8 red;
            uint8 green;
            uint8 blue;
            uint8 alpha;
        };
        uint32 color;
    };
};

global uint32 gradient_index = 0;
global uint32 gradient[][5] = 
{
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
        GenColor(255, 0, 255, 0),
        GenColor(128, 128, 255, 0),
        GenColor(0, 255, 255, 0),
        GenColor(0, 255, 128, 0),
        GenColor(0, 255, 0, 0),
    },
};


void clear_surface(pixel_buffer buffer)
{
    int w = buffer.width;
    int h = buffer.height;
    uint32 *pixel = (uint32 *)buffer.buffer;
    uint32 color = GenColor(255, 0, 0, 0);
    debug("color: %d", color);
    
    for(int i = 0; i < w*h; i++)
    {
        *(pixel + i) = color;
    }
}

void draw_gradient(pixel_buffer buffer)
{
    for(int x = 0; x < buffer.width; x++)
    {
        real32 grad = (real32)buffer.width / (ArrayCount(gradient[gradient_index])-1);
        uint32 color = (uint32)(x / grad);
        real32 pct1 = (real32)(x - color*grad ) / (real32)grad; 
        real32 pct2 = 1.0f - pct1; 
        for(int y = 0; y < buffer.height; y++)
        {
            struct pixel *p = (struct pixel *)((uint32 *)buffer.buffer + buffer.width*y + x);
            
            struct pixel color1 = {};
            color1.color = (uint32)gradient[gradient_index][color+1]; 
            color1.red *= pct1;
            color1.green *= pct1;
            color1.blue *= pct1;
            
            color1.red *= ((real32)(buffer.height - y) / (real32)buffer.height);
            color1.green *= ((real32)(buffer.height - y) / (real32)buffer.height);
            color1.blue *= ((real32)(buffer.height - y) / (real32)buffer.height);
            
            struct pixel color2 = {};
            color2.color = (real32)gradient[gradient_index][color]; 
            color2.red *= pct2;
            color2.green *= pct2;
            color2.blue *= pct2;
            
            color2.red *= ((real32)(buffer.height - y) / (real32)buffer.height);
            color2.green *= ((real32)(buffer.height - y) / (real32)buffer.height);
            color2.blue *= ((real32)(buffer.height - y) / (real32)buffer.height);
            
            
            p->color = (color1.color + color2.color);
            p->alpha = 0;
            //((uint32)( + (real32)gradient[color+1]*pct2)) * (1.0f - (real32)y/(real32)surface->h);
            
        }
    }
}

global bool32 is_pressed = 0;
global uint32 start_x = 0;
global uint32 start_y = 0;
global uint32 end_x = 0;
global uint32 end_y = 0;
global struct pixel outline_color;
global struct pixel fill_color;

internal inline
struct pixel invert_color(struct pixel color)
{
    struct pixel p = {};
    p.red = 255 - color.red;
    p.green = 255 - color.green;
    p.blue = 255 - color.blue;
    p.alpha = color.alpha;
    return p;
}

internal inline
void mix_color(struct pixel *dst_color, struct pixel src_color)
{
    struct pixel src = src_color;
    struct pixel *dst = dst_color;
    real32 alpha = (real32)src.alpha / 255.0f;
    
    dst->red = (src.red * (1.0f-alpha)) + (dst->red * (alpha));
    dst->green = (src.green* (1.0f-alpha)) + (dst->green * (alpha));
    dst->blue = (src.blue * (1.0f-alpha)) + (dst->blue * (alpha));
    dst->alpha = 0;
}

void draw_selection(pixel_buffer Buffer, mouse_info Mouse)
{
    outline_color.color = GenColor(0x00, 0x80, 0xFF, 102);
    fill_color.color = GenColor(0x24, 0x24, 0x24, 102);
    
    if(Mouse.left_button_is_pressed)
    {
        if(Mouse.left_button_was_pressed)
        {
            end_x = Mouse.x;
            end_y = Mouse.y;
        }
        else
        {
            if(start_x == 0 && start_y == 0)
            {
                start_x = Mouse.x;
                start_y = Mouse.y;
            }
            end_x = Mouse.x;
            end_y = Mouse.y;
        }
        is_pressed = 1;
    }
    else
    {
        start_x = 0;
        start_y = 0;
        end_x = 0;
        end_y = 0;
        is_pressed = 0;
    }
    
    uint32 left_x = start_x < end_x ? start_x : end_x;
    uint32 right_x = start_x >= end_x ? start_x : end_x;
    uint32 top_y = start_y < end_y ? start_y : end_y;
    uint32 bottom_y = start_y >= end_y ? start_y : end_y;
    
    uint32* buffer = Buffer.buffer;
    for(int y = top_y; y <= bottom_y; y++)
    {
        for(int x = left_x; x <= right_x; x++)
        {
            struct pixel *p = ((struct pixel*)buffer + y*Buffer.width + x);
            if(y == top_y || y == bottom_y || x == left_x || x == right_x)
            {
                struct pixel color = *p;
                color.alpha = 0x40;
                mix_color(p, invert_color(color));
            }
            else
                mix_color(p, fill_color);
        }
    }
}

internal void
Processing(program_data *data)
{
    // process input
    
    // draw
    //     draw video frame
    //     draw UI
    draw_gradient(data->Pixels);
    draw_selection(data->Pixels, data->Mouse);
    
    // Audio
    PlatformEnqueueAudio(data->SoundSample);
    
    //
}