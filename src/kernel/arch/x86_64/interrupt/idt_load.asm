; Boruix OS x86_64架构IDT加载

[BITS 64]

global idt_load

idt_load:
    lidt [rdi]  ; 第一个参数在rdi中（x86_64调用约定）
    ret
