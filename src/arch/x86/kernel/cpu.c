#include <oak/cpu.h>
#include <oak/debug/kassert.h>

/**
 *  @brief  清除 IF 位并返回清除前的值
 *  @return  清除前 IF 的值
 */
bool cpu_diable_intr() {
    asm volatile("pushfl\n"
                 "cli\n"
                 "popl %eax\n"
                 "shrl $9, %eax\n"
                 "andl $1, %eax\n");
}

/**
 *  @brief  获取 IF 位的值
 *  @return  IF 的值
 */
bool cpu_get_intr_state() {
    asm volatile("pushfl\n"
                 "popl %eax\n"
                 "shrl $9, %eax\n"
                 "andl $1, %eax\n");
}

/**
 *  @brief  设置或清除 IF 位
 *  @param  bool
 */
void cpu_set_intr_state(bool state) {
    if (state) {
        asm volatile("sti\n");
    } else {
        asm volatile("cli\n");
    }
}

/*  @breif  刷新页表缓存
 *  @param  vaddr  页表虚拟地址
 */
void flush_tlb(u32 vaddr) {
    asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

/**
 *  @brief  写 cr3 寄存器
 *  @param  page_dir_addr  页目录地址
 */
void cpu_set_cr3(u32 page_dir_addr) {
    kassert((page_dir_addr & 0xfff) == 0);
    asm volatile("movl %%eax, %%cr3\n" ::"a"(page_dir_addr));
}
