#ifndef OAK_BITMAP_H
#define OAK_BITMAP_H

#include <oak/types.h>

typedef struct bitmap_t {
    u8 *bits;   // buffer
    u32 length; // length
    u32 offset; // offset
} bitmap_t;

/* Initialize bitmap
 *
 * @param map Bitmap struct
 * @param bits Array
 * @param length Length of array in bytes
 * @param offset Start location in bits
 */
void bitmap_init(bitmap_t *map, u8 *bits, u32 length, u32 offset);

/* Make bitmap
 *
 * @param map Bitmap struct
 * @param bits Array
 * @param length Length of array in bytes
 * @param offset Start location in bits
 */
void bitmap_make(bitmap_t *map, u8 *bits, u32 length, u32 offset);

/* Check whether certain bit is set or not
 *
 * @param map Bitmap struct
 * @param index Index in bytes
 *
 * @return Boolean;
 */
bool bitmap_is_set(bitmap_t *map, u32 index);

/* Set certain bit
 *
 * @param map Bitmap struct
 * @param index Index in bytes
 * @param value Value to set
 */
void bitmap_set(bitmap_t *map, u32 index, bool value);

/* Get continuous bits
 *
 * @param map Bitmap struct
 * @param count Amount of bits
 *
 * @return Start location of continuous bits
 */
u32 bitmap_scan(bitmap_t *map, u32 count);
#endif // !OAK_BITMAP_H
