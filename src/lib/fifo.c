#include <oak/debug/kassert.h>
#include <oak/fifo.h>

/**
 *  @brief  获取指定位置的下一个位置的索引
 *  @param  fifo  循环队列
 *  @param  pos  指定位置的索引
 *  @return  索引
 */
static __inline u32 fifo_next(fifo_t *fifo, u32 pos) {
    return (pos + 1) % fifo->capacity;
}

/**
 *  @brief  初始化循环队列
 *  @param  fifo  循环队列
 *  @param  buf  队列缓冲区
 *  @param  capacity  队列长度
 */
void fifo_init(fifo_t *fifo, char *buf, u32 capacity) {
    fifo->buf = buf;
    fifo->capacity = capacity;
    fifo->size = 0;
    fifo->head = 0;
    fifo->tail = 0;
}

/**
 *  @brief  检查队列是否已满
 *  @param  fifo  循环队列
 *  @return  已满则返回 1，未满则返回 0
 */
bool fifo_is_full(fifo_t *fifo) { return fifo->size == fifo->capacity; }

/**
 *  @brief  检查队列是否为空
 *  @param  fifo  循环队列
 *  @return  为空则返回 1，不为空则返回 0
 */
bool fifo_is_empty(fifo_t *fifo) { return fifo->size == 0; }

/**
 *  @brief  获取队列长度
 *  @param  fifo  循环队列
 *  @return  队列长度
 */
u32 fifo_size(fifo_t *fifo) { return fifo->size; }

/**
 *  @brief  在队尾入队一个元素
 *  @param  fifo  循环队列
 *  @param  byte  要入队的元素
 */
void fifo_push(fifo_t *fifo, char byte) {
    // 若队列已满则丢弃队首元素
    while (fifo_is_full(fifo)) {
        fifo_pop(fifo);
    }

    if (fifo->size == 0) {
        fifo->buf[fifo->tail] = byte;
    } else {
        fifo->tail = fifo_next(fifo, fifo->tail);
        fifo->buf[fifo->tail] = byte;
    }

    fifo->size++;
}

/**
 *  @brief  出队队首元素
 *  @param  fifo  循环队列
 *  @return  队首元素
 */
char fifo_pop(fifo_t *fifo) {
    kassert(!fifo_is_empty(fifo));
    char byte = fifo->buf[fifo->head];
    fifo->head = fifo_next(fifo, fifo->head);
    fifo->size--;
    return byte;
}

void fifo_test() {
    const u32 LENGTH = 8;
    char buf[LENGTH];
    fifo_t fifo;
    fifo_init(&fifo, buf, LENGTH);
    kassert(fifo_size(&fifo) == 0);

    for (size_t i = 0; i < LENGTH + 1; i++) {
        fifo_push(&fifo, i);
    }

    kassert(fifo_size(&fifo) == 8);

    for (size_t i = 1; i < LENGTH; i++) {
        kassert(fifo_pop(&fifo) == i);
    }

    kassert(fifo_size(&fifo) == 1);
}
