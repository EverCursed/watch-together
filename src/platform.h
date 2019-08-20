#ifndef WT_PLATFORM
#define WT_PLATFORM

static real64
PlatformGetTime();

static int32
PlatformFlipBuffers(program_data *);

static int
PlatformGetInput(program_data *);

static void
PlatformPauseAudio(bool32);

static int32
PlatformUpdateFrame(program_data *);

static void
PlatformSleep(real64);

static thread_info
PlatformCreateThread(int32 (*f)(void *), void *, char *);

static void
PlatformWaitThread(thread_info, int32 *);

static cond_info
PlatformCreateConditionVar();

static int32
PlatformConditionWait(cond_info *);

static int32
PlatformConditionSignal(cond_info *);

static bool32
PlatformConditionDestroy(cond_info *);

static void
PlatformInitAudio(program_data *);

static void
PlatformCloseAudio(program_data *);

static void
PlatformInitVideo(program_data *);

static void
PlatformToggleFullscreen(program_data *);

static int32
PlatformQueueAudio(output_audio *);

#endif