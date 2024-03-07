#ifndef _STRING_H
#define _STRING_H

// MIT License
// 
// Copyright (c) 2024 Justin Schartner
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

typedef unsigned char string_u8;
typedef unsigned int string_u32;
typedef int string_s32;
typedef long long string_s64;
typedef unsigned long long string_u64;

#define u8 string_u8
#define u32 string_u32
#define s32 string_s32
#define s64 string_s64
#define u64 string_u64

#ifndef STRING_DEF
#  define STRING_DEF static inline
#endif // STRING_DEF

#ifndef STRING_ASSERT
#  include <assert.h>
#  define STRING_ASSERT assert
#endif // STRING_ASSERT

#ifndef STRING_ALLOC
#  include <stdlib.h>
#  define STRING_ALLOC malloc
#endif // STRING_ALLOC

#ifndef STRING_FREE
#  include <stdlib.h>
#  define STRING_FREE free
#endif // STRING_FREE

STRING_DEF u64 string_cstrlen(u8 *cstr);
STRING_DEF s32 string_memcmp(const void *a, const void *b, u64 len);
STRING_DEF void *string_memcpy(void *dst, const void *src, u64 len);

typedef struct{
  u8 *data;
  u64 len;
}string;

#define str_fmt "%.*s"
#define str_arg(s) (int) (s).len, (s).data

#define string_from(d, l) (string) { .data = (d), .len = (l) }
#define string_fromc(cstr) string_from((cstr), string_cstrlen(cstr))

STRING_DEF bool string_eq(string a, string b);
#define string_eqc(s, cstr) string_eq((s), string_fromc(cstr))
STRING_DEF bool string_parse_s64(string s, s64 *n);

STRING_DEF s32 string_index_of(string s, string needle);
STRING_DEF s32 string_index_of_off(string s, u64 off, string needle);
#define string_index_ofc(s, cstr) string_index_of((s), string_fromc(cstr))
#define string_index_of_offc(s, off, cstr) string_index_of_off((s), (off), string_fromc(cstr))

STRING_DEF bool string_chop_by(string *s, char *delim, string *d);

#ifndef STRING_BUILDER_DEFAULT_CAP
#  define STRING_BUILDER_DEFAULT_CAP 1024
#endif // STRING_BUILDER_DEFAULT_CAP

typedef struct{
  u8 *data;
  u64 len;
  u64 cap;
}string_builder;

STRING_DEF void string_builder_reserve(string_builder *sb, u64 cap);

STRING_DEF void string_builder_append(string_builder *sb, const u8 *data, u64 len);
#define string_builder_appendc(sb, cstr) string_builder_append((sb), (cstr), string_cstrlen(cstr))
#define string_builder_appends(sb, s) string_builder_append((sb), (s).data, (s).len)
STRING_DEF void string_builder_appends64(string_builder *sb, s64 n);

typedef u32 Rune;

STRING_DEF Rune rune_decode(const u8 **data, u64 *data_len);
STRING_DEF void rune_encode(Rune rune, u8 buf[4], u64 *buf_len);
STRING_DEF Rune rune_unescape(u8 **data, u64 *data_len);
STRING_DEF Rune rune_decode(const u8 **data, u64 *data_len);

#ifdef STRING_IMPLEMENTATION

STRING_DEF u64 string_cstrlen(u8 *cstr) {
  u64 len = 0;
  while(*cstr) {
    len++;
    cstr++;
  }
  return len;
}

STRING_DEF s32 string_memcmp(const void *a, const void *b, u64 len) {
  const u8 *pa = a;
  const u8 *pb = b;

  s32 d = 0;
  while(!d && len) {
    d = *pa++ - *pb++;
    len--;
  }

  return d;
}

