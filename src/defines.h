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

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  bool8;
typedef int16_t bool16;
typedef int32_t bool32;
typedef int64_t bool64;

typedef bool8 b8;
typedef bool16 b16;
typedef bool32 b32;
typedef bool64 b64;

typedef int8   i8;
typedef int16  i16;
typedef int32  i32;
typedef int64  i64;

typedef uint8   u8;
typedef uint16  u16;
typedef uint32  u32;
typedef uint64  u64;

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
