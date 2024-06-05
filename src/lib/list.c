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

void list_insert_sort(list_t *list, list_node_t *node, int offset) {
    list_node_t *anchor = &list->tail;
    int key = element_node_key(node, offset);
    for (list_node_t *ptr = list->head.next; ptr != &list->tail;
         ptr = ptr->next) {
        int compare = element_node_key(ptr, offset);
        if (compare > key) {
            anchor = ptr;
            break;
        }
    }

    assert(node->next == NULL);
    assert(node->prev == NULL);

    list_insert_before(anchor, node);
}
