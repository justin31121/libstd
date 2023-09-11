#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdint.h>

typedef uint8_t u8;
typedef char s8; // because of mingw warning not 'int8_t'
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

typedef float f32;
typedef double f64;

#define return_defer(n) do{			\
    result = (n);				\
    goto defer;					\
  }while(0)

#define errorf(...) do{						\
    fflush(stdout);						\
    fprintf(stderr, "%s:%d:ERROR: ", __FILE__, __LINE__);	\
    fprintf(stderr,  __VA_ARGS__ );				\
    fprintf(stderr, "\n");					\
    fflush(stderr);						\
  }while(0)

#define panicf(...) do{						\
    fflush(stdout);						\
    fprintf(stderr, "%s:%d:ERROR: ", __FILE__, __LINE__);	\
    fprintf(stderr,  __VA_ARGS__ );				\
    fprintf(stderr, "\n");					\
    fflush(stderr);						\
    exit(1);							\
  }while(0)

#endif TYPES_H
