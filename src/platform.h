#ifndef WT_PLATFORM
#define WT_PLATFORM

typedef struct _input_data {
    
} input_data;

static uint32
PlatformGetTime();

static int
PlatformGetInput(program_data *);

static void
PlatformPauseAudio(bool32 on);

static int32
PlatformFrameUpdater(void *data);

static void
PlatformSleep(int32);

static thread_info
PlatformCreateThread(int32 (*f)(void *), void*, char*);

static int32
PlatformWaitThread(thread_info, int32 *);

static void
PlatformInitAudio(program_data *);

static void
PlatformInitVideo(program_data *);

#endif