#ifndef SH_H
#define SH_H

#ifndef SH_DEF
#  define SH_DEF static inline
#endif // SH_DEF

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <windows.h>

#define sh_da_append_many(n, xs, xs_len) do{				\
    size_t new_cap = (n)->cap;						\
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

SH_DEF const char *sh_last_error_cstr();
void sh_keep_updated(int argc, char **argv);
SH_DEF const char *sh_next(int *argc, char ***argv);
SH_DEF bool sh_needs_update(const char *src, const char *dst);

typedef enum{
  SH_CMD,
  SH_INFO,
  SH_ERROR,
}Sh_Log_Type;

SH_DEF void sh_log(Sh_Log_Type type, const char *fmt, ...);

typedef SYSTEMTIME Sh_Time;

SH_DEF bool sh_time_get(const char *file_path, Sh_Time *time);
SH_DEF int sh_time_compare(const Sh_Time *a, const Sh_Time *b);

typedef struct{
  char *items;
  size_t len;
  size_t cap;
}Sh;

SH_DEF void sh_append_impl(Sh *sh, ...);
SH_DEF int sh_run(Sh *sh);

#define sh_append(sh, ...) sh_append_impl(sh, __VA_ARGS__, NULL)

#ifdef SH_IMPLEMENTATION

SH_DEF const char *sh_last_error_cstr() {

  DWORD error = GetLastError();
  static char buffer[1024];

  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
		 NULL,
		 error,
		 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		 (LPSTR) &buffer,
		 sizeof(buffer),
		 NULL);

  size_t buffer_len = strlen(buffer);
  if(buffer[buffer_len - 1] == '\n') {
    buffer[buffer_len - 1] = 0;    
  }
  
  return buffer;
}

#ifdef _MSC_VER
#  define __sh_keep_updated_compiler() sh_append(&sh, "cl", "/Fe:", dst, src)
#elif __GNUC__
#  define __sh_keep_updated_compiler() sh_append(&sh, "gcc", "-o", dst, src)
#else
#  define __sh_keep_updated_compiler() do{				\
    sh_log(SH_ERROR, "Unknown Compiler: implement '__sh_keep_updated_compiler()' in sh.h"); \
    return 1;								\
  }while(0)
#endif

#define sh_keep_updated(argc, argv) do{					\
									\
  char dst[MAX_PATH];							\
  if(strstr(argv[0], ".exe")) {						\
    assert((size_t) snprintf(dst, sizeof(dst), "%s", argv[0]) < sizeof(dst));	\
  } else {								\
    assert((size_t) snprintf(dst, sizeof(dst), "%s.exe", argv[0]) < sizeof(dst)); \
  }									\
									\
  char dst_old[MAX_PATH];						\
  assert((size_t) snprintf(dst_old, sizeof(dst_old), "%s.old", dst) < sizeof(dst_old)); \
									\
  const char *src = __FILE__;						\
									\
  Sh_Time src_time, dst_time;						\
  if(!sh_time_get(src, &src_time)) return 1;				\
  if(!sh_time_get(dst, &dst_time)) return 1;				\
  if(sh_time_compare(&src_time, &dst_time) > 0) {			\
									\
    if(!DeleteFile(dst_old) && GetLastError() != ERROR_FILE_NOT_FOUND) { \
      sh_log(SH_ERROR, "Can not delete '%s': %s",			\
	     dst_old, sh_last_error_cstr());				\
      return 1;								\
    }									\
  									\
    if(!MoveFile(dst, dst_old)) {					\
      sh_log(SH_ERROR, "Can not move file '%s' to '%s': %s",		\
	     dst, dst_old, sh_last_error_cstr());			\
      return 1;								\
    }									\
									\
    Sh sh = {0};							\
    __sh_keep_updated_compiler();					\
    int exit_code = sh_run(&sh);					\
    if(exit_code != 0) {						\
									\
      if(!MoveFile(dst_old, dst)) {					\
        sh_log(SH_ERROR, "Failed to move file: '%s' to '%s' : %s\n",	\
	       dst_old, dst, sh_last_error_cstr());			\
	return 1;							\
      }									\
									\
      return exit_code;							\
    }									\
									\
    sh.len = 0;								\
    sh_append(&sh, dst);						\
    for(int i=1;i<argc;i++) {						\
      sh_append(&sh, argv[i]);						\
    }									\
									\
    return sh_run(&sh);							\
  }									\
}while(0)

