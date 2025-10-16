; Boruix OS 内核入口点汇编文件
; 确保正确的multiboot参数传递

[BITS 32]

; 声明C函数
extern kernel_main

; 全局入口点
global _start

_start:
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
