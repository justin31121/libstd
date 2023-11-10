
#include <stdio.h>
#include <assert.h>

#include "src/types.h"

#define IO_IMPLEMENTATION
#include "src/io.h"

bool get_title_uppercase(char *name, char buffer[MAX_PATH]) {

  char *p = strrchr(name, '.');
  if(!p) {
    return false;
  }
  
  size_t name_len = (p - name);
  
  size_t i=0;
  for(;i<name_len;i++) {

    char c = name[i];
    if('a' <= c && c <= 'z') {
      c -= ' ';
    }
    buffer[i] = c;
    
  }
  buffer[i] = 0;
  
  return true;
}

typedef struct{
  char name[MAX_PATH];
  char path[MAX_PATH];
}Component;

#define COMPONENTS_CAP 48

Component components[COMPONENTS_CAP] = {0};
size_t components_count = 0;

int main() {

  //OPEN
  const char *output = "libstd.h";

  Io_File f;
  if(!io_file_open(&f, output, IO_MODE_WRITE)) {
    panicf("Can not open: '%s': (%d) %s\n",
	   output, io_last_error(), io_last_error_cstr());
  }

  char temp[8192];
  size_t temp_size = 0;
  char name[MAX_PATH];

  //FOR EVERY FILE IN ...
  const char *source_dir = "thirdparty/";
 
  Io_Dir dir;
  if(!io_dir_open(&dir, source_dir)) {
    panicf("Can not open: '%s': (%d) %s",
	   source_dir, io_last_error(), io_last_error_cstr());
  }

  Io_Dir_Entry entry;
  while(io_dir_next(&dir, &entry)) {
    if(entry.is_dir) continue;
    if(!get_title_uppercase(entry.name, name)) continue;

    assert(components_count < COMPONENTS_CAP);
    Component *c = &components[components_count++];

    memcpy(c->name, name, strlen(name) + 1);
    memcpy(c->path, entry.abs_name, strlen(entry.abs_name) + 1);
  }

  io_dir_close(&dir);

  source_dir = "src/";

  if(!io_dir_open(&dir, source_dir)) {
    panicf("Can not open: '%s': (%d) %s",
	   source_dir, io_last_error(), io_last_error_cstr());
  }

  while(io_dir_next(&dir, &entry)) {
    if(entry.is_dir) continue;
    if(!get_title_uppercase(entry.name, name)) continue;

    assert(components_count < COMPONENTS_CAP);
    Component *c = &components[components_count++];

    memcpy(c->name, name, strlen(name) + 1);
    memcpy(c->path, entry.abs_name, strlen(entry.abs_name) + 1);
  }

  io_dir_close(&dir);


  //PROCESS

  io_file_write_cstr(&f, "#ifndef LIBSTD_H\n");
  io_file_write_cstr(&f, "#define LIBSTD_H\n\n");

  // _ENABLE

  for(size_t i=0;i<components_count;i++) {
    Component *c = &components[i];

    temp_size = snprintf(temp, sizeof(temp), "// #define %s_ENABLE\n", c->name);
    assert(temp_size < sizeof(temp));

    io_file_write(&f, temp, temp_size, 1);
  }

  io_file_write_cstr(&f, "\n");

  //  _IMPLEMENTATION

  io_file_write_cstr(&f, "#ifdef LIBSTD_IMPLEMENTATION\n");

  for(size_t i=0;i<components_count;i++) {
    Component *c = &components[i];

    temp_size = snprintf(temp, sizeof(temp), "#  define %s_IMPLEMENTATION\n", c->name);
    assert(temp_size < sizeof(temp));

    io_file_write(&f, temp, temp_size, 1);
  }

  io_file_write_cstr(&f, "#endif // LIBSTD_IMPLEMENTATION\n\n");

  //  _DEF

  io_file_write_cstr(&f, "#ifdef LIBSTD_DEF\n");

  for(size_t i=0;i<components_count;i++) {
    Component *c = &components[i];

    temp_size = snprintf(temp, sizeof(temp), "#  define %s_DEF LIBSTD_DEF\n", c->name);
    assert(temp_size < sizeof(temp));

    io_file_write(&f, temp, temp_size, 1);
  }  
  
  io_file_write_cstr(&f, "#endif // LIBSTD_DEF\n\n");

  // *CONTENT*
  temp_size = 0;
  for(size_t i=0;i<components_count;i++) {
    Component *c = &components[i];

    unsigned char *data;
    size_t size;
    if(!io_slurp_file(c->path, &data, &size)) {
      return 1;
    }

    io_file_write_cstr(&f, "#ifdef ");
    io_file_write_cstr(&f, c->name);
    io_file_write_cstr(&f, "_ENABLE\n\n");
    
    //Fix \r\n's
    for(size_t j=0;j<size;j++) {

      if(temp_size == sizeof(temp))  {
	io_file_write(&f, temp, temp_size, 1);
	temp_size = 0;
      }
      
      if(data[j] == '\r') continue;
      temp[temp_size++] = data[j];     
    }

    if(temp_size > 0) {
      io_file_write(&f, temp, temp_size, 1);
      temp_size = 0;
    }

    io_file_write_cstr(&f, "#endif // ");
    io_file_write_cstr(&f, c->name);
    io_file_write_cstr(&f, "_DISABLE\n\n");
  }

  if(temp_size > 0) {
    io_file_write(&f, temp, temp_size, 1);
  }

  io_file_write_cstr(&f, "#endif // LIBSTD_H\n");
  
  io_file_close(&f);  
  
  return 0;
  
}
