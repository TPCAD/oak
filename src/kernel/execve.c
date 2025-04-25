#include "oak/cpu.h"
#include "oak/debug/kassert.h"
#include "oak/debug/kdebug.h"
#include "oak/fs/minix.h"
#include "oak/fs/stat.h"
#include "oak/mm/memory.h"
#include "oak/mm/pmm.h"
#include "oak/mm/vmm.h"
#include "oak/stdlib.h"
#include "oak/syscall.h"
#include "oak/task.h"
#include <oak/string.h>
#include <oak/types.h>

typedef u32 Elf32_Word;
typedef u32 Elf32_Addr;
typedef u32 Elf32_Off;
typedef u16 Elf32_Half;

// ELF 文件标记
typedef struct ELFIdent {
    u8 ei_magic[4];    // 内容为 0x7F, E, L, F
    u8 ei_class;       // 文件种类 1-32位，2-64 位
    u8 ei_data;        // 标记大小端 1-小端，2-大端
    u8 ei_version;     // 与 e_version 一样，必须为 1
    u8 ei_pad[16 - 7]; // 占满 16 个字节
} __packed ELFIdent;

// ELF 文件头
typedef struct Elf32_Ehdr {
    ELFIdent e_ident;       // ELF 文件标记，文件最开始的 16 个字节
    Elf32_Half e_type;      // 文件类型，见 Etype
    Elf32_Half e_machine;   // 处理器架构类型，标记运行的 CPU，见 EMachine
    Elf32_Word e_version;   // 文件版本，见 EVersion
    Elf32_Addr e_entry;     // 程序入口地址
    Elf32_Off e_phoff;      // program header offset 程序头表在文件中的偏移量
    Elf32_Off e_shoff;      // section header offset 节头表在文件中的偏移量
    Elf32_Word e_flags;     // 处理器特殊标记
    Elf32_Half e_ehsize;    // ELF header size ELF 文件头大小
    Elf32_Half e_phentsize; // program header entry size 程序头表入口大小
    Elf32_Half e_phnum;     // program header number 程序头数量
    Elf32_Half e_shentsize; // section header entry size 节头表入口大小
    Elf32_Half e_shnum;     // section header number 节头表数量
    Elf32_Half e_shstrndx;  // 节字符串表在节头表中的索引
} Elf32_Ehdr;

// ELF 文件类型
enum Etype {
    ET_NONE = 0,        // 无类型
    ET_REL = 1,         // 可重定位文件
    ET_EXEC = 2,        // 可执行文件
    ET_DYN = 3,         // 动态链接库
    ET_CORE = 4,        // core 文件，未说明，占位
    ET_LOPROC = 0xff00, // 处理器相关低值
    ET_HIPROC = 0xffff, // 处理器相关高值
};

// ELF 机器(CPU)类型
enum EMachine {
    EM_NONE = 0,  // No machine
    EM_M32 = 1,   // AT&T WE 32100
    EM_SPARC = 2, // SPARC
    EM_386 = 3,   // Intel 80386
    EM_68K = 4,   // Motorola 68000
    EM_88K = 5,   // Motorola 88000
    EM_860 = 7,   // Intel 80860
    EM_MIPS = 8,  // MIPS RS3000
};

// ELF 文件版本
enum EVersion {
    EV_NONE = 0,    // 不可用版本
    EV_CURRENT = 1, // 当前版本
};

// 程序头
typedef struct Elf32_Phdr {
    Elf32_Word p_type;   // 段类型，见 SegmentType
    Elf32_Off p_offset;  // 段在文件中的偏移量
    Elf32_Addr p_vaddr;  // 加载到内存中的虚拟地址
    Elf32_Addr p_paddr;  // 加载到内存中的物理地址
    Elf32_Word p_filesz; // 文件中占用的字节数
    Elf32_Word p_memsz;  // 内存中占用的字节数
    Elf32_Word p_flags;  // 段标记，见 SegmentFlag
    Elf32_Word p_align;  // 段对齐约束
} Elf32_Phdr;

