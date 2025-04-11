#include "oak/debug/kassert.h"
#include "oak/debug/kdebug.h"
#include "oak/fifo.h"
#include "oak/interrupt/idt.h"
#include "oak/interrupt/pic.h"
#include "oak/io.h"
#include "oak/mutex.h"
#include "oak/task.h"
#include "oak/types.h"

#define PS2KBD_DATA_PORT 0x60
#define PS2KBD_CTRL_PORT 0x64
#define PS2KBD_CMD_LED 0xed // 设置键盘 LED
#define PS2KBD_CMD_ACK 0xfa // PS2 控制器确认的返回值

#define INV 0 // 不可见字符
#define CODE_PRINT_SCREEN_DOWN 0xb7

// 扫描码
typedef enum {
    KEY_NONE,
    KEY_ESC,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUAL,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_BRACKET_L,
    KEY_BRACKET_R,
    KEY_ENTER,
    KEY_CTRL_L,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_SEMICOLON,
    KEY_QUOTE,
    KEY_BACKQUOTE,
    KEY_SHIFT_L,
    KEY_BACKSLASH,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_POINT,
    KEY_SLASH,
    KEY_SHIFT_R,
    KEY_STAR,
    KEY_ALT_L,
    KEY_SPACE,
    KEY_CAPSLOCK,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_NUMLOCK,
    KEY_SCRLOCK,
    KEY_PAD_7,
    KEY_PAD_8,
    KEY_PAD_9,
    KEY_PAD_MINUS,
    KEY_PAD_4,
    KEY_PAD_5,
    KEY_PAD_6,
    KEY_PAD_PLUS,
    KEY_PAD_1,
    KEY_PAD_2,
    KEY_PAD_3,
    KEY_PAD_0,
    KEY_PAD_POINT,
    KEY_54,
    KEY_55,
    KEY_56,
    KEY_F11,
    KEY_F12,
    KEY_59,
    KEY_WIN_L,
    KEY_WIN_R,
    KEY_CLIPBOARD,
    KEY_5D,
    KEY_5E,

    // custom key below to fix index of keymap
    KEY_PRINT_SCREEN,
} KEY;