SH_DEF const char *sh_next(int *argc, char ***argv) {
  if((*argc) == 0) return NULL;

  const char *next = (*argv)[0];

  *argc = (*argc) + 1;
  *argv = (*argv) + 1;

  return next;
}

SH_DEF bool sh_needs_update(const char *src, const char *dst) {
  Sh_Time src_time, dst_time;
  if(!sh_time_get(src, &src_time)) return false;
  if(!sh_time_get(dst, &dst_time)) return true;
  return sh_time_compare(&src_time, &dst_time) > 0;
}

SH_DEF void sh_log(Sh_Log_Type type, const char *fmt, ...) {

  switch(type) {
  case SH_CMD:
    fprintf(stderr, "[CMD]: ");
    break;
  case SH_INFO:
    fprintf(stderr, "[INFO]: ");
    break;
  case SH_ERROR:
    fprintf(stderr, "[ERROR]: ");
    break;
  }
  
  va_list argv;
  va_start(argv, fmt);

  vfprintf(stderr, fmt, argv);
  fprintf(stderr, "\n");
  fflush(stderr);

  va_end(argv);
}

SH_DEF bool sh_time_get(const char *file_path, Sh_Time *time) {
  HANDLE handle = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ,
			     NULL, OPEN_EXISTING, 0, NULL);
  if(handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  FILETIME written;
  SYSTEMTIME system_time;
  bool result = GetFileTime(handle, NULL, NULL, &written);
  
  CloseHandle(handle);

  if(result) {

    FileTimeToSystemTime(&written, &system_time);
    SystemTimeToTzSpecificLocalTime(NULL, &system_time, time);

    return true;    
  } else {
    return false;
  }
}

SH_DEF int sh_time_compare(const Sh_Time *a, const Sh_Time *b) {
  if(a->wYear < b->wYear) return -1;
  if(a->wYear > b->wYear) return  1;
  if(a->wMonth < b->wMonth) return -1;
  if(a->wMonth > b->wMonth) return  1;
  if(a->wDay < b->wDay) return -1;
  if(a->wDay > b->wDay) return  1;
  if(a->wHour < b->wHour) return -1;
  if(a->wHour > b->wHour) return  1;
  if(a->wMinute < b->wMinute) return -1;
  if(a->wMinute > b->wMinute) return  1;
  if(a->wSecond < b->wSecond) return -1;
  if(a->wSecond > b->wSecond) return  1;
  if(a->wMilliseconds < b->wMilliseconds) return -1;
  if(a->wMilliseconds > b->wMilliseconds) return  1;
  return 0;
}

SH_DEF void sh_append_impl(Sh *sh, ...) {

  va_list argv;
  va_start(argv, sh);

  for(;;) {
    char *item = va_arg(argv, char *);
    if(!item) break;

    sh_da_append_many(sh, item, strlen(item));
    sh_da_append_many(sh, " ", 1);
  }

  va_end(argv);
}

SH_DEF int sh_run(Sh *sh) {

  if(sh->len == 0) {
    sh_log(SH_ERROR, "Cannot start empty process!");
    return -1;
  }

  sh_da_append_many(sh, "\0", 1);
  sh_log(SH_CMD, "%s", sh->items);
  
  STARTUPINFO si;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);

  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(pi));

  if(!CreateProcess(NULL, sh->items, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    sh_log(SH_ERROR, "Cannot start process '%s': %s",
	   sh->items, sh_last_error_cstr());
    return -1;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code;
  GetExitCodeProcess(pi.hProcess, &code);
  
  return (int) code;
}

#endif // SH_IMPLEMENTATION

#endif // SH_H
