; Boruix OS x86_64 TSS汇编支持

[BITS 64]

global tss_load

; 加载TSS
; 参数: rdi = TSS选择子
tss_load:
    ltr di
    ret

