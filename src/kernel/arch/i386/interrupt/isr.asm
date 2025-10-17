; Boruix OS i386架构中断服务程序(ISR)

[BITS 32]

; 导出ISR处理程序
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
global isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

; 导出IRQ处理程序  
global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
global irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15

; 外部C函数
extern isr_handler
extern irq_handler

; 宏：无错误码的ISR
%macro ISR_NOERRCODE 1
isr%1:
    cli
    push byte 0         ; 压入伪错误码
    push byte %1        ; 压入中断号
    jmp isr_common_stub
%endmacro

; 宏：有错误码的ISR
%macro ISR_ERRCODE 1
isr%1:
    cli
    push byte %1        ; 压入中断号
    jmp isr_common_stub
%endmacro

; 宏：IRQ处理程序
%macro IRQ 2
irq%1:
    cli
    push byte 0         ; 伪错误码
    push byte %2        ; 中断号（IRQ号+32）
    jmp irq_common_stub
%endmacro

; CPU异常（0-31）
ISR_NOERRCODE 0     ; Division By Zero
ISR_NOERRCODE 1     ; Debug
ISR_NOERRCODE 2     ; Non Maskable Interrupt
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; Bound Range
ISR_NOERRCODE 6     ; Invalid Opcode
ISR_NOERRCODE 7     ; Device Not Available
ISR_ERRCODE 8       ; Double Fault (有错误码)
ISR_NOERRCODE 9     ; Coprocessor Segment Overrun
ISR_ERRCODE 10      ; Invalid TSS (有错误码)
ISR_ERRCODE 11      ; Segment Not Present (有错误码)
ISR_ERRCODE 12      ; Stack Fault (有错误码)
ISR_ERRCODE 13      ; General Protection Fault (有错误码)
ISR_ERRCODE 14      ; Page Fault (有错误码)
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; x87 Floating Point
ISR_ERRCODE 17      ; Alignment Check (有错误码)
ISR_NOERRCODE 18    ; Machine Check
ISR_NOERRCODE 19    ; SIMD Floating Point
ISR_NOERRCODE 20    ; Virtualization
ISR_NOERRCODE 21    ; Reserved
ISR_NOERRCODE 22    ; Reserved
ISR_NOERRCODE 23    ; Reserved
ISR_NOERRCODE 24    ; Reserved
ISR_NOERRCODE 25    ; Reserved
ISR_NOERRCODE 26    ; Reserved
ISR_NOERRCODE 27    ; Reserved
ISR_NOERRCODE 28    ; Reserved
ISR_NOERRCODE 29    ; Reserved
ISR_NOERRCODE 30    ; Reserved
ISR_NOERRCODE 31    ; Reserved

; IRQ（32-47）
IRQ 0, 32   ; Timer
IRQ 1, 33   ; Keyboard
IRQ 2, 34   ; Cascade
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; Floppy
IRQ 7, 39   ; LPT1
IRQ 8, 40   ; RTC
IRQ 9, 41   ; Free
IRQ 10, 42  ; Free
IRQ 11, 43  ; Free
IRQ 12, 44  ; PS2 Mouse
IRQ 13, 45  ; FPU
IRQ 14, 46  ; Primary ATA
IRQ 15, 47  ; Secondary ATA

; ISR通用存根
isr_common_stub:
    pusha               ; 保存所有通用寄存器
    
    mov ax, ds          ; 保存数据段
    push eax
    
    mov ax, 0x10        ; 加载内核数据段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call isr_handler    ; 调用C处理函数
    
    pop eax             ; 恢复数据段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa                ; 恢复所有通用寄存器
    add esp, 8          ; 清理错误码和中断号
    sti
    iret                ; 从中断返回

; IRQ通用存根
irq_common_stub:
    pusha
    
    mov ax, ds
    push eax
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call irq_handler    ; 调用C处理函数
    
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa
    add esp, 8
    sti
    iret