// 段类型
enum SegmentType {
    PT_NULL = 0,    // 未使用
    PT_LOAD = 1,    // 可加载程序段
    PT_DYNAMIC = 2, // 动态加载信息
    PT_INTERP = 3,  // 动态加载器名称
    PT_NOTE = 4,    // 一些辅助信息
    PT_SHLIB = 5,   // 保留
    PT_PHDR = 6,    // 程序头表
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7fffffff,
};

enum SegmentFlag {
    PF_X = 0x1, // 可执行
    PF_W = 0x2, // 可写
    PF_R = 0x4, // 可读
};

typedef struct Elf32_Shdr {
    Elf32_Word sh_name;      // 节名
    Elf32_Word sh_type;      // 节类型，见 SectionType
    Elf32_Word sh_flags;     // 节标记，见 SectionFlag
    Elf32_Addr sh_addr;      // 节地址
    Elf32_Off sh_offset;     // 节在文件中的偏移量
    Elf32_Word sh_size;      // 节大小
    Elf32_Word sh_link;      // 保存了头表索引链接，与节类型相关
    Elf32_Word sh_info;      // 额外信息，与节类型相关
    Elf32_Word sh_addralign; // 地址对齐约束
    Elf32_Word sh_entsize;   // 子项入口大小
} Elf32_Shdr;

enum SectionType {
    SHT_NULL = 0,            // 不可用
    SHT_PROGBITS = 1,        // 程序信息
    SHT_SYMTAB = 2,          // 符号表
    SHT_STRTAB = 3,          // 字符串表
    SHT_RELA = 4,            // 有附加重定位
    SHT_HASH = 5,            // 符号哈希表
    SHT_DYNAMIC = 6,         // 动态链接信息
    SHT_NOTE = 7,            // 标记文件信息
    SHT_NOBITS = 8,          // 该节文件中无内容
    SHT_REL = 9,             // 无附加重定位
    SHT_SHLIB = 10,          // 保留，用于非特定的语义
    SHT_DYNSYM = 11,         // 符号表
    SHT_LOPROC = 0x70000000, // 以下与处理器相关
    SHT_HIPROC = 0x7fffffff,
    SHT_LOUSER = 0x80000000,
    SHT_HIUSER = 0xffffffff,
};

enum SectionFlag {
    SHF_WRITE = 0x1,           // 执行时可写
    SHF_ALLOC = 0x2,           // 执行时占用内存，有些节执行时可以不在内存中
    SHF_EXECINSTR = 0x4,       // 包含可执行的机器指令，节里有代码
    SHF_MASKPROC = 0xf0000000, // 保留，与处理器相关
};

typedef struct Elf32_Sym {
    Elf32_Word st_name;  // 符号名称，在字符串表中的索引
    Elf32_Addr st_value; // 符号值，与具体符号相关
    Elf32_Word st_size;  // 符号的大小
    u8 st_info;          // 指定符号类型和约束属性，见 SymbolBinding
    u8 st_other;         // 为 0，无意义
    Elf32_Half st_shndx; // 符号对应的节表索引
} Elf32_Sym;

// 通过 info 获取约束
#define ELF32_ST_BIND(i) ((i) >> 4)
// 通过 info 获取类型
#define ELF32_ST_TYPE(i) ((i) & 0xF)
// 通过 约束 b 和 类型 t 获取 info
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))

// 符号约束
enum SymbolBinding {
    STB_LOCAL = 0,   // 外部不可见符号，优先级最高
    STB_GLOBAL = 1,  // 外部可见符号
    STB_WEAK = 2,    // 弱符号，外部可见，如果符号重定义，则优先级更低
    STB_LOPROC = 13, // 处理器相关低位
    STB_HIPROC = 15, // 处理器相关高位
};

// 符号类型
enum SymbolType {
    STT_NOTYPE = 0,  // 未定义
    STT_OBJECT = 1,  // 数据对象，比如 变量，数组等
    STT_FUNC = 2,    // 函数或可执行代码
    STT_SECTION = 3, // 节，用于重定位
    STT_FILE = 4,    // 文件，节索引为 SHN_ABS，约束为 STB_LOCAL，
                     // 而且优先级高于其他 STB_LOCAL 符号
    STT_LOPROC = 13, // 处理器相关
    STT_HIPROC = 15, // 处理器相关
};

/**
 *  @brief  验证 ELF 文件是否有效
 *  @param  ehdr  ELF 头
 */
static bool elf_validate(Elf32_Ehdr *ehdr) {
    // 非 ELF 文件
    if (memcmp(&ehdr->e_ident, "\177ELF\1\1\1", 7)) {
        return false;
    }
    // 非可执行文件
    if (ehdr->e_type != ET_EXEC) {
        return false;
    }
    // 非 i386 程序
    if (ehdr->e_machine != EM_386) {
        return false;
    }
    // 版本不可识别
    if (ehdr->e_version != EV_CURRENT) {
        return false;
    }

    if (ehdr->e_phentsize != sizeof(Elf32_Phdr)) {
        return false;
    }

    return true;
}

// 加载 ELF 段到内存
static void load_segment(inode_t *inode, Elf32_Phdr *phdr) {
    kassert(phdr->p_align == 0x1000);
    if ((phdr->p_vaddr & 0xfff) != 0) {
        return;
    }
    // kassert((phdr->p_vaddr & 0xfff) == 0);

    u32 vaddr = phdr->p_vaddr;
    u32 pg_count = DIV_ROUNDUP(MAX(phdr->p_memsz, phdr->p_filesz), PAGE_SIZE);

    for (size_t i = 0; i < pg_count; i++) {
        u32 addr = vaddr + i * PAGE_SIZE;
        kassert(addr >= USER_EXEC_ADDR && addr < USER_MMAP_ADDR);
        vmm_map_page((void *)addr);
    }

    inode_read(inode, (char *)vaddr, phdr->p_filesz, phdr->p_offset);
    if (phdr->p_filesz < phdr->p_memsz) {
        memset((char *)vaddr + phdr->p_filesz, 0,
               phdr->p_memsz - phdr->p_filesz);
    }

    // 段不可写则置为只读
    if ((phdr->p_flags & PF_W) == 0) {
        for (size_t i = 0; i < pg_count; i++) {
            u32 addr = vaddr + i * PAGE_SIZE;
            page_entry_t *page_tbl_entry = (page_entry_t *)PT_VADDR(DIDX(addr));
            *page_tbl_entry = PG_UNSET_WRITE(*page_tbl_entry);
            *page_tbl_entry = PG_SET_RDONLY(*page_tbl_entry);
            flush_tlb(addr);
        }
    }

    task_t *curr_task = task_current_running();
    if (phdr->p_flags == (PF_R | PF_X)) {
        curr_task->text = vaddr;
    } else if (phdr->p_flags == (PF_R | PF_W)) {
        curr_task->data = vaddr;
    }

    curr_task->end = MAX(curr_task->end, (vaddr + pg_count * PAGE_SIZE));
}

static u32 load_elf_file(inode_t *inode) {
    vmm_map_page((void *)USER_EXEC_ADDR);
    int n = 0;

    // 读取 ELF 文件头
    n = inode_read(inode, (char *)USER_EXEC_ADDR, sizeof(Elf32_Ehdr), 0);
    kassert(n == sizeof(Elf32_Ehdr));

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)USER_EXEC_ADDR;
    if (!elf_validate(ehdr)) {
        return EOF;
    }

    // 读取程序头表
    Elf32_Phdr *phdr = (Elf32_Phdr *)(USER_EXEC_ADDR + sizeof(Elf32_Ehdr));
    n = inode_read(inode, (char *)phdr, ehdr->e_phnum * ehdr->e_phentsize,
                   ehdr->e_phoff);

    Elf32_Phdr *ptr = phdr;
    for (size_t i = 0; i < ehdr->e_phnum; i++) {
        if (ptr->p_type != PT_LOAD) {
            continue;
        }
        load_segment(inode, ptr);
        ptr++;
    }
    return ehdr->e_entry;
}

