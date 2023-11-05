#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>
#include <string.h>

#ifdef ARRAY_IMPLEMENTATION
#  define IS_ARRAY_OF(typeL, type, prefix)				\
									\
  void prefix##_append_many( typeL * acc, type * xs, size_t xs_len) {	\
    size_t new_cap = (acc)->cap;					\
    while((acc)->len + xs_len >= new_cap) {				\
      new_cap *= 2;							\
      if(new_cap == 0) new_cap = 16;					\
    }									\
    if(new_cap != (acc)->cap) {						\
      (acc)->cap = new_cap;						\
      (acc)->items = realloc((acc)->items, (acc)->cap * sizeof(*((acc)->items))); \
      assert((acc)->items);						\
    }									\
    memcpy((acc)->items + (acc)->len, xs, xs_len * sizeof(*((acc)->items))); \
    (acc)->len += xs_len;						\
  }									\
  									\
  void prefix##_append( typeL * acc, type x ) {				\
    size_t new_cap = (acc)->cap;					\
    while((acc)->len >= new_cap) {					\
      new_cap *= 2;							\
      if(new_cap == 0) new_cap = 16;					\
    }									\
    if(new_cap != (acc)->cap) {						\
      (acc)->cap = new_cap;						\
      (acc)->items = realloc((acc)->items, (acc)->cap * sizeof(*((acc)->items))); \
      assert((acc)->items);						\
    }									\
    (acc)->items[(acc)->len++] = x;					\
  }									\
  
#else
#  define IS_ARRAY_OF(typeL, type, prefix)	\
  void prefix##_append_many( typeL * acc, type * xs, size_t xs_len);	\
  void prefix##_append( typeL * acc, type x )  
#endif // ARRAY_IMPLEMENTATION

#endif // ARRAY_H
