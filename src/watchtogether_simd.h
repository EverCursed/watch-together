#ifndef WATCHTOGETHER_SIMD_H
#define WATCHTOGETHER_SIMD_H

#include "defines.h"

typedef struct v4 {
    union {
        __m128 PS;
        __m128i EPI32;
        f32 F32[4];
        i32 I32[4];
        u32 U32[4];
    };
} v4;

typedef struct v4x4 {
    union {
        struct {
            union {
                v4 a;
                v4 x;
            };
            union {
                v4 b;
                v4 y;
            };
            union {
                v4 c;
                v4 z;
            };
            union {
                v4 d;
                v4 w;
            };
        };
        v4 E[4];
    };
} v4x4;

internal void v4_print(v4 a)
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
           a.F32[0], a.F32[1], a.F32[2], a.F32[3],
           a.I32[0], a.I32[1], a.I32[2], a.I32[3]);
}

internal v4 v4_set1_ps(f32 a)
{
    return (v4){ .PS = _mm_set1_ps(a) };
}

internal v4 v4_set_ps(f32 a, f32 b, f32 c, f32 d)
{
    return (v4){ .PS = _mm_set_ps(a, b, c, d) };
}

internal v4 v4_add_ps(v4 a, v4 b)
{
    return (v4){ .PS = _mm_add_ps(a.PS, b.PS) };
}

internal v4 v4_sub_ps(v4 a, v4 b)
{
    v4 temp;
    
    temp.PS = _mm_sub_ps(a.PS, b.PS);
    
    return temp;
}

internal v4 v4_mul_ps(v4 a, v4 b)
{
    v4 temp;
    
    temp.PS = _mm_mul_ps(a.PS, b.PS);
    
    return temp;
}

internal v4 v4_div_ps(v4 a, v4 b)
{
    v4 temp;
    
    temp.PS = _mm_div_ps(a.PS, b.PS);
    
    return temp;
}

internal v4 v4_cmpge_ps(v4 a, v4 b)
{
    v4 temp;
    temp.PS = _mm_cmpge_ps(a.PS, b.PS);
    return temp;
}

internal v4 v4_cmple_ps(v4 a, v4 b)
{
    v4 temp;
    temp.PS = _mm_cmple_ps(a.PS, b.PS);
    return temp;
}

internal v4 v4_and_ps(v4 a, v4 b)
{
    v4 temp;
    temp.PS = _mm_and_ps(a.PS, b.PS);
    return temp;
}

internal v4 v4_add_epi32(v4 a, v4 b)
{
    v4 temp;
    
    temp.EPI32 = _mm_add_epi32(a.EPI32, b.EPI32);
    
    return temp;
}

internal v4 v4_cvtps_epi32(v4 a)
{
    v4 temp;
    
    temp.EPI32 = _mm_cvtps_epi32(a.PS);
    
    return temp;
}



#endif //WATCHTOGETHER_SIMD_H
