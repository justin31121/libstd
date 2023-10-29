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

#define return_defer(n) do{			\
    result = (n);				\
    goto defer;					\
  }while(0)

#define da_append_many(n, xs, xs_len) do{				\
    u64 new_cap = (n)->cap;						\
    while((n)->len + xs_len >= new_cap) {				\
      new_cap *= 2;							\
      if(new_cap == 0) new_cap = 16;					\
    }									\
    if(new_cap != (n)->cap) {						\
      (n)->cap = new_cap;						\
      (n)->items = realloc((n)->items, (n)->cap * sizeof(*((n)->items))); \
      assert((n)->items);						\
    }									\
    memcpy((n)->items + (n)->len, xs, xs_len * sizeof(*((n)->items)));	\
    (n)->len += xs_len;							\
  }while(0)
    

#define da_append(n, x)	do{						\
    u64 new_cap = (n)->cap;						\
    while((n)->len >= new_cap) {					\
      new_cap *= 2;							\
      if(new_cap == 0) new_cap = 16;					\
    }									\
    if(new_cap != (n)->cap) {						\
      (n)->cap = new_cap;						\
      (n)->items = realloc((n)->items, (n)->cap * sizeof(*((n)->items))); \
      assert((n)->items);						\
    }									\
    (n)->items[(n)->len++] = x;						\
  }while(0)

#define errorf(...) do{						\
    fflush(stdout);						\
    fprintf(stderr, "%s:%d:ERROR: ", __FILE__, __LINE__);	\
    fprintf(stderr,  __VA_ARGS__ );				\
    fprintf(stderr, "\n");					\
    fflush(stderr);						\
  }while(0)

#define panicf(...) do{						\
    errorf(__VA_ARGS__);					\
    exit(1);							\
  }while(0)

#endif // TYPES_H

#ifndef JV_H
#define JV_H

#ifndef JV_DEF
#  define JV_DEF static inline
#endif // JV_DEF

#include <assert.h>
#include <string.h>
#include <ctype.h>

typedef enum{
  JSON_VIEW_TYPE_NULL,
  JSON_VIEW_TYPE_TRUE,
  JSON_VIEW_TYPE_FALSE,
  JSON_VIEW_TYPE_NUMBER,
  JSON_VIEW_TYPE_STRING,
  JSON_VIEW_TYPE_ARRAY,
  JSON_VIEW_TYPE_OBJECT,
}Json_View_Type;

typedef struct{
  const char *data;
  u64 len;
  Json_View_Type type;
}Json_View;

#define jv_from(d, l, t) (Json_View) { (d), (l), (t) }

JV_DEF bool jv_parse(Json_View *view, const char *data, u64 len);
JV_DEF bool jv_parse_number(Json_View *view, const char *data, u64 len);
JV_DEF bool jv_parse_string(Json_View *view, const char *data, u64 len);
JV_DEF bool jv_parse_array(Json_View *view, const char *data, u64 len);
JV_DEF bool jv_parse_object(Json_View *view, const char *data, u64 len);
JV_DEF bool jv_parse_impl(Json_View *view, const char *data, u64 len, const char *target, u64 target_len, Json_View_Type type);

JV_DEF bool jv_array_next(Json_View *array, Json_View *sub_view);
JV_DEF bool jv_object_next(Json_View *object, Json_View *key_view, Json_View *value_view);

#ifdef JV_IMPLEMENTATION

static char jv_data_null[] = "null";
#define jv_parse_null(v, d, l) jv_parse_impl((v), (d), (l), jv_data_null, sizeof(jv_data_null) - 1, JSON_VIEW_TYPE_NULL)

static char jv_data_false[] = "false";
#define jv_parse_false(v, d, l) jv_parse_impl((v), (d), (l), jv_data_false, sizeof(jv_data_false) - 1, JSON_VIEW_TYPE_FALSE)

static char jv_data_true[] = "true";
#define jv_parse_true(v, d, l) jv_parse_impl((v), (d), (l), jv_data_true, sizeof(jv_data_true) - 1, JSON_VIEW_TYPE_TRUE)

JV_DEF bool jv_parse(Json_View *view, const char *data, u64 len) {

  if(len == 0) return false;

  if(*data == 'n') return jv_parse_null(view, data, len);
  else if(*data == 'f') return jv_parse_false(view, data, len);
  else if(*data == 't') return jv_parse_true(view, data, len);
  else if(*data == '\"') return jv_parse_string(view, data, len);
  else if(*data == '[') return jv_parse_array(view, data, len);
  else if(*data == '{') return jv_parse_object(view, data, len);
  else if(isdigit(*data)) return jv_parse_number(view, data, len);
  else return false;

}

JV_DEF bool jv_parse_number(Json_View *view, const char *data, u64 len) {

  u64 i = 1;
  while(true) {
    if(i >= len) return false;
    if(!isdigit(data[i])) break;
    i++;
  }
  *view = jv_from(data, i, JSON_VIEW_TYPE_NUMBER);
  
  return true;
}

JV_DEF bool jv_parse_string(Json_View *view, const char *data, u64 len) {
  
  u64 i = 1;
  while(true) {
    if(i >= len) return false;
    if(data[i] == '\"') break;

    if(data[i] == '\\') {
      i++;      
    }
    
    i++;
  }
  *view = jv_from(data, i + 1, JSON_VIEW_TYPE_STRING);
  
  return true;
}

