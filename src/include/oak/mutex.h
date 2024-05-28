#ifndef OAK_MUTEX_H
#define OAK_MUTEX_H

#include <oak/list.h>
#include <oak/types.h>

typedef struct mutex_t {
    bool value;
    list_t waiters;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif // !OAK_MUTEX_H
