#ifndef OAK_IDE_H
#define OAK_IDE_H

#include <oak/mutex.h>
#include <oak/types.h>

#define SECTOR_SIZE 512 // sector size

#define IDE_CTRL_NR 2 // controller amount, fixed to 2
#define IDE_DISK_NR 2 // disk amount of each controller, fixed to 2

// IDE disk
typedef struct ide_disk_t {
    char name[8];            // disk name
    struct ide_ctrl_t *ctrl; // controller pointer
    u8 selector;             // disk selector
    bool master;             // is master
} ide_disk_t;

// IDE controller
typedef struct ide_ctrl_t {
    char name[8];                  // controller name
    lock_t lock;                   // controller lock
    u16 iobase;                    // IO register base address
    ide_disk_t disks[IDE_DISK_NR]; // disk
    ide_disk_t *active;            // current selected disk
} ide_ctrl_t;

int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, idx_t lba);
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, idx_t lba);

#endif