extern i32 dmm_brk(void *addr);

int elf_execve(char *filename, char *argv[], char *envp[]) {
    inode_t *inode = namei(filename);
    int ret = EOF;
    if (!inode) {
        goto rollback;
    }

    // 非常规文件
    if (!ISFILE(inode->inode->mode)) {
        goto rollback;
    }

    // 非可执行文件
    if (!permission(inode, P_EXEC)) {
        goto rollback;
    }

    task_t *curr_task = task_current_running();
    strncpy(curr_task->name, filename, TASK_NAME_LEN);

    curr_task->end = USER_EXEC_ADDR;
    dmm_brk((void *)USER_EXEC_ADDR);

    u32 entry = load_elf_file(inode);
    if (entry == EOF) {
        goto rollback;
    }

    dmm_brk((void *)curr_task->end);

    inode_free(curr_task->iexec);
    curr_task->iexec = inode;

    intr_context_t *intr_cont =
        (intr_context_t *)((u32)curr_task + PAGE_SIZE - sizeof(intr_context_t));

    intr_cont->eip = entry;
    intr_cont->esp = (u32)USER_STACK_BOTTOM;

    asm volatile("movl %0, %%esp\n"
                 "jmp interrupt_exit\n" ::"m"(intr_cont));

rollback:
    inode_free(inode);
    return ret;
}

