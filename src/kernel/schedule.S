.code32

.section .text

.globl task_switch

task_switch:
    push %ebp
    movl %esp, %ebp

    # System V ABI
    push %ebx
    push %esi
    push %edi

    # 获取当前页地址
    movl %esp, %eax
    andl $0xfffff000, %eax

    # 保存当前任务栈顶地址到当前任务的 task_t.stack_addr
    movl %esp, (%eax)

    # 加载参数，即要切换到的任务的 task_t 指针
    movl 8(%ebp), %eax
    # 加载 task_t.stack_addr 到 esp
    movl (%eax), %esp

    # 出栈 task_frame_t
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp

    # 此时位于栈顶的是任务的函数地址
    ret
