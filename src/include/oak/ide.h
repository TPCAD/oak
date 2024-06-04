#ifndef OAK_IDE_H
#define OAK_IDE_H

#include <oak/mutex.h>
#include <oak/types.h>

#define SECTOR_SIZE 512 // sector size

#define IDE_CTRL_NR 2 // controller amount, fixed to 2
#define IDE_DISK_NR 2 // disk amount of each controller, fixed to 2
#define IDE_PART_NR 4 // partition amount of each disk

typedef struct part_entry_t {
    u8 bootable;             // boot flag
    u8 start_head;           // partition start head
    u8 start_sector : 6;     // partition start sector
    u16 start_cylinder : 10; // partition start cylinder
    u8 system;               // partition type
    u8 end_head;             // partition end head
    u8 end_sector : 6;       // partition end head
    u16 end_cylinder : 10;   // partition end head
    u32 start;               // partition start lba
    u32 count;               // sector amount occupied by partition
} _packed part_entry_t;

typedef struct boot_sector_t {
    u8 code[446];
    part_entry_t entry[4];
    u16 signature;
} _packed boot_sector_t;

typedef struct ide_part_t {
    char name[8];            // partition name
    struct ide_disk_t *disk; // disk pointer
    u32 system;              // partition type
    u32 start;
    u32 count;
} ide_part_t;

// IDE disk
typedef struct ide_disk_t {
    char name[8];            // disk name
    struct ide_ctrl_t *ctrl; // controller pointer
    u8 selector;             // disk selector
    bool master;             // is master
    u32 total_lba;           // available sectors
    u32 cylinders;
    u32 heads;
    u32 sectors;
    ide_part_t parts[IDE_PART_NR]; // disk partition
} ide_disk_t;

// IDE controller
typedef struct ide_ctrl_t {
    char name[8];                  // controller name
    lock_t lock;                   // controller lock
    u16 iobase;                    // IO register base address
    ide_disk_t disks[IDE_DISK_NR]; // disk
    ide_disk_t *active;            // current selected disk
    u8 control;                    // control byte
    struct task_t *waiter;         // process waitting for controller
} ide_ctrl_t;

int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, idx_t lba);
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, idx_t lba);

#endif
