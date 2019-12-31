#ifndef WT_PLATFORM
#define WT_PLATFORM

#include <libavutil/frame.h>
#include "SDL_Platform.h"
#include "file_data.h"

typedef struct _platform_data platform_data;
typedef struct _thread_info thread_info;
typedef struct _cond_info cond_info;
typedef struct _platform_mutex platform_mutex;
typedef struct _output_audio output_audio;
typedef struct _output_video output_video;

int32
PlatformFlipBuffers();

int
PlatformGetInput();

void
PlatformPauseAudio(bool32);

int32
PlatformUpdateVideoFrame(AVFrame *frame);

int32
PlatformRender();


// Threads 

thread_info
PlatformCreateThread(int32 (*f)(void *), void *, char *);

void
PlatformWaitThread(thread_info, int32 *);

cond_info
PlatformCreateConditionVar();

int32
PlatformConditionWait(cond_info *);

int32
PlatformConditionSignal(cond_info *);

bool32
PlatformConditionDestroy(cond_info *);

platform_mutex
PlatformCreateMutex();

int32
PlatformLockMutex(platform_mutex *);

int32
PlatformUnlockMutex(platform_mutex *);

void
PlatformDestroyMutex(platform_mutex *);

int32
PlatformInitAudio(open_file_info *);

void
PlatformCloseAudio();

void
PlatformInitVideo(open_file_info *);

void
PlatformToggleFullscreen();

int32
PlatformResizeClientArea(open_file_info *, int32, int32);

int32
PlatformQueueAudio();

int64
PlatformGetThreadID();



#endif