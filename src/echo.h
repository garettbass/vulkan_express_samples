#pragma once

#ifdef __cplusplus

    #include <cstdio>

    static inline int
    _echo(const char* x, bool v)
    { return printf("%s: %s", x, v?"true":"false"); }

    static inline int
    _echo(const char* x, char v)
    { return printf("%s: '%c'", x, v); }

    static inline int
    _echo(const char* x, signed char v)
    { return printf("%s: %hhi", x, v); }

    static inline int
    _echo(const char* x, signed short v)
    { return printf("%s: %hi", x, v); }

    static inline int
    _echo(const char* x, signed int v)
    { return printf("%s: %i", x, v); }

    static inline int
    _echo(const char* x, signed long int v)
    { return printf("%s: %li", x, v); }

    static inline int
    _echo(const char* x, signed long long int v)
    { return printf("%s: %lli", x, v); }

    static inline int
    _echo(const char* x, unsigned char v)
    { return printf("%s: %hhu", x, v); }

    static inline int
    _echo(const char* x, unsigned short v)
    { return printf("%s: %hu", x, v); }

    static inline int
    _echo(const char* x, unsigned int v)
    { return printf("%s: %u", x, v); }

    static inline int
    _echo(const char* x, unsigned long int v)
    { return printf("%s: %lu", x, v); }

    static inline int
    _echo(const char* x, unsigned long long int v)
    { return printf("%s: %llu", x, v); }

    static inline int
    _echo(const char* x, float v)
    { return printf("%s: %g", x, v); }

    static inline int
    _echo(const char* x, double v)
    { return printf("%s: %g", x, v); }

    static inline int
    _echo(const char* x, long double v)
    { return printf("%s: %Lg", x, v); }

    static inline int
    _echo(const char* x, const char* v)
    { return printf("%s: \"%s\"", x, v); }

    static inline int
    _echo(const char* x, const void* v)
    { return printf("%s: %p", x, v); }

    #define _echo(expr) (_echo(#expr, (expr)))

