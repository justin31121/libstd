
#define LIBSTD_IMPLEMENTATION
#define DECODER_DISABLE
#include "../libstd.h"

int main() {

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
