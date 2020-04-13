/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed
*/

#ifndef WT_DEFINES_H
#define WT_DEFINES_H

#ifndef __cplusplus
#include <stdbool.h>
#endif // __cplusplus

#include <stdint.h>
#include "errors.h"

#ifdef internal
#undef internal
#endif

#define internal        static
#define local_persist   static
#define global          static

typedef float f32;
typedef double f64;

typedef int8_t  b8;
typedef int16_t b16;
typedef int32_t b32;
typedef int64_t b64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

// NOTE(Val): 32-bit targets don't support 128-bit integers, so defining my own if needed
#ifdef __x86_64__

typedef __int128 int128;
typedef unsigned __int128 uint128;

#else

typedef struct { i64 _pad[2]; } int128;
typedef struct { u64 _pad[2]; } uint128;

#endif // __x86_64__

typedef int128 i128;
typedef uint128 u128;

#endif
