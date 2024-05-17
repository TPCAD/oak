#include "oak/debug.h"
#include <oak/assert.h>
#include <oak/bitmap.h>
#include <oak/string.h>
#include <oak/types.h>

void bitmap_make(bitmap_t *map, u8 *bits, u32 length) {
    map->bits = bits;
    map->length = length;
}

void bitmap_init(bitmap_t *map, u8 *bits, u32 length) {
    memset(bits, 0, length);
    bitmap_make(map, bits, length);
}

bool bitmap_if_set(bitmap_t *map, u32 index) {
    u32 bytes = index / 8;
    u8 bits = index % 8;

    assert(bytes < map->length);

    return (map->bits[bytes] & (1 << bits));
}

void bitmap_set(bitmap_t *map, u32 index, bool value) {
    assert(value == 0 || value == 1);

    u32 bytes = index / 8;
    u8 bits = index % 8;

    assert(bytes < map->length);

    if (value) {
        // set 1
        map->bits[bytes] |= (1 << bits);
    } else {
        // set 0
        map->bits[bytes] &= (1 << bits);
    }
}

int bitmap_scan(bitmap_t *map, u32 count) {
    int start = EOF;
    u32 bits_left = map->length * 8;
    u32 next_bit = 0;
    u32 counter = 0;

    while (bits_left-- > 0) {
        if (!bitmap_if_set(map, next_bit)) {
            counter++;
        } else {
            counter = 0;
        }

        next_bit++;

        if (counter == count) {
            start = next_bit - count;
            break;
        }
    }

    if (start == EOF) {
        return EOF;
    }

    bits_left = count;
    next_bit = start;
    while (bits_left--) {
        bitmap_set(map, next_bit, true);
        next_bit++;
    }

    return start;
}

void bitmap_test() {
    int LEN = 2;
    u8 bits[LEN];
    bitmap_t map;
    int count = 10;

    bitmap_init(&map, bits, LEN);

    if (bitmap_scan(&map, count) != EOF) {
        DEBUGK("Get %d bits", count);
    } else {
        DEBUGK("Failed");
    }
}
