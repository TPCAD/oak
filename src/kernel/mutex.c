#include "oak/debug/kassert.h"
#include "oak/list.h"
#include "oak/oak.h"
#include "oak/task.h"
#include <oak/cpu.h>
#include <oak/mutex.h>

/**
 *  @brief  初始化互斥量
 *  @param  mutex  互斥量
 */
void mutex_init(mutex_t *mutex) {
    mutex->value = false;
    list_init(&mutex->waiters);
}

/**
 *  @brief  尝试持有互斥量
 *  @param  mutex  互斥量
 *
 *  尝试失败会进入阻塞状态，直至互斥量被释放
 */
void mutex_lock(mutex_t *mutex) {
    bool intr = cpu_diable_intr();
    task_t *curr_task = task_current_running();

    // 尝试持有互斥量，若互斥量已被持有则进入阻塞状态
    while (mutex->value == true) {
        task_block(curr_task, &mutex->waiters, TASK_BLOCKED);
    }

    kassert(mutex->value == false);

    // 持有互斥量
    mutex->value++;
    kassert(mutex->value == true);

    // 恢复中断状态
    cpu_set_intr_state(intr);
}

/**
 *  @brief  释放互斥量
 *  @param  mutex  互斥量
 */
void mutex_unlock(mutex_t *mutex) {
    bool intr = cpu_diable_intr();

    kassert(mutex->value == true);

    mutex->value--;
    kassert(mutex->value == false);

    // 唤醒正在等待互斥量的任务
    if (!list_is_empty(&mutex->waiters)) {
        task_t *task = element_entry(task_t, node, mutex->waiters.tail.prev);
        kassert(task->magic == OAK_MAGIC);
        task_unblock(task);

        // 主动让出 CPU，防止其他任务饿死
        task_yield();
    }

    cpu_set_intr_state(intr);
}

/**
 *  @brief  初始化互斥锁
 *  @param  lock  互斥锁指针
 */
void lock_init(lock_t *lock) {
    lock->holder = NULL;
    lock->repeat = 0;
    mutex_init(&lock->mutex);
}

/**
 *  @brief  对互斥锁上锁
 *  @param  lock  互斥锁指针
 */
void lock_acquire(lock_t *lock) {
    task_t *curr_task = task_current_running();

    if (lock->holder != curr_task) {
        mutex_lock(&lock->mutex);
        lock->holder = curr_task;
        kassert(lock->repeat == 0);
        lock->repeat = 1;
    } else {
        lock->repeat++;
    }
}

/**
 *  @brief  释放互斥锁
 *  @param  lock  互斥锁指针
 */
void lock_release(lock_t *lock) {
    task_t *curr_task = task_current_running();
    kassert(lock->holder == curr_task);

    if (lock->repeat > 1) {
        lock->repeat--;
        return;
    }

    kassert(lock->repeat == 1);
    lock->holder = NULL;
    lock->repeat = 0;
    mutex_unlock(&lock->mutex);
}
