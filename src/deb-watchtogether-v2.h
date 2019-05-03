#ifndef DEB_WT
#define DEB_WT

typedef struct _platform_data {
    
} platform_data;

typedef struct _thread_info {
    SDL_Thread *thread;
} thread_info;

//static void
//EnqueueAudio(sound_sample *SoundSample);

static void
blit_frame();

static void
set_FPS(float value);

#endif