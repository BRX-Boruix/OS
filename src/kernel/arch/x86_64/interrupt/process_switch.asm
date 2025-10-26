; Boruix OS 进程切换汇编实现
; 在中断中进行进程切换

section .text
global irq_timer_with_switch
extern timer_irq_handler_with_switch
extern get_next_process_context

; PIC端口定义
%define PIC1_COMMAND 0x20
%define PIC1_DATA 0x21
%define PIC2_COMMAND 0xA0
%define PIC2_DATA 0xA1
%define PIC_EOI 0x20

; 定时器中断处理（支持进程切换）
; 这个函数会被定时器中断调用
irq_timer_with_switch:
    ; CPU已经压入了SS, RSP, RFLAGS, CS, RIP
    
    ; 保存错误码和中断号（定时器中断没有错误码）
    push qword 0        ; 错误码
    push qword 32       ; 中断号（IRQ0 = 32）
    
    ; 保存所有通用寄存器
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; 此时栈上的布局与ProcessContext结构完全一致
    ; 栈指针rsp指向完整的进程上下文
    
    ; 对齐栈到16字节（x86_64 ABI要求）
    mov rbp, rsp
    and rsp, ~0xF
    
    ; 调用C函数处理定时器中断并决定是否切换进程
    ; 参数1: 当前进程上下文指针（栈上的寄存器）
    mov rdi, rbp
    call timer_irq_handler_with_switch
    
    ; rax返回值：
    ;   0 = 不切换进程，恢复当前进程
    ;   非0 = 切换到新进程，rax是新进程上下文指针
    
    ; 检查是否需要切换
    test rax, rax
    jz .no_switch
    
    ; 需要切换进程
    ; rax指向新进程的上下文
    mov rsp, rax        ; 切换到新进程的上下文栈
    jmp .restore_context
    
.no_switch:
    ; 不切换，恢复原来的栈指针
    mov rsp, rbp
    
.restore_context:
    ; 发送EOI到PIC（必须在恢复寄存器之前）
    push rax
    mov al, PIC_EOI
    out PIC1_COMMAND, al
    pop rax
    
    ; 从栈上恢复所有寄存器
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    ; 清理错误码和中断号
    add rsp, 16
    
    ; 中断返回（会自动恢复RIP, CS, RFLAGS, RSP, SS）
    iretq

