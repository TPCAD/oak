#ifndef OAK_BITMAP_H
#define OAK_BITMAP_H

#include "oak/types.h"

typedef struct bitmap_t {
    u8 *bits;   // buffer
    u32 length; // length
} bitmap_t;

// init bitmap
void bitmap_init(bitmap_t *map, u8 *bits, u32 length);

// make bitmap
void bitmap_make(bitmap_t *map, u8 *bits, u32 length);

// whether certain bit is set or not
bool bitmap_if_set(bitmap_t *map, u32 index);

// set certain bit
void bitmap_set(bitmap_t *map, u32 index, bool value);

// get continuous `count` bits
int bitmap_scan(bitmap_t *map, u32 count);
#endif // !OAK_BITMAP_H
