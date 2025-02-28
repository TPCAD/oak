#include <oak/io.h>

u8 inb(u16 port) {
  u8 ret;
  asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

u16 inw(u16 port) {
  u16 ret;
  asm volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

void outb(u16 port, u8 value) {
  asm volatile("outb %b0, %w1" ::"a"(value), "Nd"(port));
}

void outw(u16 port, u16 value) {
  asm volatile("outw %w0, %w1" ::"a"(value), "Nd"(port));
}
