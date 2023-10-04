#ifndef EBML_H
#define EBML_H

//https://datatracker.ietf.org/doc/rfc8794/
//https://www.matroska.org/technical/elements.html

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
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

#ifndef EBML_DEF
#  define EBML_DEF static inline
#endif // EBML_DEF

#ifndef EBML_LOG
#  ifdef EBML_QUIET
#    define EBML_LOG(...)
#  else
#    include <stdio.h>
#    define EBML_LOG(...) fprintf(stderr, "EBML_LOG: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#  endif // EBML_QUIET
#endif // EBML_QUIET

typedef enum{
  EBML_TYPE_NONE = 0,
  EBML_TYPE_MASTER,
  EBML_TYPE_UINT,
  EBML_TYPE_STRING,
}Ebml_Type;

#define EBML_TABLE				\
  EBML_ENTRY(0x1A45DFA3, MASTER, EBML)		\
  EBML_ENTRY(0x4286, UINT, EBMLVersion)		\
  EBML_ENTRY(0x42F7, UINT, EBMLReadVersion)	\
  EBML_ENTRY(0x42f2, UINT, EBMLMaxIDLength)	\
  EBML_ENTRY(0x42f3, UINT, EBMLMaxSizeLength)	\
  EBML_ENTRY(0x4282, STRING, DocType)		\
  EBML_ENTRY(0x4287, UINT, DocTypeVersion)	\
  EBML_ENTRY(0x4285, UINT, DocTypeReadVersion)	\

typedef enum {
  EBML_ID_NONE = 0,
  
#define EBML_ENTRY(id, type, name) EBML_ID_##name,
  EBML_TABLE
#undef EBML_ENTRY
  
}Ebml_Id;

typedef struct{
  Ebml_Type type;
  Ebml_Id id;
}Ebml_Elem;

typedef struct{
  u8 *data;
  u64 len;
}Ebml;

#define ebml_from(d, l) (Ebml) {(d), (l)}

EBML_DEF bool ebml_next(Ebml *e, u64 *size, Ebml_Elem *elem);
EBML_DEF const char *ebml_id_name(Ebml_Id id);
EBML_DEF const char *ebml_type_name(Ebml_Type type);

#ifdef EBML_IMPLEMENTATION

EBML_DEF bool ebml_next(Ebml *e, u64 *size, Ebml_Elem *elem) {

  // READ id
  if(e->len == 0) {
    return false;
  }

  u8 b = *e->data;

  u8 bit = 1 << 7;
  u8 i=1;
  for(;i<=8;i++) {
    if(b & bit) break;
    bit >>= 1;
  }

  if(e->len < i) {
    return false;
  }
  
  u64 id = *e->data;
  for(u8 j=1;j<i;j++) {
    id <<= 8;
    id += e->data[j];
  }
  e->data += i;
  e->len  -= i;

  switch(id) {
#define EBML_ENTRY(ebml_id, ebml_type, ebml_name)	\
    case (ebml_id): {					\
      elem->id = (EBML_ID_##ebml_name);			\
      elem->type = (EBML_TYPE_##ebml_type);		\
    } break;
    EBML_TABLE
#undef EBML_ENTRY
  default: {
      EBML_LOG("Unknown id: 0x%llx", id);
      return false;
    } break;
  }

  //READ size
  if(e->len == 0) {
    return false;
  }

  b = *e->data;

  bit = 1 << 7;
  i=1;
  for(;i<=8;i++) {
    if(b & bit) break;
    bit >>= 1;
  }

  if(e->len < i) {
    return false;
  }
  
  *size = *e->data & ~bit;
  for(u8 j=1;j<i;j++) {
    *size <<= 8;
    *size += e->data[j];
  }
  e->data += i;
  e->len  -= i;

  // LOOKUP id

  e->data += *size;
  e->len  -= *size;
  
  return true;
}

EBML_DEF const char *ebml_id_name(Ebml_Id id) {
  switch(id) {
#define EBML_ENTRY(ebml_id, ebml_type, ebml_name) case EBML_ID_##ebml_name: return #ebml_name;
    EBML_TABLE
#undef EBML_ENTRY
  }

  return NULL;
}

EBML_DEF const char *ebml_type_name(Ebml_Type type) {
  switch(type) {
  case EBML_TYPE_NONE: return "NONE";
  case EBML_TYPE_MASTER: return "Master Element";
  case EBML_TYPE_UINT: return "Unsigned Integer";
  case EBML_TYPE_STRING: return "String";
  }

  return NULL;
}

#endif // EBML_IMPLEMENTATION

#endif // EBML_H
