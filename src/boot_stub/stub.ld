/* Limine Boot Stub 链接器脚本 */

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS {
    limine_requests PT_LOAD FLAGS(6);
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}

SECTIONS {
    . = 0xffffffff80000000;
    
    .limine_requests : ALIGN(8) {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } : limine_requests
    
    . = ALIGN(4K);
    
    .text : ALIGN(4K) {
        *(.text)
        *(.text.*)
    } : text
    
    . = ALIGN(4K);
    
    .rodata : ALIGN(4K) {
        *(.rodata)
        *(.rodata.*)
    } : rodata
    
    . = ALIGN(4K);
    
    .data : ALIGN(4K) {
        *(.data)
        *(.data.*)
    } : data
    
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    } : data
    
    . = ALIGN(4K);
}
