#ifndef OAK_BITMAP_H
#define OAK_BITMAP_H

#include <oak/types.h>

typedef struct bitmap_t {
    u8 *buf;
    u32 size;
    u32 offset;
    bool high; // 高位起始或低位起始
} bitmap_t;

void bitmap_create(bitmap_t *map, u8 *buf, u32 size, u32 offset, bool high);
void bitmap_init(bitmap_t *map, u8 *buf, u32 size, u32 offset, bool high);
bool bitmap_is_set(bitmap_t *map, u32 idx);
void bitmap_set_bit(bitmap_t *map, u32 idx, bool value);
void bitmap_set_bits(bitmap_t *map, u32 idx, u32 count, bool value);
u32 bitmap_find_bits(bitmap_t *map, u32 count);

#endif // !OAK_BITMAP_H
