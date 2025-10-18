; Boruix OS i386内核入口点汇编文件
; 确保正确的multiboot参数传递和栈设置

[BITS 32]

; 声明C函数
extern kernel_main

; 全局入口点
global _start

; 常量定义
KERNEL_STACK_SIZE equ 16384

; Multiboot头 - 必须在文件开头
section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002               ; multiboot magic
    dd 0x00000003               ; flags (align modules on 4KB, provide memory map)
    dd -(0x1BADB002 + 0x00000003)  ; checksum

section .text
_start:
    ; 禁用中断
    cli
    
    ; 设置栈指针（非常重要！）
    mov esp, kernel_stack_top
    mov ebp, 0
    
    ; 保存multiboot参数
    push ebx        ; multiboot_info
    push eax        ; magic
    
    ; 调用C内核主函数
    call kernel_main
    
    ; 如果内核返回，进入无限循环
    cli
.hang:
    hlt
    jmp .hang

section .bss
align 16
; 内核栈
kernel_stack_bottom:
    resb KERNEL_STACK_SIZE
kernel_stack_top:
