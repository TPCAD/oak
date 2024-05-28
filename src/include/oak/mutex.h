#ifndef OAK_MUTEX_H
#define OAK_MUTEX_H

#include <oak/list.h>
#include <oak/types.h>

typedef struct mutex_t {
    bool value;
    list_t waiters;
} mutex_t;

typedef struct lock_t {
    struct task_t *holder;
    mutex_t mutex;
    u32 repeat;
} lock_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

void lock_init(lock_t *lock);
void lock_acquire(lock_t *lock);
void lock_release(lock_t *lock);
#endif // !OAK_MUTEX_H