JV_DEF bool jv_parse_array(Json_View *view, const char *data, u64 len) {

  u64 i = 1;
  while(true) {

    // whitespace
    while(true) {
      if(i >= len) return false;
      if(!isspace(data[i])) break;
      i++;
    }

    if(i >= len) return false;
    if(data[i] == ']') break;

    Json_View sub_view;
    if(!jv_parse(&sub_view, data + i, len - i)) {
      return false;
    }
    i += sub_view.len;

    // whitespace
    while(true) {
      if(i >= len) return false;
      if(!isspace(data[i])) break;
      i++;
    }

    if(i >= len) return false;
    if(data[i] == ']') break;
    if(data[i] != ',') return false;
    i++;
  }
  *view = jv_from(data, i + 1, JSON_VIEW_TYPE_ARRAY);
  
  return true;
}

JV_DEF bool jv_parse_object(Json_View *view, const char *data, u64 len) {
  
  u64 i = 1;
  while(true) {

    // whitespace
    while(true) {
      if(i >= len) return false;
      if(!isspace(data[i])) break;
      i++;
    }

    if(i >= len) return false;
    if(data[i] == '}') break;

    Json_View sub_key_view;
    if(!jv_parse(&sub_key_view, data + i, len - i)) {
      return false;
    }
    i += sub_key_view.len;
    if(sub_key_view.type != JSON_VIEW_TYPE_STRING) return false;

    // whitespace
    while(true) {
      if(i >= len) return false;
      if(!isspace(data[i])) break;
      i++;
    }

    if(i >= len) return false;
    if(data[i] != ':') return false;
    i++;

    // whitespace
    while(true) {
      if(i >= len) return false;
      if(!isspace(data[i])) break;
      i++;
    }

    Json_View sub_value_view;
    if(!jv_parse(&sub_value_view, data + i, len - i)) {
      return false;
    }
    i += sub_value_view.len;

    // whitespace
    while(true) {
      if(i >= len) return false;
      if(!isspace(data[i])) break;
      i++;
    }

    if(i >= len) return false;
    if(data[i] == '}') break;
    if(data[i] != ',') return false;
    i++;
  }
  *view = jv_from(data, i + 1, JSON_VIEW_TYPE_OBJECT);

  return true;
}

JV_DEF bool jv_parse_impl(Json_View *view, const char *data, u64 len,
			  const char *target, u64 target_len, Json_View_Type type) {
  
  if(len != target_len) return false;
  if(memcmp(data, target, target_len) != 0) return false;
  *view = jv_from(data, len, type);
  
  return true;
}

JV_DEF bool jv_array_next(Json_View *array, Json_View *sub_view) {
  assert(array->type == JSON_VIEW_TYPE_ARRAY);

  u64 i = 0;
  while(true) {
    if(i >= array->len) return false;
    if(array->data[i] == ']') return false;    
    if(array->data[i] == '[') {
      i++;
      continue;
    }

    // whitespace
    while(true) {
      if(i >= array->len) return false;
      if(!isspace(array->data[i])) break;
      i++;
    }

    if(!jv_parse(sub_view, array->data + i, array->len - i)) {
      return false;
    }
    i += sub_view->len;

    // whitespace
    while(true) {
      if(i >= array->len) return false;
      if(!isspace(array->data[i])) break;
      i++;
    }

    if(i >= array->len) return false;
    if(array->data[i] == ']' || array->data[i] == ',') {
      array->data += i + 1;
      array->len  -= i + 1;
      
      return true;
    } else {
      return false;
    }
    
  }

  return false;
}

JV_DEF bool jv_object_next(Json_View *object, Json_View *key_view, Json_View *value_view) {
  assert(object->type == JSON_VIEW_TYPE_OBJECT);

  u64 i = 0;
  while(true) {
    if(i >= object->len) return false;
    if(object->data[i] == '}') return false;    
    if(object->data[i] == '{') {
      i++;
      continue;
    }

    // whitespace
    while(true) {
      if(i >= object->len) return false;
      if(!isspace(object->data[i])) break;
      i++;
    }

    if(!jv_parse(key_view, object->data + i, object->len - i)) {
      return false;
    }
    if(key_view->type != JSON_VIEW_TYPE_STRING) return false;
    i += key_view->len;

    // whitespace
    while(true) {
      if(i >= object->len) return false;
      if(!isspace(object->data[i])) break;
      i++;
    }

    if(i >= object->len) return false;
    if(object->data[i] != ':') return false;
    i++;

    // whitespace
    while(true) {
      if(i >= object->len) return false;
      if(!isspace(object->data[i])) break;
      i++;
    }

    if(!jv_parse(value_view, object->data + i, object->len - i)) {
      return false;
    }
    i += value_view->len;

    // whitespace
    while(true) {
      if(i >= object->len) return false;
      if(!isspace(object->data[i])) break;
      i++;
    }

    if(i >= object->len) return false;
    if(object->data[i] == '}' || object->data[i] == ',') {
      object->data += i + 1;
      object->len  -= i + 1;
      
      return true;
    } else {
      return false;
    }
  }

  return false;
}
  
#endif // JV_IMPLEMENTATION

#endif // JV_H
