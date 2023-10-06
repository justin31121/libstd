
#define LIBSTD_IMPLEMENTATION
#  define TYPES_ENABLE
#  define THREAD_ENABLE
#  define HTTP_ENABLE
#    define HTTP_OPEN_SSL
#  define STB_TRUETYPE_ENABLE
#  define WINDOW_ENABLE
#    define WINDOW_STB_TRUETYPE
#include "../libstd.h"

void *thread_func(void *arg) {
  (void) arg;
  
  Http http;
  if(!http_init("www.example.com", HTTPS_PORT, true, &http)) {
    panicf("http_init");
  }

  Http_Request request;
  if(!http_request_from(&http, "/", "GET", NULL, NULL, 0, &request)) {
    panicf("http_request_from");
  }  

  s8 *data;
  u64 data_len;
  while(http_next_body(&request, &data, &data_len)) {
    printf("%.*s", (int) data_len, data);
  }
  fflush(stdout);

  
  return NULL;
}

int main() {

  Thread id;
  thread_create(&id, thread_func, NULL);    
    
  Window window;
  if(!window_init(&window, 400, 400, "yoyo", 0)) {
    panicf("window_init");
  }

  if(!push_font("c:\\Windows\\Fonts\\times.ttf", 64)) {
    panicf("push_font");
  }
  
  Window_Event event;
  while(window.running) {
    while(window_peek(&window, &event)) {
      if(event.type == WINDOW_EVENT_KEYPRESS) {
	if(event.as.key == 'q') {
	  window.running = false;
	}
      }
    }

    Vec2f size;
    const char *cstr = "Hello, World!";
    measure_text(cstr, 1.f, &size);
    draw_text_colored(cstr, vec2f(window.width/2 - size.x/2, window.height/2 - size.y/2), 1.f, WHITE);

    window_swap_buffers(&window);
  }

  window_free(&window);
  
  return 0;
}
