#include <oak/assert.h>
#include <oak/bitmap.h>
#include <oak/debug.h>
#include <oak/string.h>
#include <oak/types.h>

/* Make bitmap
 *
 * @param map Bitmap struct
 * @param bits Arrage
 * @param length Length of arragy in bytes
 * @param offset Start location in bits
 */
void bitmap_make(bitmap_t *map, u8 *bits, u32 length, u32 offset) {
    map->bits = bits;
    map->length = length;
    map->offset = offset;
}

/* Initialize bitmap
 *
 * @param map Bitmap struct
 * @param bits Arrage
 * @param length Length of arragy in bytes
 * @param offset Start location in bits
 */
void bitmap_init(bitmap_t *map, u8 *bits, u32 length, u32 offset) {
    memset(bits, 0, length);
    bitmap_make(map, bits, length, offset);
}

/* Check whether certain bit is set or not
 *
 * @param map Bitmap struct
 * @param index Index in bytes
 *
 * @return Boolean;
 */
bool bitmap_is_set(bitmap_t *map, u32 index) {
    assert(index >= map->offset);
    idx_t idx = index - map->offset;
    u32 bytes = idx / 8;
    u8 bits = idx % 8;

    assert(bytes < map->length);

    return (map->bits[bytes] & (1 << bits));
}

/* Set certain bit
 *
 * @param map Bitmap struct
 * @param index Index in bytes
 * @param value Value to set
 */
void bitmap_set(bitmap_t *map, u32 index, bool value) {
    assert(index >= map->offset);
    assert(value == 0 || value == 1);

    idx_t idx = index - map->offset;
    u32 bytes = idx / 8;
    u8 bits = idx % 8;

    assert(bytes < map->length);

    if (value) {
        // set 1
        map->bits[bytes] |= (1 << bits);
    } else {
        // set 0
        map->bits[bytes] &= ~(1 << bits);
    }
}

/* Get continuous bits
 *
 * @param map Bitmap struct
 * @param count Amount of bits
 *
 * @return Start location of continuous bits
 */
u32 bitmap_scan(bitmap_t *map, u32 count) {
    int start = EOF;
    u32 bits_left = map->length * 8;
    u32 next_bit = 0;
    u32 counter = 0;

    while (bits_left-- > 0) {
        if (!bitmap_is_set(map, map->offset + next_bit)) {
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
        bitmap_set(map, map->offset + next_bit, true);
        next_bit++;
    }

    return start + map->offset;
}

void bitmap_test() {
    int LEN = 2;
    int OFFSET = 3;
    u8 bits[LEN];
    bitmap_t map;
    int count = 10;

    bitmap_init(&map, bits, LEN, OFFSET);

    if (bitmap_scan(&map, count) != EOF) {
        DEBUGK("Get %d bits", count);
    } else {
        DEBUGK("Failed");
    }
}
