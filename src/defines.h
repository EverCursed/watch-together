#ifndef WT_DEFINES_H
#define WT_DEFINES_H

#define DEBUG

#include <stdint.h>

//#define internal        static
#define local_persist   static
#define global          static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

typedef uint32 bool32;

#ifdef DEBUG
#define debug(s, ...) printf("%s|%s|%d\t" s "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define debug(s, ...) {}
#endif

#endif