#include <oak/assert.h>
#include <oak/list.h>
#include <oak/types.h>

void list_init(list_t *list) {
    list->head.prev = NULL;
    list->head.next = &list->tail;

    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

void list_insert_before(list_node_t *anchor, list_node_t *node) {
    node->prev = anchor->prev;
    node->next = anchor;

    anchor->prev->next = node;
    anchor->prev = node;
}

void list_insert_after(list_node_t *anchor, list_node_t *node) {
    node->prev = anchor;
    node->next = anchor->next;

    anchor->next->prev = node;
    anchor->next = node;
}

void list_push(list_t *list, list_node_t *node) {
    assert(!list_search(list, node));
    list_insert_after(&list->head, node);
}

list_node_t *list_pop(list_t *list) {
    assert(!list_empty(list));
    list_node_t *node = list->head.next;
    list_remove(node);

    return node;
}

void list_pushback(list_t *list, list_node_t *node) {
    assert(!list_search(list, node));
    list_insert_before(&list->tail, node);
}

list_node_t *list_popback(list_t *list) {
    assert(!list_empty(list));
    list_node_t *node = list->tail.prev;
    list_remove(node);

    return node;
}

bool list_search(list_t *list, list_node_t *node) {
    list_node_t *next = list->head.next;
    while (next != &list->tail) {
        if (next == node) {
            return true;
        }
        next = next->next;
    }
    return false;
}

void list_remove(list_node_t *node) {
    assert(node->prev != NULL);
    assert(node->next != NULL);

    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->next = NULL;
    node->prev = NULL;
}

bool list_empty(list_t *list) { return (list->head.next == &list->tail); }

u32 list_size(list_t *list) {
    list_node_t *next = list->head.next;
    u32 size = 0;

    while (next != &list->tail) {
        size++;
        next = next->next;
    }
    return size;
}

#include <oak/debug.h>
#include <oak/memory.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

void list_test() {
    u32 count = 3;
    list_t holder;
    list_t *list = &holder;
    list_init(list);
    list_node_t *node;

    while (count--) {
        node = (list_node_t *)alloc_kpage(1);
        list_push(list, node);
    }

    while (!list_empty(list)) {
        node = list_pop(list);
        free_kpage((u32)node, 1);
    }

    count = 3;
    while (count--) {
        node = (list_node_t *)alloc_kpage(1);
        list_pushback(list, node);
    }

    LOGK("list size %d\n", list_size(list));

    while (!list_empty(list)) {
        node = list_popback(list);
        free_kpage((u32)node, 1);
    }

    node = (list_node_t *)alloc_kpage(1);
    list_pushback(list, node);

    LOGK("search node 0x%p --> %d\n", node, list_search(list, node));
    LOGK("search node 0x%p --> %d\n", 0, list_search(list, 0));

    list_remove(node);
    free_kpage((u32)node, 1);
}
