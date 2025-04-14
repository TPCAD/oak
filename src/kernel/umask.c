#include <oak/task.h>
#include <oak/types.h>

/**
 *  @brief  设置进程用户权限
 *  @param  mask  掩码
 *  @return  旧的用户权限
 */
mode_t syscall_umask(mode_t mask) {
    task_t *curr_task = task_current_running();
    mode_t old = curr_task->umask;
    curr_task->umask = mask & 0777;
    return old;
}
