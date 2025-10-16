; 简单的引导加载程序
; 使用NASM汇编器编写，支持i386架构

[BITS 16]                    ; 16位模式
[ORG 0x7C00]                ; 引导扇区加载地址

start:
    ; 清屏
    mov ax, 0x0003
    int 0x10
    
    ; 设置段寄存器
    mov ax, 0x0000
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; 显示欢迎消息
    mov si, welcome_msg
    call print_string
    
    ; 加载内核
    call load_kernel
    
    ; 跳转到内核
    jmp 0x1000:0x0000
    
print_string:
    pusha
    mov ah, 0x0E            ; BIOS视频服务
print_loop:
    lodsb                   ; 加载字符到AL
    cmp al, 0               ; 检查字符串结束
    je print_done
    int 0x10                ; 打印字符
    jmp print_loop
print_done:
    popa
    ret

load_kernel:
    ; 从软盘加载内核到内存0x10000
    mov ah, 0x02            ; BIOS读取扇区功能
    mov al, 1               ; 读取1个扇区
    mov ch, 0               ; 柱面0
    mov cl, 2               ; 扇区2
    mov dh, 0               ; 磁头0
    mov dl, 0               ; 驱动器A
    mov bx, 0x1000          ; 目标地址段
    mov es, bx
    mov bx, 0x0000          ; 目标地址偏移
    int 0x13                ; BIOS磁盘服务
    ret

welcome_msg db 'Boruix OS Bootloader', 0x0D, 0x0A, 'Loading kernel...', 0x0D, 0x0A, 0

; 填充到510字节
times 510-($-$$) db 0

; 引导扇区标识
dw 0xAA55
