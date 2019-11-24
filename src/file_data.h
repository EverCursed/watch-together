#ifndef FILE_DATA_H
#define FILE_DATA_H

#include "defines.h"

typedef struct _open_file_info {
    char *filename;
    
    uint32 width;
    uint32 height;
    uint32 video_format;
    real32 fps;
    real32 target_time;
    
    uint32 sample_rate;
    uint32 bytes_per_sample;
    uint32 channels;
    uint32 sample_format;
    
    bool32 has_audio;
    bool32 has_video;
    bool32 has_subtitles;
    
    bool32 file_opened;        // signals that the file was successfully opened
    bool32 file_ready;         // // TODO(Val): signals that the data is ready to be displayed
    bool32 open_failed;        // signals that opening the file failed
    
    volatile bool32 file_finished;
    // TODO(Val): audio format
} open_file_info;



#endif // FILE_DATA_H