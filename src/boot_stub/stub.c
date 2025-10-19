// Boruix OS - Limine Boot Stub
// 一个最小的Limine兼容程序，用于验证Limine协议并引导主内核

// 基础类型定义（不使用标准库）
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef uint8_t bool;
#define true 1
#define false 0
#define NULL ((void*)0)

// 复制limine.h的核心部分
#define LIMINE_PTR(TYPE) TYPE

#define LIMINE_COMMON_MAGIC 0xc7b1dd30df4c8b88, 0x0a82e883a194f07b

// Framebuffer请求
#define LIMINE_FRAMEBUFFER_REQUEST {LIMINE_COMMON_MAGIC, 0x9d5827dcd881dd75, 0xa3148604f6fab11b}

struct limine_framebuffer {
    void *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
};

struct limine_framebuffer_response {
    uint64_t revision;
    uint64_t framebuffer_count;
    struct limine_framebuffer **framebuffers;
};

struct limine_framebuffer_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_framebuffer_response *response;
};

// 模块请求（用于加载kernel.bin）
#define LIMINE_MODULE_REQUEST {LIMINE_COMMON_MAGIC, 0x3e7e279702be32af, 0xca1c4f3bd1280cee}

struct limine_uuid {
    uint32_t a;
    uint16_t b;
    uint16_t c;
    uint8_t d[8];
};

struct limine_file {
    uint64_t revision;
    void *address;
    uint64_t size;
    char *path;
    char *cmdline;
    uint32_t media_type;
    uint32_t unused;
    uint32_t tftp_ip;
    uint32_t tftp_port;
    uint32_t partition_index;
    uint32_t mbr_disk_id;
    struct limine_uuid gpt_disk_uuid;
    struct limine_uuid gpt_part_uuid;
    struct limine_uuid part_uuid;
};

struct limine_module_response {
    uint64_t revision;
    uint64_t module_count;
    struct limine_file **modules;
};

struct limine_module_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_module_response *response;
    uint64_t internal_module_count;
    struct limine_internal_module **internal_modules;
};

struct limine_internal_module {
    char *path;
    char *cmdline;
    uint64_t flags;
};

// Limine请求标记
#define LIMINE_REQUESTS_START_MARKER \
    uint64_t limine_requests_start_marker[4] = {0xf6b8f4b39de7d1ae, 0xfab91a6940fcb9cf, \
                                                0x785c6ed015d3e316, 0x181e920a7852b9d9};

#define LIMINE_REQUESTS_END_MARKER \
    uint64_t limine_requests_end_marker[2] = {0xadc0e0531bb10d03, 0x9572709f31764c62};

#define LIMINE_BASE_REVISION(N) \
    uint64_t limine_base_revision[3] = {0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, (N)};

// === Limine请求定义 ===
__attribute__((used, section(".limine_requests_start")))
static LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = NULL
};

// Internal module数组 - 让Limine加载kernel.bin
static struct limine_internal_module kernel_module = {
    .path = "boot():/boot/kernel.bin",
    .cmdline = NULL,
    .flags = 0
};

static struct limine_internal_module *kernel_module_ptr = &kernel_module;

__attribute__((used, section(".limine_requests")))
static struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 1,
    .response = NULL,
    .internal_module_count = 1,
    .internal_modules = &kernel_module_ptr
};

__attribute__((used, section(".limine_requests")))
static LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests_end")))
static LIMINE_REQUESTS_END_MARKER;

// === 简单的framebuffer输出函数 ===
static struct limine_framebuffer *fb = NULL;

static void putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb || x >= fb->width || y >= fb->height) return;
    uint32_t *pixel = (uint32_t*)((uint8_t*)fb->address + y * fb->pitch + x * 4);
    *pixel = color;
}

static void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t dy = 0; dy < h; dy++) {
        for (uint32_t dx = 0; dx < w; dx++) {
            putpixel(x + dx, y + dy, color);
        }
    }
}

