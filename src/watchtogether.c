struct pixel {
    union{
        struct {
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

void draw_dot(pixel_buffer buffer, int x, int y, uint32 color)
{
    *((uint32 *)buffer.buffer + buffer.width*y + x) = color;
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
            //((uint32)( + (real32)gradient[color+1]*pct2)) * (1.0f - (real32)y/(real32)surface->h);
            
        }
    }
}


internal void
Processing(game_data *data)
{
    // process input
    
    // draw
    //clear_surface(data->Pixels);
    draw_gradient(data->Pixels);
    
    //draw_dot(surface, 75, 50, GenColor(255, 255, 255, 0));
    
    // sound
    
    //
}