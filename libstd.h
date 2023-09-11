#ifndef LIBSTD_H
#define LIBSTD_H

#ifdef LIBSTD_IMPLEMENTATION
#  define IO_IMPLEMENTATION
#  define TYPES_IMPLEMENTATION
#endif // LIBSTD_IMPLEMENTATION

#ifdef IO_DISABLE
#  define IO_H
#endif // IO_DISABLE

#ifdef TYPES_DISABLE
#  define TYPES_H
#endif // TYPES_DISABLE

#ifdef LIBSTD_DEF
#  define IO_DEF LIBSTD_DEF
#  define TYPES_DEF LIBSTD_DEF
#endif // LIBSTD_DEF

#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
#  include <windows.h>
#endif //_WIN32

#ifndef IO_DEF
#  define IO_DEF
#endif //IO_DEF

#ifndef IO_LOG
#  ifndef IO_QUIET
#    define IO_LOG(fmt, ...) fprintf(stderr, "IO_LOG: "fmt"\n", __VA_ARGS__)
#  else 
#    define IO_LOG(...)
#  endif // IO_VERBOSE
#endif //IO_LOG

////////////////////////////////////////////////////////////////////////////////////////

// Io_Util

IO_DEF bool io_slurp_file(const char *filepath, unsigned char **data, size_t *data_size);
IO_DEF bool io_exists(const char *file_path, bool *is_file);

////////////////////////////////////////////////////////////////////////////////////////

// Io_Dir

typedef struct{
#ifdef _WIN32
    WIN32_FIND_DATAW file_data;
    HANDLE handle;
    bool stop;
#else
    struct dirent *ent;
    DIR *handle;
#endif //_WIN32

    const char *name;
}Io_Dir;

typedef struct{
#ifdef _WIN32
  char abs_name[MAX_PATH];
#else
  char abs_name[PATH_MAX];
#endif //_WIN32

  char *name;
  bool is_dir;  
}Io_Dir_Entry;

IO_DEF bool io_dir_open(Io_Dir *dir, const char *dir_path);
IO_DEF bool io_dir_next(Io_Dir *dir, Io_Dir_Entry *entry);
IO_DEF void io_dir_close(Io_Dir *dir);

////////////////////////////////////////////////////////////////////////////////////////

// Io_File

typedef enum{
  IO_MODE_READ = 0,
  IO_MODE_WRITE,
  COUNT_IO_MODE,
}Io_Mode;

#ifdef _WIN32
typedef struct{ HANDLE handle; DWORD size; DWORD pos; }Io_File;
#else
typedef struct{ FILE *f; }Io_File;
#endif //_WIN32

IO_DEF bool io_file_size(Io_File *f, size_t *size);

IO_DEF bool io_file_open(Io_File *f, const char *filepath, Io_Mode mode);
IO_DEF int io_file_seek(Io_File *f, long int offset, int whence);
IO_DEF long int io_file_tell(Io_File *f);
IO_DEF size_t io_file_read(Io_File *f, void *ptr, size_t size, size_t count);
IO_DEF size_t io_file_write(Io_File *f, void *ptr, size_t size, size_t nmemb);
IO_DEF void io_file_close(Io_File *f);

////////////////////////////////////////////////////////////////////////////////////////

IO_DEF int io_last_error();
IO_DEF const char *io_last_error_cstr();

#ifdef IO_IMPLEMENTATION

#define io_file_write_cstr(f, cstr) io_file_write((f), (cstr), 1, (strlen(cstr)))

IO_DEF bool io_slurp_file(const char *filepath, unsigned char **data, size_t *data_size) {
  Io_File f;
  if(!io_file_open(&f, filepath, IO_MODE_READ)) {
    IO_LOG("Failed to open '%s': (%d) %s",
	   filepath, io_last_error(), io_last_error_cstr());
    return false;
  }

  if(!io_file_size(&f, data_size)) {
    io_file_close(&f);
    return false;
  }

  unsigned char *result = malloc(*data_size);
  if(!result) {
    io_file_close(&f);
    IO_LOG("Failed to allocate enough memory. Tried to allocate: %zu bytes", *data_size);
    return false;
  }

  if(*data_size != io_file_read(&f, result, 1, *data_size)) {
    io_file_close(&f);
    IO_LOG("Failed to read: '%s': (%d) %s",
	   filepath, io_last_error(), io_last_error_cstr());
    return false;
  }

  *data = result;

  return true;
}

IO_DEF bool io_exists(const char *file_path, bool *is_file) {
  DWORD attribs = GetFileAttributes(file_path);
  if(is_file) *is_file = !(attribs & FILE_ATTRIBUTE_DIRECTORY);
  return attribs != INVALID_FILE_ATTRIBUTES;
}

////////////////////////////////////////////////////////////////////////////////////////