static char keymap[][4] = {
    /* 扫描码 0. 原始字符, 1. 与 shift 结合的字符, 2,3. 按键状态 */
    /* ---------------------------------- */
    /* 0x00 */ {INV, INV, false, false},   // NULL
    /* 0x01 */ {0x1b, 0x1b, false, false}, // ESC
    /* 0x02 */ {'1', '!', false, false},
    /* 0x03 */ {'2', '@', false, false},
    /* 0x04 */ {'3', '#', false, false},
    /* 0x05 */ {'4', '$', false, false},
    /* 0x06 */ {'5', '%', false, false},
    /* 0x07 */ {'6', '^', false, false},
    /* 0x08 */ {'7', '&', false, false},
    /* 0x09 */ {'8', '*', false, false},
    /* 0x0A */ {'9', '(', false, false},
    /* 0x0B */ {'0', ')', false, false},
    /* 0x0C */ {'-', '_', false, false},
    /* 0x0D */ {'=', '+', false, false},
    /* 0x0E */ {'\b', '\b', false, false}, // BACKSPACE
    /* 0x0F */ {'\t', '\t', false, false}, // TAB
    /* 0x10 */ {'q', 'Q', false, false},
    /* 0x11 */ {'w', 'W', false, false},
    /* 0x12 */ {'e', 'E', false, false},
    /* 0x13 */ {'r', 'R', false, false},
    /* 0x14 */ {'t', 'T', false, false},
    /* 0x15 */ {'y', 'Y', false, false},
    /* 0x16 */ {'u', 'U', false, false},
    /* 0x17 */ {'i', 'I', false, false},
    /* 0x18 */ {'o', 'O', false, false},
    /* 0x19 */ {'p', 'P', false, false},
    /* 0x1A */ {'[', '{', false, false},
    /* 0x1B */ {']', '}', false, false},
    /* 0x1C */ {'\n', '\n', false, false}, // ENTER
    /* 0x1D */ {INV, INV, false, false},   // CTRL_L
    /* 0x1E */ {'a', 'A', false, false},
    /* 0x1F */ {'s', 'S', false, false},
    /* 0x20 */ {'d', 'D', false, false},
    /* 0x21 */ {'f', 'F', false, false},
    /* 0x22 */ {'g', 'G', false, false},
    /* 0x23 */ {'h', 'H', false, false},
    /* 0x24 */ {'j', 'J', false, false},
    /* 0x25 */ {'k', 'K', false, false},
    /* 0x26 */ {'l', 'L', false, false},
    /* 0x27 */ {';', ':', false, false},
    /* 0x28 */ {'\'', '"', false, false},
    /* 0x29 */ {'`', '~', false, false},
    /* 0x2A */ {INV, INV, false, false}, // SHIFT_L
    /* 0x2B */ {'\\', '|', false, false},
    /* 0x2C */ {'z', 'Z', false, false},
    /* 0x2D */ {'x', 'X', false, false},
    /* 0x2E */ {'c', 'C', false, false},
    /* 0x2F */ {'v', 'V', false, false},
    /* 0x30 */ {'b', 'B', false, false},
    /* 0x31 */ {'n', 'N', false, false},
    /* 0x32 */ {'m', 'M', false, false},
    /* 0x33 */ {',', '<', false, false},
    /* 0x34 */ {'.', '>', false, false},
    /* 0x35 */ {'/', '?', false, false},
    /* 0x36 */ {INV, INV, false, false},  // SHIFT_R
    /* 0x37 */ {'*', '*', false, false},  // PAD *
    /* 0x38 */ {INV, INV, false, false},  // ALT_L
    /* 0x39 */ {' ', ' ', false, false},  // SPACE
    /* 0x3A */ {INV, INV, false, false},  // CAPSLOCK
    /* 0x3B */ {INV, INV, false, false},  // F1
    /* 0x3C */ {INV, INV, false, false},  // F2
    /* 0x3D */ {INV, INV, false, false},  // F3
    /* 0x3E */ {INV, INV, false, false},  // F4
    /* 0x3F */ {INV, INV, false, false},  // F5
    /* 0x40 */ {INV, INV, false, false},  // F6
    /* 0x41 */ {INV, INV, false, false},  // F7
    /* 0x42 */ {INV, INV, false, false},  // F8
    /* 0x43 */ {INV, INV, false, false},  // F9
    /* 0x44 */ {INV, INV, false, false},  // F10
    /* 0x45 */ {INV, INV, false, false},  // NUMLOCK
    /* 0x46 */ {INV, INV, false, false},  // SCRLOCK
    /* 0x47 */ {'7', INV, false, false},  // pad 7 - Home
    /* 0x48 */ {'8', INV, false, false},  // pad 8 - Up
    /* 0x49 */ {'9', INV, false, false},  // pad 9 - PageUp
    /* 0x4A */ {'-', '-', false, false},  // pad -
    /* 0x4B */ {'4', INV, false, false},  // pad 4 - Left
    /* 0x4C */ {'5', INV, false, false},  // pad 5
    /* 0x4D */ {'6', INV, false, false},  // pad 6 - Right
    /* 0x4E */ {'+', '+', false, false},  // pad +
    /* 0x4F */ {'1', INV, false, false},  // pad 1 - End
    /* 0x50 */ {'2', INV, false, false},  // pad 2 - Down
    /* 0x51 */ {'3', INV, false, false},  // pad 3 - PageDown
    /* 0x52 */ {'0', INV, false, false},  // pad 0 - Insert
    /* 0x53 */ {'.', 0x7F, false, false}, // pad . - Delete
    /* 0x54 */ {INV, INV, false, false},  //
    /* 0x55 */ {INV, INV, false, false},  //
    /* 0x56 */ {INV, INV, false, false},  //
    /* 0x57 */ {INV, INV, false, false},  // F11
    /* 0x58 */ {INV, INV, false, false},  // F12
    /* 0x59 */ {INV, INV, false, false},  //
    /* 0x5A */ {INV, INV, false, false},  //
    /* 0x5B */ {INV, INV, false, false},  // Left Windows
    /* 0x5C */ {INV, INV, false, false},  // Right Windows
    /* 0x5D */ {INV, INV, false, false},  // Clipboard
    /* 0x5E */ {INV, INV, false, false},  //

    // Print Screen is forcly deined,  it is 0xB7 oringally
    /* 0x5F */ {INV, INV, false, false}, // PrintScreen
};

static lock_t lock;
static task_t *waiter;

#define BUFFER_SIZE 64
static char buf[BUFFER_SIZE];
static fifo_t fifo;

static bool capslock_state = false;
static bool scrllock_state = false;
static bool numlock_state = false;
static bool extcode_state = false;

#define ctrl_state (keymap[KEY_CTRL_L][2] || keymap[KEY_CTRL_L][3])
#define alt_state (keymap[KEY_ALT_L][2] || keymap[KEY_ALT_L][3])
#define shift_state (keymap[KEY_SHIFT_L][2] || keymap[KEY_SHIFT_R][2])

