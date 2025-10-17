; Boruix OS i386架构IDT加载

[BITS 32]

global idt_load

idt_load:
    mov eax, [esp + 4]  ; 获取IDT指针参数
    lidt [eax]          ; 加载IDT
    ret
