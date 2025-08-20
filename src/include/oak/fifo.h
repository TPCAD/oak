#ifndef OAK_FIFO_H
#define OAK_FIFO_H

#include <oak/types.h>

// 字符队列
typedef struct fifo_t {
    char *buf;
    u32 capacity; // 队列最大长度
    u32 size;     // 队列实际长度
    u32 head;     // 队首索引
    u32 tail;     // 队尾索引
} fifo_t;

void fifo_init(fifo_t *fifo, char *buf, u32 length);
bool fifo_is_full(fifo_t *fifo);
bool fifo_is_empty(fifo_t *fifo);
u32 fifo_size(fifo_t *fifo);

char fifo_pop(fifo_t *fifo);
void fifo_push(fifo_t *fifo, char byte);

#endif // !OAK_FIFO_H
