#ifndef OAK_LIST_H
#define OAK_LIST_H

#include "oak/types.h"

#define element_offset(type, member) (u32)(&((type *)0)->member)
#define element_entry(type, member, ptr)                                       \
    (type *)((u32)ptr - element_offset(type, member))
#define element_node_offset(type, node, key)                                   \
    ((int)(&((type *)0)->key) - (int)(&((type *)0)->node))
#define element_node_key(node, offset) *(int *)((int)node + offset)
// list node
typedef struct list_node_t {
    struct list_node_t *prev;
    struct list_node_t *next;
} list_node_t;

// list
typedef struct list_t {
    list_node_t head;
    list_node_t tail;
} list_t;

// init list
void list_init(list_t *list);

// insert before anchor
void list_insert_before(list_node_t *anchor, list_node_t *node);

// insert after anchor
void list_insert_after(list_node_t *anchor, list_node_t *node);

// push (insert after head node)
void list_push(list_t *list, list_node_t *node);

// pop (remove the node after head)
list_node_t *list_pop(list_t *list);

// pushback (insert before tail)
void list_pushback(list_t *list, list_node_t *node);

// popback (remove the node before tail)
list_node_t *list_popback(list_t *list);

// search node in list
bool list_search(list_t *list, list_node_t *node);

// remove node in list
void list_remove(list_node_t *node);

// is list empty
bool list_empty(list_t *list);

// size of list
u32 list_size(list_t *list);

// insert sort
void list_insert_sort(list_t *list, list_node_t *node, int offset);
#endif // !OAK_LIST_H
