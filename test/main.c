
#define LIBSTD_IMPLEMENTATION
#define HTTP_OPEN_SSL
#include "../libstd.h"

int main() {

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

  Window window;
  if(!window_init(&window, 400, 400, "yoyo", 0)) {
    panicf("window_init");
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

    window_swap_buffers(&window);
  }

  window_free(&window);
  
  return 0;
}
