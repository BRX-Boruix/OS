; Boruix OS x86_64 GDT汇编支持

[BITS 64]

global gdt_load
global gdt_reload_segments

; 加载GDT
; 参数: rdi = GDT指针地址
gdt_load:
    lgdt [rdi]
    ret

; 重新加载段寄存器
gdt_reload_segments:
    ; 重新加载代码段（通过far return）
    push 0x08           ; 新的CS（GDT第1项）
    lea rax, [rel .reload_CS]
    push rax
    retfq
.reload_CS:
    ; 重新加载数据段寄存器
    mov ax, 0x10        ; 新的DS/ES/SS（GDT第2项）
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    ; FS和GS保持为0（x86_64通常不使用）
    xor ax, ax
    mov fs, ax
    mov gs, ax
    
    ret

