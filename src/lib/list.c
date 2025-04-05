#include "oak/debug/kassert.h"
#include <oak/list.h>

/**
 *  @brief  初始化链表
 *  @param  list  链表结构体
 */
void list_init(list_t *list) {
    list->head.prev = NULL;
    list->head.next = &list->tail;

    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

/**
 *  @brief  在指定结点前插入新结点
 *  @param  anchor  指定结点
 *  @param  node  要插入的结点
 */
void list_insert_before(list_node_t *anchor, list_node_t *node) {
    if (anchor && node) {
        node->prev = anchor->prev;
        node->next = anchor;

        anchor->prev->next = node;
        anchor->prev = node;
    }
}

/**
 *  @brief  在指定结点后插入新结点
 *  @param  anchor  指定结点
 *  @param  node  要插入的结点
 */
void list_insert_after(list_node_t *anchor, list_node_t *node) {
    if (anchor && node) {
        node->prev = anchor;
        node->next = anchor->next;

        anchor->next->prev = node;
        anchor->next = node;
    }
}

/**
 *  @brief  在链表头部插入结点
 *  @param  list  链表
 *  @param  node  要插入的结点
 */
void list_push(list_t *list, list_node_t *node) {
    if (!list_is_node_exist(list, node)) {
        list_insert_after(&list->head, node);
    }
}

/**
 *  @brief  删除并返回链表最后一个结点
 *  @param  list  链表
 */
list_node_t *list_pop(list_t *list) {
    if (!list_is_empty(list)) {
        list_node_t *node = list->head.next;
        list_remove(node);
        return node;
    }
    return NULL;
}

/**
 *  @brief  在链表尾部插入结点
 *  @param  list  链表
 *  @param  node  要插入的结点
 */
void list_pushback(list_t *list, list_node_t *node) {
    if (!list_is_node_exist(list, node)) {
        list_insert_before(&list->tail, node);
    }
}

/**
 *  @brief  删除并返回链表最后一个结点
 *  @param  list  链表
 */
list_node_t *list_popback(list_t *list) {
    if (!list_is_empty(list)) {
        list_node_t *node = list->tail.prev;
        list_remove(node);
        return node;
    }
    return NULL;
}

/**
 *  @brief  删除链表中的指定结点
 *  @param  node  要删除的结点
 */
void list_remove(list_node_t *node) {
    if (node && node->next && node->prev) {
        node->prev->next = node->next;
        node->next->prev = node->prev;

        node->prev = NULL;
        node->next = NULL;
    }
}

/**
 *  @brief  检查指定结点是否存在
 *  @param  list  链表
 *  @param  node  要检查的结点
 *  @return  存在则返回 1，不存在则返回 0
 */
bool list_is_node_exist(list_t *list, list_node_t *node) {
    list_node_t *next = list->head.next;
    while (next != &list->tail) {
        if (next == node) {
            return true;
        }
        next = next->next;
    }
    return false;
}

/**
 *  @brief  检查链表是否为空
 *  @param  list  链表
 *  @return  为空则返回 1，不为空则返回 0
 */
bool list_is_empty(list_t *list) { return (list->head.next == &list->tail); }

/**
 *  @brief  获取链表结点数（不含头尾结点）
 *  @param  list  链表
 *  @return  链表结点数
 */
u32 list_size(list_t *list) {
    list_node_t *node = list->head.next;
    u32 size = 0;

    while (node != &list->tail) {
        size++;
        node = node->next;
    }

    return size;
}

void list_insert_sort(list_t *list, list_node_t *node, int offset) {}

void list_test() {
    list_t list;
    list_init(&list);

    list_node_t node1;
    list_node_t node2;
    list_push(&list, &node1);
    list_pushback(&list, &node2);

    kassert(list_size(&list) == 2);

    kassert(list.head.next == &node1);
    kassert(list.tail.prev == &node2);

    list_node_t *node_ptr = list_popback(&list);
    kassert(node_ptr == &node2);

    kassert(list_size(&list) == 1);

    node_ptr = list_pop(&list);
    kassert(node_ptr == &node1);
}
