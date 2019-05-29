#include "watchtogether.h"
#include "audio_queue.h"

// TODO(Val): Add thread synchronization if its even needed

#define AUDIO_SECONDS 5 

static void
init_audio_queue(audio_queue_data *data,
                 uint32 f,
                 uint32 c,
                 uint32 b)
{
    data->frequency = f;
    data->channels = c;
    data->bytes_per_sample = b;
    
    data->audio_queue_size = f*c*b*AUDIO_SECONDS;
    data->audio_queue_buffer = malloc(data->audio_queue_size);
    data->audio_queue_start = 0;
    data->audio_queue_end = 0;
    data->audio_queue_used_space = 0;
}

/**
Returns the number of samples enqueued
*/

static int32
enqueue_audio_bytes(audio_queue_data *data, void *src, uint32 bytes)
{
    if(data->audio_queue_size - data->audio_queue_used_space < bytes)
    {
        dbg_print("Warning: Not enough space to enqueue audio.\n");
        return -1;
    }
    
    uint32 new_end = (data->audio_queue_end + bytes) % data->audio_queue_size;
    
    if(data->audio_queue_end < new_end)
    {
        memcpy(data->audio_queue_buffer + data->audio_queue_end, src, bytes);
    }
    else
    {
        uint32 size1 = data->audio_queue_size - data->audio_queue_end;
        uint32 size2 = bytes - size1;
        
        memcpy(data->audio_queue_buffer + data->audio_queue_end, src, size1);
        memcpy(data->audio_queue_buffer, src+size1, size2);
    }
    data->audio_queue_end = new_end;
    data->audio_queue_used_space += bytes;
    
    return 0;
}

static inline int32
enqueue_audio_samples(audio_queue_data *data, void *src, uint32 sample_count)
{
    return enqueue_audio_bytes(data, src, sample_count*data->channels*data->bytes_per_sample);
}

static int32
dequeue_audio_bytes(audio_queue_data *data, void *dst, uint32 size)
{
    //if(!((data->audio_queue_start + size) < data->audio_queue_end ||
    //(data->audio_queue_start + size) > data->audio_queue_start))
    if(data->audio_queue_used_space < size)
    {
        /*
        dbg_print("audio_queue_start: %d\n"
                  "audio_queue_end: %d\n"
                  "audio_queue_size: %d\n"
                  "size: %d",
                  data->audio_queue_start,
                  data->audio_queue_end,
                  data->audio_queue_size,
                  size);
        */
        return -1;
    }
    
    if(data->audio_queue_start + size <= data->audio_queue_size)
    {
        memcpy(dst, data->audio_queue_buffer+data->audio_queue_start, size);
        
        data->audio_queue_start += size;
    }
    else
    {
        uint32 part1 = data->audio_queue_size - data->audio_queue_start;
        uint32 part2 = size - part1;
        memcpy(dst, data->audio_queue_buffer+data->audio_queue_start, part1);
        memcpy(dst+part1, data->audio_queue_buffer, part2);
        
        data->audio_queue_start = part2;
    }
    data->audio_queue_used_space -= size;
    //dbg_success("End of audio dequeue function.\n");
    
    return 0;
}

static int32
dequeue_audio_samples(audio_queue_data *data, void *dst, uint32 sample_count)
{
    return dequeue_audio_bytes(data, dst, sample_count*data->channels*data->bytes_per_sample);
}

static void
close_audio_queue(audio_queue_data *data)
{
    if(data->audio_queue_buffer)
    {
        free(data->audio_queue_buffer);
        data->audio_queue_buffer = NULL;
    }
}

static void
reset_audio_queue(audio_queue_data *data,
                  uint32 f,
                  uint32 c,
                  uint32 b)
{
    close_audio_queue(data);
    init_audio_queue(data, f, c, b);
}