static void draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    // 简单的8x8字符绘制 (只画几个基本字符)
    static const uint8_t font_O[8] = {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C};
    static const uint8_t font_K[8] = {0x42, 0x44, 0x48, 0x50, 0x68, 0x44, 0x42, 0x42};
    
    const uint8_t *font = NULL;
    if (c == 'O') font = font_O;
    else if (c == 'K') font = font_K;
    else return;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (font[row] & (1 << (7 - col))) {
                putpixel(x + col, y + row, color);
            }
        }
    }
}

static void draw_string(uint32_t x, uint32_t y, const char *str, uint32_t color) {
    uint32_t cx = x;
    while (*str) {
        draw_char(cx, y, *str, color);
        cx += 10;
        str++;
    }
}

// === ELF64解析 ===
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define ELFCLASS64 2
#define EM_X86_64 62
#define PT_LOAD 1

typedef struct {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

// 简单的内存复制和清零
static void *memcpy(void *dest, const void *src, uint64_t n) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    for (uint64_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

static void *memset(void *s, int c, uint64_t n) {
    uint8_t *p = (uint8_t*)s;
    for (uint64_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

// 验证ELF头
static bool verify_elf(Elf64_Ehdr *ehdr) {
    return (ehdr->e_ident[EI_MAG0] == 0x7F &&
            ehdr->e_ident[EI_MAG1] == 'E' &&
            ehdr->e_ident[EI_MAG2] == 'L' &&
            ehdr->e_ident[EI_MAG3] == 'F' &&
            ehdr->e_ident[EI_CLASS] == ELFCLASS64 &&
            ehdr->e_machine == EM_X86_64);
}

// 加载ELF内核
static uint64_t load_kernel_elf(void *kernel_data, uint64_t size) {
    (void)size; // 暂时不使用size参数
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)kernel_data;
    
    if (!verify_elf(ehdr)) {
        return 0; // 无效的ELF
    }
    
    // 加载每个PT_LOAD段
    Elf64_Phdr *phdr = (Elf64_Phdr*)((uint8_t*)kernel_data + ehdr->e_phoff);
    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            void *dest = (void*)phdr[i].p_vaddr;
            void *src = (uint8_t*)kernel_data + phdr[i].p_offset;
            
            // 复制文件内容
            memcpy(dest, src, phdr[i].p_filesz);
            
            // 清零BSS段
            if (phdr[i].p_memsz > phdr[i].p_filesz) {
                memset((uint8_t*)dest + phdr[i].p_filesz, 0, 
                       phdr[i].p_memsz - phdr[i].p_filesz);
            }
        }
    }
    
    return ehdr->e_entry;
}

// === 主入口点 ===
void _start(void) {
    // 检查Limine响应
    if (framebuffer_request.response == NULL) {
        // 无法获取framebuffer，停机
        while (1) __asm__("hlt");
    }
    
    struct limine_framebuffer_response *fb_response = framebuffer_request.response;
    if (fb_response->framebuffer_count == 0) {
        while (1) __asm__("hlt");
    }
    
    fb = fb_response->framebuffers[0];
    
    // 清屏为蓝色
    draw_rect(0, 0, fb->width, fb->height, 0x001F3FFF);
    
    // 绘制OK字样（表示Limine工作正常）
    draw_string(100, 100, "OK", 0x00FFFFFF);
    
    // 检查模块请求响应
    if (module_request.response == NULL || module_request.response->module_count == 0) {
        // 无法加载内核模块，停机
        while (1) __asm__("hlt");
    }
    
    // 获取kernel.bin模块
    struct limine_file *kernel_file = module_request.response->modules[0];
    
    // 加载ELF内核
    uint64_t entry_point = load_kernel_elf(kernel_file->address, kernel_file->size);
    
    if (entry_point == 0) {
        // ELF解析失败，停机
        while (1) __asm__("hlt");
    }
    
    // 跳转到内核入口点
    void (*kernel_entry)(void) = (void (*)(void))entry_point;
    kernel_entry();
    
    // 如果内核返回，停机
    while (1) {
        __asm__("hlt");
    }
}
