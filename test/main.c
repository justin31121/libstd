
#define LIBSTD_IMPLEMENTATION
#include "../libstd.h"

int main() {

  printf("Exists: %s!\n",
	 io_exists("../libstd.h", NULL)
	 ? "true" : "false");

  return 0;
}
