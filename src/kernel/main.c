#include <oak/io.h>
char msg[] = "Hello Oak!";
char buf[1024];

void kernel_init() {
  char *video = (char *)0xb8000;
  for (int i = 0; i < sizeof(msg); i++) {
    video[i * 2] = msg[i];
  }
  outb(0x3d4, 0xe);
  u16 pos = inb(0x3d5) << 8;
}
