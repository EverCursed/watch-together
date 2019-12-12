#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

// compilation
#define warn_unused        __attribute__((warn_unused_result))
#define not_used           __attribute__((unused))
//#define packed               __attribute__((packed))
#define align(v)           __attribute__ ((aligned(v)))

// branch prediction
#define likely(x)          __builtin_expect((x),1)
#define unlikely(x)        __builtin_expect((x),0)
#define pct_likely(x, pct) __builtin_expect_with_probability((x),1,pct)

#endif // ATTRIBUTES_H
