/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

Header for open file information.
*/

#ifndef FILE_DATA_H
#define FILE_DATA_H

#include "defines.h"

typedef struct _open_file_info {
    char filename[256];
    
    u32 width;
    u32 height;
    u32 video_format;
    f32 fps;
    f32 target_time;
    
    u32 sample_rate;
    u32 bytes_per_sample;
    u32 channels;
    u32 sample_format;
    
    b32 has_audio;
    b32 has_video;
    b32 has_subtitles;
    
    b32 file_opened;        // signals that the file was successfully opened
    b32 file_ready;         // // TODO(Val): signals that the data is ready to be displayed
    b32 open_failed;        // signals that opening the file failed
    
    b32 file_finished;
    // TODO(Val): audio format
} open_file_info;



#endif // FILE_DATA_H