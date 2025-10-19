; Boruix OS x86_64内核入口点汇编文件
; Limine会直接跳转到kmain（C函数）

[BITS 64]

extern kmain

global _start

section .text

_start:
    ; Limine直接跳转到kmain
    call kmain
    
    ; 无限循环
    cli
.hang:
    hlt
    jmp .hang