; Boruix OS x86_64 TSS汇编支持

[BITS 64]

global tss_load
global get_gdt_base
global get_gdt_limit

; 加载TSS
; 参数: rdi = TSS选择子
tss_load:
    ltr di
    ret

; 获取GDT基址
get_gdt_base:
    sub rsp, 10
    sgdt [rsp]
    mov rax, [rsp + 2]  ; GDT基址在偏移2处
    add rsp, 10
    ret

; 获取GDT限制
get_gdt_limit:
    sub rsp, 10
    sgdt [rsp]
    movzx rax, word [rsp]  ; GDT限制在偏移0处
    add rsp, 10
    ret

