#include <oak/assert.h>
#include <oak/interrupt.h>
#include <oak/list.h>
#include <oak/mutex.h>
#include <oak/oak.h>
#include <oak/task.h>
#include <oak/types.h>

void mutex_init(mutex_t *mutex) {
    mutex->value = false; // nobody held when init
    list_init(&mutex->waiters);
}

void mutex_lock(mutex_t *mutex) {
    bool intr = interrupt_diable();

    task_t *current = running_task();
    while (mutex->value == true) {
        task_block(current, &mutex->waiters, TASK_BLOCKED);
    }

    // nobody held
    assert(mutex->value == false);

    // held it
    mutex->value++;
    assert(mutex->value == true);

    set_interrupt_state(intr);
}

void mutex_unlock(mutex_t *mutex) {

    bool intr = interrupt_diable();

    // already held
    assert(mutex->value == true);

    // release
    mutex->value--;
    assert(mutex->value == false);

    if (!list_empty(&mutex->waiters)) {
        task_t *task = element_entry(task_t, node, mutex->waiters.tail.prev);
        assert(task->magic == OAK_MAGIC);
        task_unblock(task);

        task_yield();
    }

    set_interrupt_state(intr);
}

void lock_init(lock_t *lock) {
    lock->holder = NULL;
    lock->repeat = 0;
    mutex_init(&lock->mutex);
}

void lock_acquire(lock_t *lock) {
    task_t *current = running_task();

    if (lock->holder != current) {
        mutex_lock(&lock->mutex);
        lock->holder = current;
        assert(lock->repeat == 0);
        lock->repeat = 1;
    } else {
        lock->repeat++;
    }
}

void lock_release(lock_t *lock) {
    task_t *current = running_task();
    assert(lock->holder == current);

    if (lock->repeat > 1) {
        lock->repeat--;
        return;
    }

    assert(lock->repeat == 1);

    lock->holder = NULL;
    lock->repeat = 0;
    mutex_unlock(&lock->mutex);
}
