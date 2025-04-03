#include <oak/bitmap.h>
#include <oak/debug/kassert.h>
#include <oak/string.h>

/**
 *  @brief  创建 bitmap_t 结构体
 *  @param  map  bitmap_t
 *  @param  buf  位图数组
 *  @param  size  数组大小
 *  @param  offset  偏移量，对位图的操作会从偏移量之后开始
 */
void bitmap_create(bitmap_t *map, u8 *buf, u32 size, u32 offset) {
    map->buf = buf;
    map->size = size;
    map->offset = offset;
}

/**
 *  @brief  初始化位图
 *  @param  map  bitmap_t
 *  @param  buf  位图数组
 *  @param  size  数组大小
 *  @param  offset  偏移量，对位图的操作会从偏移量之后开始
 */
void bitmap_init(bitmap_t *map, u8 *buf, u32 size, u32 offset) {
    memset(buf, 0, size);
    bitmap_create(map, buf, size, offset);
}

/**
 *  @brief  检查指定是否已设置
 *  @param  map  bitmap_t
 *  @param  idx  位索引
 *  @return  布尔值
 */
bool bitmap_is_set(bitmap_t *map, u32 idx) {
    kassert(idx >= map->offset);

    u32 off_idx = idx - map->offset;
    u32 byte_offset = off_idx / 8;
    u32 bit_offset = off_idx % 8;

    kassert(byte_offset < map->size);

    return map->buf[byte_offset] & (0x80 >> bit_offset);
}

void bitmap_set_bit(bitmap_t *map, u32 idx, bool value) {
    kassert(idx >= map->offset);

    u32 off_idx = idx - map->offset;
    u32 byte_offset = off_idx / 8;
    u32 bit_offset = off_idx % 8;
    u32 mask = 0x80 >> bit_offset;

    kassert(byte_offset < map->size);

    if (value) {
        map->buf[byte_offset] = map->buf[byte_offset] | mask;
    } else {
        map->buf[byte_offset] = map->buf[byte_offset] & ~mask;
    }
}

void bitmap_set_bits(bitmap_t *map, u32 idx, u32 count, bool value) {
    u32 off_idx = idx - map->offset;
    u32 byte_offset = off_idx / 8;
    u32 bit_offset = off_idx % 8;

    u32 marked_bits_in_1st_byte =
        (count + bit_offset) < 8 ? count : 8 - bit_offset;

    if (value) {
        map->buf[byte_offset] |=
            (((1U << marked_bits_in_1st_byte) - 1)
             << (8 - bit_offset - marked_bits_in_1st_byte));
    } else {
        map->buf[byte_offset] &=
            ~(((1U << marked_bits_in_1st_byte) - 1)
              << (8 - bit_offset - marked_bits_in_1st_byte));
    }

    byte_offset++;

    u32 continuous_bytes = (count + bit_offset) / 8;

    for (int i = 0; continuous_bytes != 0 && i < continuous_bytes - 1;
         i++, byte_offset++) {
        map->buf[byte_offset] = value;
    }

    u32 remaining_bits = (count + bit_offset) % 8;
    if (value) {
        map->buf[byte_offset] |=
            (((1U << remaining_bits) - 1) << (8 - remaining_bits));
    } else {
        map->buf[byte_offset] &=
            ~(((1U << remaining_bits) - 1) << (8 - remaining_bits));
    }
}

u32 bitmap_find_bits(bitmap_t *map, u32 count) {
    u32 start = 0;
    u32 left_bits = map->size * 8;
    u32 next_bit = 0;
    u32 temp_count = 0;

    while (left_bits-- > 0) {
        if (!bitmap_is_set(map, map->offset + next_bit)) {
            temp_count++;
        } else {
            temp_count = 0;
        }

        next_bit++;

        if (temp_count == count) {
            start = next_bit - count;
            break;
        }
    }

    if (start == 0) {
        return 0;
    }

    // left_bits = count;
    // next_bit = start;
    bitmap_set_bits(map, map->offset + start, count, true);
    // while (left_bits--) {
    //     bitmap_set_bit(map, map->offset + next_bit, true);
    //     next_bit++;
    // }

    return map->offset + start;
}

void bitmap_test() {
    const int LEN = 8;
    const int OFFSET = 5;
    u8 bits[LEN];
    bitmap_t map;
    int count = 10;
    bitmap_init(&map, bits, LEN, OFFSET);

    bitmap_set_bit(&map, 10, true);
    kassert(bits[0] == 0b00000100);

    bitmap_set_bit(&map, 5, true);
    kassert(bits[0] == 0b10000100);

    bitmap_set_bits(&map, 16, count, true);
    kassert(bits[1] == 0b00011111);
    kassert(bits[2] == 0b11111000);
}