#else // C11

    #include <stdbool.h>
    #include <stdio.h>

    static inline int
    __echo_bool(const char* x, bool v)
    { return printf("%s: %s", x, v?"true":"false"); }

    static inline int
    __echo_char(const char* x, char v)
    { return printf("%s: '%c'", x, v); }

    static inline int
    __echo_schar(const char* x, signed char v)
    { return printf("%s: %hhi", x, v); }

    static inline int
    __echo_short(const char* x, short v)
    { return printf("%s: %hi", x, v); }

    static inline int
    __echo_int(const char* x, int v)
    { return printf("%s: %i", x, v); }

    static inline int
    __echo_long(const char* x, long v)
    { return printf("%s: %li", x, v); }

    static inline int
    __echo_long_long(const char* x, long long v)
    { return printf("%s: %lli", x, v); }

    static inline int
    __echo_uchar(const char* x, unsigned char v)
    { return printf("%s: %hhu", x, v); }

    static inline int
    __echo_ushort(const char* x, unsigned short v)
    { return printf("%s: %hu", x, v); }

    static inline int
    __echo_uint(const char* x, unsigned int v)
    { return printf("%s: %u", x, v); }

    static inline int
    __echo_ulong(const char* x, unsigned long v)
    { return printf("%s: %lu", x, v); }

    static inline int
    __echo_ulong_long(const char* x, unsigned long long v)
    { return printf("%s: %llu", x, v); }

    static inline int
    __echo_float(const char* x, float v)
    { return printf("%s: %g", x, v); }

    static inline int
    __echo_double(const char* x, double v)
    { return printf("%s: %g", x, v); }

    static inline int
    __echo_long_double(const char* x, long double v)
    { return printf("%s: %Lg", x, v); }

    static inline int
    __echo_cstr(const char* x, const char* v)
    { return printf("%s: \"%s\"", x, v); }

    static inline int
    __echo_ptr(const char* x, const void* v)
    { return printf("%s: %p", x, v); }

    #define _echo(expr)\
            (_Generic((expr),\
                bool:                   __echo_bool,\
                char:                   __echo_char,\
                /* signed */\
                signed char:            __echo_schar,\
                signed short:           __echo_short,\
                signed int:             __echo_int,\
                signed long:            __echo_long,\
                signed long long:       __echo_long_long,\
                /* unsigned */\
                unsigned char:          __echo_uchar,\
                unsigned short:         __echo_ushort,\
                unsigned int:           __echo_uint,\
                unsigned long:          __echo_ulong,\
                unsigned long long:     __echo_ulong_long,\
                /* floating point */\
                float:                  __echo_float,\
                double:                 __echo_double,\
                long double:            __echo_long_double,\
                /* pointers */\
                const char*:            __echo_cstr,\
                char*:                  __echo_cstr,\
                default:                __echo_ptr\
            )(#expr, expr))

#endif

//------------------------------------------------------------------------------

#define echo(...) _echo_each(_echo,_echo_comma,_echo_newline, __VA_ARGS__)

#define _echo_comma() ,fputs(", ", stdout),

#define _echo_newline() ,fputc('\n', stdout)

//------------------------------------------------------------------------------

#define _echo_each(X, S, T, ...)\
        __echo_each_(_echo_each_count(__VA_ARGS__))(X, S, T, __VA_ARGS__)

#define __echo_each_(count) ___echo_each_(count)
#define ___echo_each_(count) __echo_each_##count

#define _echo_each_count(...)\
        __echo_each_count(__VA_ARGS__,\
            16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,end)
#define __echo_each_count(\
            _16,_15,_14,_13,_12,_11,_10,_9,_8,_7,_6,_5,_4,_3,_2,_1,X,...) X

#define __echo_each_1(X, S, T, _1)\
        X(_1)T()
#define __echo_each_2(X, S, T, _1, _2)\
        X(_1)S()X(_2)T()
#define __echo_each_3(X, S, T, _1, _2, _3)\
        X(_1)S()X(_2)S()X(_3)T()
#define __echo_each_4(X, S, T, _1, _2, _3, _4)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)T()
#define __echo_each_5(X, S, T, _1, _2, _3, _4, _5)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)T()
#define __echo_each_6(X, S, T, _1, _2, _3, _4, _5, _6)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)T()
#define __echo_each_7(X, S, T, _1, _2, _3, _4, _5, _6, _7)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)T()
#define __echo_each_8(X, S, T, _1, _2, _3, _4, _5, _6, _7, _8)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)S()X(_8)T()
#define __echo_each_9(X, S, T, _1, _2, _3, _4, _5, _6, _7, _8, _9)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)S()X(_8)S()X(_9)T()
#define __echo_each_10(X, S, T, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)S()X(_8)S()X(_9)S()X(_10)T()
#define __echo_each_11(X, S, T, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)S()X(_8)S()X(_9)S()X(_10)S()X(_11)T()
#define __echo_each_12(X, S, T, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)S()X(_8)S()X(_9)S()X(_10)S()X(_11)S()X(_12)T()
#define __echo_each_13(X, S, T, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)S()X(_8)S()X(_9)S()X(_10)S()X(_11)S()X(_12)S()X(_13)T()
#define __echo_each_14(X, S, T, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)S()X(_8)S()X(_9)S()X(_10)S()X(_11)S()X(_12)S()X(_13)S()X(_14)T()
#define __echo_each_15(X, S, T, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)S()X(_8)S()X(_9)S()X(_10)S()X(_11)S()X(_12)S()X(_13)S()X(_14)S()X(_15)T()
#define __echo_each_16(X, S, T, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16)\
        X(_1)S()X(_2)S()X(_3)S()X(_4)S()X(_5)S()X(_6)S()X(_7)S()X(_8)S()X(_9)S()X(_10)S()X(_11)S()X(_12)S()X(_13)S()X(_14)S()X(_15)S()X(_16)T()

//------------------------------------------------------------------------------
