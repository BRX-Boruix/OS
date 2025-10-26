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
    ; rax指向新进程的上下文（PCB中的ProcessContext结构）
    
    ; 保存rax到rbx（新进程上下文指针）
    mov rbx, rax
    
    ; 获取新进程的CR3（调用Rust FFI）
    extern rust_get_next_process_cr3
    call rust_get_next_process_cr3
    
    ; rax现在包含新进程的CR3
    ; 如果CR3不为0，加载它
    test rax, rax
    jz .skip_cr3_load
    mov cr3, rax
    
.skip_cr3_load:
    ; rbx指向新进程的上下文（PCB中的ProcessContext）
    ; ProcessContext布局：
    ; 0: rax, 8: rbx, 16: rcx, 24: rdx, 32: rsi, 40: rdi, 48: rbp
    ; 56: r8, 64: r9, 72: r10, 80: r11, 88: r12, 96: r13, 104: r14, 112: r15
    ; 120: int_no, 128: err_code
    ; 136: rip, 144: cs, 152: rflags, 160: rsp, 168: ss
    
    ; 获取新进程的栈指针
    mov rsp, [rbx + 160]
    
    ; 在栈上构建iretq栈帧
    push qword [rbx + 168]  ; SS
    push qword [rbx + 160]  ; RSP
    push qword [rbx + 152]  ; RFLAGS
    push qword [rbx + 144]  ; CS
    push qword [rbx + 136]  ; RIP
    
    ; 发送EOI到PIC
    push rax
    mov al, PIC_EOI
    out PIC1_COMMAND, al
    pop rax
    
    ; 恢复所有通用寄存器
    mov r15, [rbx + 112]
    mov r14, [rbx + 104]
    mov r13, [rbx + 96]
    mov r12, [rbx + 88]
    mov r11, [rbx + 80]
    mov r10, [rbx + 72]
    mov r9, [rbx + 64]
    mov r8, [rbx + 56]
    mov rbp, [rbx + 48]
    mov rdi, [rbx + 40]
    mov rsi, [rbx + 32]
    mov rdx, [rbx + 24]
    mov rcx, [rbx + 16]
    mov rax, [rbx + 0]
    mov rbx, [rbx + 8]
    
    ; 执行中断返回
    iretq
    
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