/**
 *  @brief  等待 PS2 控制器的输入缓冲区空
 *
 *  PS2 控制器的状态寄存器（0x64）第 1 位表示其输入缓冲区状态，1 为满，0 为空。
 *  在向 PS2 控制器的寄存器写入数据前必须确保输入缓冲区为空。
 */
static void ps2kbd_wait() {
    u8 state;
    do {
        state = inb(PS2KBD_CTRL_PORT);
    } while (state & 0x02);
}

/**
 *  @brief  等待 PS2 控制器返回确定信号
 *
 *  在向 PS2 控制器发送命令后，执行无误后 PS2 控制器会返回确定信号 0xFA。
 */
static void ps2kbd_ack() {
    u8 state;
    do {
        state = inb(PS2KBD_DATA_PORT);
    } while (state != PS2KBD_CMD_ACK);
}

/**
 *  @brief  设置键盘 LED 状态
 */
static void ps2kbd_set_led() {
    u8 led = (capslock_state << 2) | (numlock_state << 1) | scrllock_state;
    ps2kbd_wait();

    outb(PS2KBD_DATA_PORT, PS2KBD_CMD_LED);
    ps2kbd_ack();

    ps2kbd_wait();

    outb(PS2KBD_DATA_PORT, led);
    ps2kbd_ack();
}

void kbd_handler(u32 vector) {
    kassert(vector == 0x21);
    pic_send_eoi(vector);

    u16 scancode = inb(PS2KBD_DATA_PORT);
    // KDEBUG("keyboard input 0x%x\n", scancode);
    u8 ext = 2;

    // 扩展码，继续获取下一个扫描码
    if (scancode == 0xe0) {
        extcode_state = true;
        return;
    }

    if (extcode_state) {
        ext = 3;
        scancode |= 0xe000;
        extcode_state = false;
    }

    u16 makecode = (scancode & 0x7f);
    if (makecode == CODE_PRINT_SCREEN_DOWN) {
        makecode = KEY_PRINT_SCREEN;
    }

    if (makecode > KEY_PRINT_SCREEN) {
        return;
    }

    bool breakcode = ((scancode & 0x0080) != 0);

    // 按键抬起
    if (breakcode) {
        keymap[makecode][ext] = false;
        return;
    }

    // 按键按下
    keymap[makecode][ext] = true;

    // 键盘 LED
    bool led = false;
    if (makecode == KEY_NUMLOCK) {
        numlock_state = !numlock_state;
        led = true;
    } else if (makecode == KEY_CAPSLOCK) {
        capslock_state = !capslock_state;
        led = true;
    } else if (makecode == KEY_SCRLOCK) {
        scrllock_state = !scrllock_state;
        led = true;
    }

    if (led) {
        ps2kbd_set_led();
    }

    bool shift = false;
    if (capslock_state && ('a' <= keymap[makecode][0]) &&
        (keymap[makecode][0] <= 'z')) {
        shift = !shift;
    }
    if (shift_state) {
        shift = !shift;
    }

    char ch = 0;
    if (ext == 3 && (makecode != KEY_SLASH)) {
        ch = keymap[makecode][1];
    } else if (ext == 3 && (makecode == KEY_SLASH)) {
        ch = keymap[makecode][0];
    } else {
        ch = keymap[makecode][shift];
    }

    if (ch == INV) {
        return;
    }

    fifo_push(&fifo, ch);
    if (waiter != NULL) {
        task_unblock(waiter);
        waiter = NULL;
    }

    // KDEBUG("keyboard input %c\n", ch);
    // KDEBUG("keyboard input 0x%x\n", scancode);
}

u32 ps2kbd_read(char *buf, u32 count) {
    lock_acquire(&lock);
    u32 nr = 0;
    while (nr < count) {
        // 若键盘输入队列为空则阻塞任务，等待键盘输入
        while (fifo_is_empty(&fifo)) {
            waiter = task_current_running();
            task_block(waiter, NULL, TASK_BLOCKED);
        }
        // 读取键盘输入
        buf[nr++] = fifo_pop(&fifo);
    }
    lock_release(&lock);
    return nr;
}

void kbd_init() {
    fifo_init(&fifo, buf, BUFFER_SIZE);
    lock_init(&lock);
    waiter = NULL;

    // TODO: init PS/2 controller(8042)
    capslock_state = false;
    scrllock_state = false;
    numlock_state = false;
    extcode_state = false;

    ps2kbd_set_led();

    idt_set_intr_handler(IRQ_KEYBOARD, kbd_handler);
    pic_set_intr_mask(IRQ_KEYBOARD, true);
}