// int elf_execve(char *filename, char *argv[], char *envp[]) {
//     int fd = open(filename, O_RDONLY, 0);
//     if (fd == EOF) {
//         return EOF;
//     }
//
//     // 读 ELF 头
//     Elf32_Ehdr *ehdr = (Elf32_Ehdr *)pmm_alloc_kpage();
//     lseek(fd, 0, SEEK_SET);
//     read(fd, (char *)ehdr, sizeof(Elf32_Ehdr));
//
//     KDEBUG("ELF ident %s\n", ehdr->e_ident.ei_magic);
//     KDEBUG("ELF class %d\n", ehdr->e_ident.ei_class);
//     KDEBUG("ELF data %d\n", ehdr->e_ident.ei_data);
//     KDEBUG("ELF type %d\n", ehdr->e_type);
//     KDEBUG("ELF machine %d\n", ehdr->e_machine);
//     KDEBUG("ELF entry 0x%p\n", ehdr->e_entry);
//     KDEBUG("ELF ehsize %d %d\n", ehdr->e_ehsize, sizeof(Elf32_Ehdr));
//     KDEBUG("ELF phoff %d\n", ehdr->e_phoff);
//     KDEBUG("ELF phnum %d\n", ehdr->e_phnum);
//     KDEBUG("ELF phsize %d %d\n", ehdr->e_phentsize, sizeof(Elf32_Phdr));
//     KDEBUG("ELF shoff %d\n", ehdr->e_shoff);
//     KDEBUG("ELF shnum %d\n", ehdr->e_shnum);
//     KDEBUG("ELF shsize %d %d\n", ehdr->e_shentsize, sizeof(Elf32_Shdr));
//
//     // 读 ELF 段头
//     Elf32_Phdr *phdr = (Elf32_Phdr *)pmm_alloc_kpage();
//     lseek(fd, ehdr->e_phoff, SEEK_SET);
//     read(fd, (char *)phdr, ehdr->e_phentsize * ehdr->e_phnum);
//
//     KDEBUG("ELF segment size mem %d\n", ehdr->e_phentsize * ehdr->e_phnum);
//
//     Elf32_Phdr *ptr = phdr;
//     // 内容
//     char *content = (char *)pmm_alloc_kpage();
//     for (size_t i = 0; i < ehdr->e_phnum; i++) {
//         memset(content, 0, PAGE_SIZE);
//         lseek(fd, ptr->p_offset, SEEK_SET);
//         read(fd, content, ptr->p_filesz);
//         KDEBUG("segment vaddr 0x%p paddr 0x%p\n", ptr->p_vaddr,
//         ptr->p_paddr); ptr++;
//     }
//
//     // 节头
//     Elf32_Shdr *shdr = (Elf32_Shdr *)pmm_alloc_kpage();
//     lseek(fd, ehdr->e_shoff, SEEK_SET);
//     read(fd, (char *)shdr, ehdr->e_shentsize * ehdr->e_shnum);
//     KDEBUG("ELF section size mem %d\n", ehdr->e_shentsize * ehdr->e_shnum);
//
//     // 节字符串表
//     char *shstrtab = (char *)pmm_alloc_kpage();
//     Elf32_Shdr *sptr = &shdr[ehdr->e_shstrndx];
//     kassert(sptr->sh_type == SHT_STRTAB);
//     kassert(sptr->sh_size > 0);
//
//     lseek(fd, sptr->sh_offset, SEEK_SET);
//     read(fd, shstrtab, sptr->sh_size);
//     char *name = shstrtab + 1;
//     while (*name) {
//         KDEBUG("section name %s\n", name);
//         name += strlen(name) + 1;
//     };
//
//     char *strtab = (char *)pmm_alloc_kpage();
//
//     sptr = shdr;
//     int strtab_idx = 0;
//     int symtab_idx = 0;
//
//     for (size_t i = 0; i < ehdr->e_shnum; i++) {
//         char *sname = &shstrtab[sptr->sh_name];
//         KDEBUG("section %s size %d vaddr 0x%p\n", sname, sptr->sh_size,
//                sptr->sh_addr);
//
//         if (!strcmp(sname, ".strtab"))
//             strtab_idx = i;
//         else if (!strcmp(sname, ".symtab"))
//             symtab_idx = i;
//         sptr++;
//     }
//
//     // 程序字符串表
//     sptr = &shdr[strtab_idx];
//     kassert(sptr->sh_size > 0);
//     lseek(fd, sptr->sh_offset, SEEK_SET);
//     read(fd, strtab, sptr->sh_size);
//     name = strtab + 1;
//     while (*name) {
//         KDEBUG("symbol name %s\n", name);
//         name += strlen(name) + 1;
//     };
//
//     // 符号表
//     Elf32_Sym *symtab = (Elf32_Sym *)pmm_alloc_kpage();
//     sptr = &shdr[symtab_idx];
//     kassert(sptr->sh_size > 0);
//     lseek(fd, sptr->sh_offset, SEEK_SET);
//     read(fd, (char *)symtab, sptr->sh_size);
//
//     Elf32_Sym *symptr = symtab;
//     int encount = sptr->sh_size / sptr->sh_entsize;
//     for (size_t i = 0; i < encount; i++) {
//         KDEBUG("v: 0x%X size: %d sec: %d b: %d t: %d n: %s\n",
//         symptr->st_value,
//                symptr->st_size, symptr->st_shndx,
//                ELF32_ST_BIND(symptr->st_info),
//                ELF32_ST_TYPE(symptr->st_info), &strtab[symptr->st_name]);
//         symptr++;
//     }
//
//     pmm_free_kpage((void *)ehdr);
//     pmm_free_kpage((void *)phdr);
//     pmm_free_kpage((void *)shdr);
//     pmm_free_kpage((void *)content);
//     pmm_free_kpage((void *)strtab);
//     pmm_free_kpage((void *)symtab);
//     pmm_free_kpage((void *)shstrtab);
//     return 0;
// }
