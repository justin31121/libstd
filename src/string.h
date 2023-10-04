#ifndef STRING_H
#define STRING_H

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

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

#endif // TYPES_H

#ifndef STRING_DEF
#  define STRING_DEF static inline
#endif // STRING_DEF

typedef struct{
  const char *data;
  u64 len;
}string;


#define str_fmt "%.*s"
#define str_arg(s) (int) (s).len, (s).data

#define string_from(d, l) (string) { (d), (l)}
#define string_from_cstr(cstr) (string) { (cstr), strlen(cstr) }
#define string_from_cstr2(cstr, cstr_len) (string) { (cstr), (cstr_len) }
#define string_to_cstr(s) ((char *) (memcpy(memset(_alloca((s).len + 1), 0, s.len + 1), (s).data, (s).len)) )
string string_from_u64(u64 n);

#define string_free(s) free((char *) (s).data)

STRING_DEF bool string_copy(string s, string *d);
STRING_DEF bool string_copy_cstr(const char *cstr, string *d);
STRING_DEF bool string_copy_cstr2(const char *cstr, u64 cstr_len, string *d);

STRING_DEF s32 string_index_of(string s, const char *needle);
STRING_DEF s32 string_index_of_off(string s, u64 off, const char *needle);
STRING_DEF bool string_substring(string s, u64 start, u64 len, string *d);
STRING_DEF bool string_chop_by(string *s, const char *delim, string *d);

#define STRING_BUILDER_DEFAULT_CAP 256

typedef struct{
  char *data;
  u64 len;
  u64 cap;
}string_builder;

#define string_builder_free(sb) free((sb).data);

STRING_DEF bool string_builder_append(string_builder *sb, const char *data, size_t data_len);
STRING_DEF bool string_builder_to_string(string_builder *sb, string *s);

#ifdef STRING_IMPLEMENTATION

#define STRING_FROM_U_CAP 32

static string string_from_u64_impl(char *space, u64 n) {
  u64 m = snprintf(space, STRING_FROM_U_CAP, "%llu", n);
  return (string) { space, m };
}

#define string_from_u64(n) string_from_u64_impl(_alloca(STRING_FROM_U_CAP), n)

STRING_DEF bool string_copy(string s, string *d) {
  return string_copy_cstr2(s.data, s.len, d);
}

STRING_DEF bool string_copy_cstr(const char *cstr, string *d) {
  return string_copy_cstr2(cstr, strlen(cstr), d);
}

STRING_DEF bool string_copy_cstr2(const char *cstr, u64 cstr_len, string *d) {
  char *data = malloc(cstr_len);
  if(!data) return false;
  memcpy(data, cstr, cstr_len);
  d->data = (const char *) data;
  d->len  = cstr_len;
  return true;
}

#define string_substring_impl(s, start, len) (string) { (s).data + start, len }

static int string_index_of_impl(const char *haystack, u64 haystack_size, const char* needle, u64 needle_size) {
  if(needle_size > haystack_size) {
    return -1;
  }
  haystack_size -= needle_size;
  u64 i, j;
  for(i=0;i<=haystack_size;i++) {
    for(j=0;j<needle_size;j++) {
      if(haystack[i+j] != needle[j]) {
	break;
      }
    }
    if(j == needle_size) {
      return (int) i;
    }
  }
  return -1;

}

STRING_DEF s32 string_index_of_off(string s, u64 off, const char *needle) {

  if(off > s.len) {
    return -1;
  }
  
  s32 pos = string_index_of_impl(s.data + off, s.len - off, needle, strlen(needle));
  if(pos < 0) {
    return -1;
  }

  return pos + (s32) off;
}

STRING_DEF s32 string_index_of(string s, const char *needle) {  
  return string_index_of_impl(s.data, s.len, needle, strlen(needle));
}

STRING_DEF bool string_chop_by(string *s, const char *delim, string *d) {
  if(!s->len) return false;
  
  s32 pos = string_index_of(*s, delim);
  if(pos < 0) pos = (int) s->len;
    
  if(d && !string_substring(*s, 0, pos, d))
    return false;

  if(pos == (int) s->len) {
    *d = *s;
    s->len = 0;
    return true;
  } else {
    return string_substring(*s, pos + 1, s->len - pos - 1, s);
  }

}

STRING_DEF bool string_substring(string s, u64 start, u64 len, string *d) {

  if(start > s.len) {
    return false;
  }

  if(start + len > s.len) {
    return false;
  }

  *d = string_substring_impl(s, start, len);
  
  return true;
}

STRING_DEF bool string_builder_append(string_builder *sb, const char *data, size_t data_len) {
  u64 cap = sb->cap;
  if(cap == 0) cap = STRING_BUILDER_DEFAULT_CAP;
  while(sb->len + data_len > cap) {
    cap *= 2;
  }
  if(cap != sb->cap) {
    sb->cap = cap;
    sb->data = realloc(sb->data, sb->cap);
    if(!sb->data) return false;
  }
  memcpy(sb->data + sb->len, data, data_len);
  sb->len += data_len;
  return true;
}

STRING_DEF bool string_builder_to_string(string_builder *sb, string *d) {
  return string_copy_cstr2(sb->data, sb->len, d);
}

#endif // STRING_IMPLEMENTATION

#endif // STRING_H
