/* 链接器脚本 - 用于链接支持multiboot的内核 */
/* 支持i386架构和GRUB multiboot */

ENTRY(kernel_main)

SECTIONS
{
    /* Multiboot头必须在文件开头 */
    .multiboot : ALIGN(4)
    {
        *(.multiboot)
    }
    
    /* 内核加载地址 */
    . = 0x100000;
    
    /* 内核代码段 */
    .text : ALIGN(4K)
    {
        *(.text)
        *(.text.*)
    }
    
    /* 只读数据段 */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* 可读写数据段 */
    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }
    
    /* 未初始化数据段 */
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
    
    /* 内核结束地址 */
    kernel_end = .;
}