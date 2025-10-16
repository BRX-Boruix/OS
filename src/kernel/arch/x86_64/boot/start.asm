; Boruix OS x86_64内核入口点汇编文件
; 实现从32位保护模式到64位长模式的切换

[BITS 32]

; 声明C函数
extern kernel_main

; 全局入口点和页表
global _start
global pml4_table
global pdp_table
global pd_table

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
    
    ; 保存multiboot参数
    mov edi, eax        ; multiboot magic
    mov esi, ebx        ; multiboot info
    
    ; 检查CPUID支持
    call check_cpuid
    test eax, eax
    jz .no_cpuid
    
    ; 检查长模式支持
    call check_long_mode
    test eax, eax
    jz .no_long_mode
    
    ; 设置页表
    call setup_page_tables
    
    ; 启用PAE
    mov eax, cr4
    or eax, 1 << 5      ; PAE位
    mov cr4, eax
    
    ; 设置页表基址
    lea eax, [pml4_table]
    mov cr3, eax
    
    ; 启用长模式
    mov ecx, 0xC0000080 ; EFER MSR
    rdmsr
    or eax, 1 << 8      ; LME位
    wrmsr
    
    ; 启用分页
    mov eax, cr0
    or eax, 1 << 31     ; PG位
    mov cr0, eax
    
    ; 加载64位GDT
    lgdt [gdt64.pointer]
    
    ; 跳转到64位代码段
    jmp gdt64.code:long_mode_start

.no_cpuid:
    mov esi, no_cpuid_msg
    call print_error
    jmp halt

.no_long_mode:
    mov esi, no_long_mode_msg
    call print_error
    jmp halt

; 检查CPUID支持
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    ret

; 检查长模式支持
check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29   ; LM位
    jz .no_long_mode
    
    mov eax, 1
    ret
    
.no_long_mode:
    xor eax, eax
    ret

; 设置页表
setup_page_tables:
    ; 清零页表
    lea edi, [pml4_table]
    xor eax, eax
    mov ecx, 4096
    rep stosd
    
    ; 设置PML4[0] -> PDP
    lea eax, [pdp_table]
    or eax, 3               ; 存在 + 可写
    lea edi, [pml4_table]
    mov [edi], eax
    
    ; 设置PDP[0] -> PD
    lea eax, [pd_table]
    or eax, 3               ; 存在 + 可写
    lea edi, [pdp_table]
    mov [edi], eax
    
    ; 设置PD[0] -> 2MB页面 (0x0 - 0x200000)
    mov eax, 0x83           ; 存在 + 可写 + 2MB页
    lea edi, [pd_table]
    mov [edi], eax
    
    ; 设置PD[1] -> 2MB页面 (0x200000 - 0x400000)
    mov eax, 0x200000 + 0x83
    mov [edi + 8], eax
    
    ret

; 简单的错误打印函数
print_error:
    mov edi, 0xb8000
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x4f        ; 白字红底
    stosw
    jmp .loop
.done:
    ret

halt:
    hlt
    jmp halt

[BITS 64]
long_mode_start:
    ; 设置段寄存器
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; 设置栈指针
    mov rsp, kernel_stack_top
    
    ; 清理寄存器
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rbp, rbp
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
    
    ; 准备参数并调用C内核主函数
    mov rdi, rdi        ; multiboot magic (已经在rdi中)
    mov rsi, rsi        ; multiboot info (已经在rsi中)
    
    ; 调用C内核主函数
    call kernel_main
    
    ; 如果内核返回，进入无限循环
    cli
.hang:
    hlt
    jmp .hang

section .rodata
; 64位GDT
gdt64:
    dq 0                ; 空描述符
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) ; 代码段
.data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41)           ; 数据段
.pointer:
    dw $ - gdt64 - 1    ; 长度
    dq gdt64            ; 地址

; 错误消息
no_cpuid_msg db 'ERROR: CPUID not supported', 0
no_long_mode_msg db 'ERROR: Long mode not supported', 0

section .bss
align 4096
; 页表结构
pml4_table:
    resb 4096
pdp_table:
    resb 4096
pd_table:
    resb 4096

; 内核栈
kernel_stack_bottom:
    resb KERNEL_STACK_SIZE
kernel_stack_top:
