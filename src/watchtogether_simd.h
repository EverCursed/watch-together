#ifndef WATCHTOGETHER_SIMD_H
#define WATCHTOGETHER_SIMD_H

#include "defines.h"

typedef struct v4_ {
    union {
        struct {
            union {
                union {
                    r32 a;
                    r32 x;
                };
                union {
                    r32 b;
                    r32 y;
                };
                union {
                    r32 c;
                    r32 z;
                };
                union {
                    r32 d;
                    r32 w;
                };
                r32 E[4];
                __m128 P;
            };
        };
        
        struct {
            union {
                union {
                    s32 a;
                    s32 x;
                };
                union {
                    s32 b;
                    s32 y;
                };
                union {
                    s32 c;
                    s32 z;
                };
                union {
                    s32 d;
                    s32 w;
                };
                s32 E[4];
                __m128i P;
            };
        } s;
    };
} v4;

static void v4_print(v4 a)
{
    printf("vector = {\n"
           "\t.f = {\n"
           "\t\ta = %f,\n"
           "\t\tb = %f,\n"
           "\t\tc = %f,\n"
           "\t\td = %f\n"
           "\t},\n"
           "\t.s = {\n"
           "\t\ta = %d,\n"
           "\t\tb = %d,\n"
           "\t\tc = %d,\n"
           "\t\td = %d\n"
           "\t}\n"
           "}\n", 
           a.a, a.b, a.c, a.d,
           a.s.a, a.s.b, a.s.c, a.s.d);
}

static inline v4 v4_set1_ps(r32 a)
{
    v4 temp;
    
    temp.P = _mm_set1_ps(a);
    
    return temp;
}

static inline v4 v4_set_ps(r32 a, r32 b, r32 c, r32 d)
{
    v4 temp;
    
    temp.P = _mm_set_ps(a, b, c, d);
    
    return temp;
}

static inline v4 v4_add_ps(v4 a, v4 b)
{
    v4 temp;
    
    temp.P = _mm_add_ps(a.P, b.P);
    
    return temp;
}

static inline v4 v4_sub_ps(v4 a, v4 b)
{
    v4 temp;
    
    temp.P = _mm_sub_ps(a.P, b.P);
    
    return temp;
}

static inline v4 v4_mul_ps(v4 a, v4 b)
{
    v4 temp;
    
    temp.P = _mm_mul_ps(a.P, b.P);
    
    return temp;
}

static inline v4 v4_div_ps(v4 a, v4 b)
{
    v4 temp;
    
    temp.P = _mm_div_ps(a.P, b.P);
    
    return temp;
}

static inline v4 v4_cmpge_ps(v4 a, v4 b)
{
    v4 temp;
    temp.P = _mm_cmpge_ps(a.P, b.P);
    return temp;
}

static inline v4 v4_cmple_ps(v4 a, v4 b)
{
    v4 temp;
    temp.P = _mm_cmple_ps(a.P, b.P);
    return temp;
}

static inline v4 v4_and_ps(v4 a, v4 b)
{
    v4 temp;
    temp.P = _mm_and_ps(a.P, b.P);
    return temp;
}

static inline v4 v4_add_s(v4 a, v4 b)
{
    v4 temp;
    
    temp.s.P = _mm_add_epi32(a.s.P, b.s.P);
    
    return temp;
}

static inline v4 v4_cvtps_epi32(v4 a)
{
    v4 temp;
    
    temp.s.P = _mm_cvtps_epi32(a.P);
    
    return temp;
}

#endif //WATCHTOGETHER_SIMD_H