IO_DEF bool io_dir_open(Io_Dir *dir, const char *dir_path) {
#ifdef _WIN32
  int num_wchars = MultiByteToWideChar(CP_UTF8, 0, dir_path, -1, NULL, 0); 
  wchar_t *my_wstring = (wchar_t *)malloc((num_wchars+1) * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, dir_path, -1, my_wstring, num_wchars);
  my_wstring[num_wchars-1] = '*';
  my_wstring[num_wchars] = 0;

  // Use my_wstring as a const wchar_t *
  dir->handle = FindFirstFileExW(my_wstring, FindExInfoStandard, &dir->file_data, FindExSearchNameMatch, NULL, 0);
  if(dir->handle == INVALID_HANDLE_VALUE) {
    free(my_wstring);
    return false;
  }

  bool is_dir = (dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
  if(!is_dir) {
    free(my_wstring);
    return false;
  }

  dir->name = dir_path;
  dir->stop = false;

  free(my_wstring);
  return true;
#else
  return false;
#endif //_WIN32
}

IO_DEF bool io_dir_next(Io_Dir *dir, Io_Dir_Entry *entry) {
#ifdef _WIN32
  if(dir->stop) {
    return false;
  }

  entry->is_dir = (dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;

  size_t len = strlen(dir->name);
  memcpy(entry->abs_name, dir->name, len);
  int len2 = WideCharToMultiByte(CP_ACP, 0, dir->file_data.cFileName, -1, NULL, 0, NULL, NULL);
  WideCharToMultiByte(CP_ACP, 0, dir->file_data.cFileName, -1, entry->abs_name + len, len2, NULL, NULL);

  //WHAT IS THIS
  if(entry->is_dir) {
    entry->abs_name[len + len2-1] = '/';
    entry->abs_name[len + len2] = 0;       
  } else {
    entry->abs_name[len + len2-1] = 0;
  }

  entry->name = (char *) &entry->abs_name[len];

  if(FindNextFileW(dir->handle, &dir->file_data) == 0) {
    dir->stop = true;
  }

  return true;
#else
  return false;
#endif //_WIN32
}

IO_DEF void io_dir_close(Io_Dir *dir) {
#ifdef _WIN32
  FindClose(dir->handle);
#else
  
#endif //_WIN32
}

////////////////////////////////////////////////////////////////////////////////////////

IO_DEF bool io_file_size(Io_File *f, size_t *size) {
#ifdef _WIN32
  *size = f->size;
  return true;
#else
  return false;
#endif //_WIN32
}

IO_DEF bool io_file_open(Io_File *f, const char *filepath, Io_Mode mode) {
#ifdef _WIN32
  if(mode < 0 || COUNT_IO_MODE <= mode)
    return false;

  f->handle = INVALID_HANDLE_VALUE;

  if(mode == IO_MODE_READ) {
    f->handle = CreateFile(filepath, GENERIC_READ,
			   FILE_SHARE_READ,
			   NULL,
			   OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL,
			   NULL);
    if(f->handle == INVALID_HANDLE_VALUE)
      goto error;

    f->size = GetFileSize(f->handle, NULL);
    if(f->size == INVALID_FILE_SIZE)
      goto error;

    f->pos = 0;
  } else {
    f->handle = CreateFile(filepath,
			   GENERIC_WRITE, 0, NULL,
			   CREATE_ALWAYS,
			   FILE_ATTRIBUTE_NORMAL,
			   NULL);
    if(f->handle == INVALID_HANDLE_VALUE)
      goto error;
    
    f->pos = 0;
    f->size = INVALID_FILE_SIZE;
  }
  
  return true;
 error:

  if(f->handle != INVALID_HANDLE_VALUE)
    CloseHandle(f->handle);
  
  return false;
#else
  return false;
#endif //_WIN32  
}

IO_DEF int io_file_seek(Io_File *f, long int offset, int whence) {
#ifdef _WIN32
  DWORD moveMethod;

  switch (whence) {
  case SEEK_SET: {
    moveMethod = FILE_BEGIN;    
  } break;
  case SEEK_CUR: {
    moveMethod = FILE_CURRENT;    
  } break;
  case SEEK_END: {
    moveMethod = FILE_END;    
  } break;
  default: {
    return -1;  // Invalid whence
  } break;
  }

  f->pos = SetFilePointer(f->handle, offset, NULL, moveMethod);
  if(f->pos == INVALID_SET_FILE_POINTER)
    return -1;

  return 0;
#else
  return -1;
#endif //_WIN32
}

IO_DEF long int io_file_tell(Io_File *f) {
#ifdef _WIN32
  return (long int) f->pos;
#else
  return -1;
#endif //_WIN32
}

IO_DEF void io_file_close(Io_File *f) {
#ifdef _WIN32
  CloseHandle(f->handle);
#else
#endif //_WIN32
}


IO_DEF size_t io_file_read(Io_File *f, void *ptr, size_t size, size_t count) {
#ifdef _WIN32
  DWORD bytes_read;
  DWORD bytes_to_read = (DWORD) (size * count);

  if(!ReadFile(f->handle, ptr, bytes_to_read, &bytes_read, NULL))
    return 0;
  f->pos += bytes_read;

  return (size_t) (bytes_read / size);
#else
  return 0;
#endif //_WIN32
}

IO_DEF size_t io_file_write(Io_File *f, void *ptr, size_t size, size_t nmemb) {
#ifdef _WIN32
  DWORD bytes_written;
  DWORD bytes_to_write = (DWORD) (size * nmemb);
  
  if(!WriteFile(f->handle, ptr, bytes_to_write, &bytes_written, NULL))
    return 0;

  return (size_t) (bytes_written / size);
#else
  return 0;
#endif //_WIN32
}

////////////////////////////////////////////////////////////////////////////////////////

IO_DEF int io_last_error() {
#ifdef _WIN32
  return GetLastError();
#else
  return 0;
#endif //_WIN32
}

IO_DEF const char *io_last_error_cstr() {
#ifdef _WIN32
  DWORD error = GetLastError();
  static char buffer[1024];

  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
		 NULL,
		 error,
		 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		 (LPSTR) &buffer,
		 sizeof(buffer),
		 NULL);
  
  return buffer;
#else
  return NULL;
#endif //_WIN32
}

#endif //IO_IMPLEMENTATION

#endif //IO_H

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

#endif // LIBSTD_H
