//#include "video.h"
#include "watchtogether.h"

int32
ClearVideoOutput(output_video *video)
{
    video->pitch = 0;
    video->pitch_sup1 = 0;
    video->pitch_sup2 = 0;
    video->pts = 0.0;
    
    if(video->video_frame)
    {
        video->video_frame = NULL;
        
        if(video->video_frame_sup1)
        {
            video->video_frame_sup1 = NULL;
        }
        
        if(video->video_frame_sup2)
        {
            video->video_frame_sup2 = NULL;
        }
        
        RETURN(SUCCESS);
    }
    else
    {
        dbg_error("ClearVideoOutput() failed for some reason.\n");
        RETURN(UNKNOWN_ERROR);
    }
    
}