STRING_DEF void *string_memcpy(void *_dst, const void *_src, u64 len) {
  u8 *dst = _dst;
  const u8 *src = _src;
  for(u64 i=0;i<len;i++) {
    *dst++ = *src++;
  }
  return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////

STRING_DEF bool string_eq(string a, string b) {
  if(a.len != b.len) {
    return false;
  }
  return string_memcmp(a.data, b.data, a.len) == 0;
}

STRING_DEF bool string_parse_s64(string s, s64 *n) {
  u64 i=0;
  s64 sum = 0;
  s32 negative = 0;

  const char *data = s.data;
  
  if(s.len && data[0]=='-') {
    negative = 1;
    i++;
  }  
  while(i<s.len &&
	'0' <= data[i] && data[i] <= '9') {
    sum *= 10;
    s32 digit = data[i] - '0';
    sum += digit;
    i++;
  }

  if(negative) sum*=-1;
  *n = sum;
  
  return i==s.len;
}

static s32 string_index_of_impl(const char *haystack, u64 haystack_size, const char* needle, u64 needle_size) {
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

STRING_DEF s32 string_index_of(string s, string needle) {
  return string_index_of_impl(s.data, s.len, needle.data, needle.len);
}

STRING_DEF s32 string_index_of_off(string s, u64 off, string needle) {
  if(off > s.len) {
    return - 1;
  }

  s32 pos = string_index_of_impl(s.data + off, s.len - off, needle.data, needle.len);
  if(pos < 0) {
    return -1;
  }

  return pos + (s32) off;
}

STRING_DEF bool string_chop_by(string *s, char *delim, string *d) {
  if(!s->len) return false;
  
  s32 pos = string_index_ofc(*s, delim);
  if(pos < 0) pos = (int) s->len;
      
  if(d) {
    *d = string_from(s->data, pos);
  }

  if(pos == (int) s->len) {
    *d = *s;
    s->len = 0;
    return true;
  } else {
    *s = string_from(s->data + pos + 1, s->len - pos - 1);
    return true;
  }

}

STRING_DEF void string_builder_reserve(string_builder *sb, u64 needed_cap) {
  u64 cap;
  if(sb->cap == 0) {
    cap = STRING_BUILDER_DEFAULT_CAP;
  } else {
    cap = sb->cap;
  }
    
  while(cap < needed_cap) {
    cap *= 2;
  }

  if(cap == sb->cap) {
    // nothing to do here
    
  } else {
    u8 *new_data = STRING_ALLOC(cap);
    string_memcpy(new_data, sb->data, sb->len);
    if(sb->cap != 0) {
      STRING_FREE(sb->data);
    }
    sb->data = new_data;
    sb->cap = cap;

  }
  
  
}

STRING_DEF void string_builder_append(string_builder *sb, const u8 *data, u64 len) {
  string_builder_reserve(sb, sb->len + len);
  string_memcpy(sb->data + sb->len, data, len);
  sb->len += len;
}

STRING_DEF void string_builder_appends64(string_builder *sb, s64 n) {
  char buf[32];
  u64 index = 31;  

  if(n == 0) {
    buf[index--] = '0';
  } else {

    boolean append_minus;
    if(n < 0) {
      append_minus = true;
      n *= -1;
    } else {
      append_minus = false;
    }
    
    while(n > 0) {
      buf[index--] = (n % 10) + '0';
      n = n / 10;
    }

    if(append_minus) {
      buf[index--] = '-';
    }
  }

  u64 len = sizeof(buf) - index - 1;
  string_builder_append(sb, &buf[index + 1], len);
}

STRING_DEF Rune rune_decode(const u8 **data, u64 *data_len) {
  u8 c = (*data)[0];

  Rune rune;
  if((0xf0 & c) == 0xf0) {    
    rune = ((Rune) (c & 0x07)) << 18;
    rune |= ((Rune) ( (*data)[1] & 0x3f )) << 12;
    rune |= ((Rune) ( (*data)[2] & 0x3f )) <<  6;
    rune |= (Rune) ( (*data)[3] & 0x3f );
      
    *data = *data + 4;
    *data_len = *data_len - 4;
  } else if((0xe0 & c) == 0xe0) {

    rune = ((Rune) (c & 0x0f)) << 12;
    rune |= ((Rune) ( (*data)[1] & 0x3f )) << 6;
    rune |= (Rune) ( (*data)[2] & 0x3f );

    *data = *data + 3;
    *data_len = *data_len - 3;
  } else if((0xc0 & c) == 0xc0) {
    
    rune = ((Rune) (c & 0x1f)) << 6;
    rune |= (Rune) ( (*data)[1] & 0x3f );

    *data = *data + 2;
    *data_len = *data_len - 2;
  } else {
    STRING_ASSERT((0x80 & c) != 0x80);
    rune = (Rune) c;

    *data = *data + 1;
    *data_len = *data_len - 1;
  }

  return rune;
}

STRING_DEF void rune_encode(Rune rune, u8 buf[4], u64 *buf_len) {

  *buf_len = 0;
  
  if(rune <= 128) {
    buf[(*buf_len)++] = (u8) rune;
  } else if(rune <= 2048) {
    // **** ****  **** ****  **** *123  4567 89AB
    //                     |
    //                     v
    // ***1 2345 **67 89AB
    buf[(*buf_len)++] = 0xc0 | ((rune >> 6) & 0x1f);
    buf[(*buf_len)++] = 0x80 | (rune & 0x3f);
  } else if(rune <= 65536) {
    // **** ****  **** ****  1234 5678  9ABC DEFG
    //                     |
    //                     v
    // **** 1234  **56 789A  **BC DEFG
    buf[(*buf_len)++] = 0xe0 | ((rune >> 12) & 0x0f);
    buf[(*buf_len)++] = 0x80 | ((rune >> 6) & 0x3f);
    buf[(*buf_len)++] = 0x80 | (rune & 0x3f);
  } else {
    // **** ****  ***1 2345  6789 ABCD  EFGH IJKL
    //                     |
    //                     v
    // **** *123  **45 6789  **AB CDEF  **GH IJKL
    buf[(*buf_len)++] = 0xf0 | ((rune >> 18) & 0x07);
    buf[(*buf_len)++] = 0x80 | ((rune >> 12) & 0x3f);
    buf[(*buf_len)++] = 0x80 | ((rune >> 6) & 0x3f);
    buf[(*buf_len)++] = 0x80 | (rune & 0x3f);
  }
}

STRING_DEF void rune_escape(Rune rune, u8 buf[6], u64 *buf_len) {
  *buf_len = 0;

  if(rune <= 128) {
    u8 c = (u8) rune;
    
    switch(c) {
    case '\"': {
      buf[(*buf_len)++] = '\\';
      buf[(*buf_len)++] = '\"';
    } break;
    case '\\': {
      buf[(*buf_len)++] = '\\';
      buf[(*buf_len)++] = '\\';	
    } break;
    case '/': {
      buf[(*buf_len)++] = '\\';
      buf[(*buf_len)++] = '/';      
    } break;
    case '\b': {
      buf[(*buf_len)++] = '\\';
      buf[(*buf_len)++] = 'f';
    } break;
    case '\f': {
      buf[(*buf_len)++] = '\\';
      buf[(*buf_len)++] = 'f';      
    } break;
    case '\n': {
      buf[(*buf_len)++] = '\\';
      buf[(*buf_len)++] = 'n';      
    } break;
    case '\r': {
      buf[(*buf_len)++] = '\\';
      buf[(*buf_len)++] = 'r';
    } break;
    case '\t': {
      buf[(*buf_len)++] = '\\';
      buf[(*buf_len)++] = 't';
    } break;
    default: {
      buf[(*buf_len)++] = (u8) rune;
    } break;
    } 
        
  } else {
    buf[0] = '\\';
    buf[1] = 'u';

    u64 pos = 5;

    while(rune > 0) {
      u8 c = (u8) (rune % 16);
      if(c < 10) {
	c += '0';
      } else {
	c += 'W';
      }
      buf[pos--] = c;
      rune /= 16;
    }

    while(pos >= 2) buf[pos--] = '0';
    *buf_len = 6;
  }
  
}


STRING_DEF Rune rune_unescape(u8 **data, u64 *data_len) {
  u8 c = (*data)[0];

  Rune rune = 0;
  if(c != '\\') {  // unescaped u8 'a'
    rune = (Rune) c;
    *data = *data + 1;
    *data_len = *data_len - 1;
  } else {
    c = (*data)[1];

    if(c != 'u') { // escaped u8 '\n'

      switch(c) {
      case '\"': rune = (Rune) '\"'; break;
      case '\\': rune = (Rune) '\\'; break;
      case '/': rune = (Rune) '/'; break;
      case 'b': rune = (Rune) '\b'; break;
      case 'f': rune = (Rune) '\f'; break;
      case 'n': rune = (Rune) '\n'; break;
      case 'r': rune = (Rune) '\r'; break;
      case 't': rune = (Rune) '\t'; break;
      }

      *data = *data + 2;
      *data_len = *data_len - 2;
    } else { // unicode u8acter '\u00d6'
      
      Rune n;
      for(u8 i=0;i<4;i++) {
    
	c = (*data)[2 + i];
	if('0' <= c && c <= '9') {
	  n = c - '0';
	} else if('a' <= c && c <= 'f') {
	  n = c - 'W';
	} else if('A' <= c && c <= 'F') {
	  n = c - '7';
	} else {
	  break;
	}
	rune *= 16;
	rune += n;
      }

      *data = *data + 6;
      *data_len = *data_len - 6;
    }
     
  }

  return rune;
}


#endif // STRING_IMPLEMENTATION

#undef u8
#undef u32
#undef s32
#undef s64
#undef u64

#endif //  _STRING_H
