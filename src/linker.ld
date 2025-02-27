ENTRY(_start)
SECTIONS {
    . = 0x10000;
    .text : {
        *(.multiboot2)
        FILL(0x00);
        . = 0x40;
        *(.text)
}
}
