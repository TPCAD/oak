ENTRY(_start)
PHDRS {
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
  bss PT_LOAD FLAGS(6);
}
/* MEMORY { */
/*     kernel (rx) : org = 0x10000, l = 30M */
/* } */
SECTIONS {
    . = 0x10000;
    .text : {
        *(.multiboot2)
        FILL(0x00);
        . = 0x40;
        *(.text)
    } :text
}
