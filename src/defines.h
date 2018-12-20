#include <stdint.h>

#define internal              static
#define local_persist         static
#define global                static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define MSG_INVALID 0
#define MSG_PLAY              52489
#define MSG_STOP              52490
#define MSG_PAUSE             52491
#define MSG_REWIND_PRESS      52492
#define MSG_REWIND_RELEASE    52493
#define MSG_FORWARD_PRESS     52494
#define MSG_FORWARD_RELEASE   52495
#define MSG_NEXT              52496
#define MSG_PREVIOUS          52497
#define MSG_CONNECT_TO_HOST   52498
#define MSG_INVITE_PARTNER    52499
