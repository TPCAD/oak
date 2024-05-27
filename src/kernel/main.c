#include <oak/types.h>
extern void interrupt_init();
extern void clock_init();
extern void time_init();
extern void rtc_init();
extern void memory_map_init();
extern void mapping_init();
extern void hang();
extern void task_init();
extern void set_interrupt_state(bool state);
extern void syscall_init();

void kernel_init() {
    memory_map_init();
    mapping_init();

    interrupt_init();
    clock_init();

    // time_init();
    // rtc_init();
    task_init();
    syscall_init();
    set_interrupt_state(true);

    return;
}
