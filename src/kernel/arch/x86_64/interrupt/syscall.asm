; Boruix OS 系统调用中断处理
; 使用 INT 0x80 触发系统调用

section .text
global syscall_yield_entry
extern syscall_yield_handler

; 系统调用：进程让出CPU（INT 0x80）
syscall_yield_entry:
    ; CPU已经压入了SS, RSP, RFLAGS, CS, RIP
    
    ; 保存错误码和中断号
    push qword 0        ; 错误码
    push qword 0x80     ; 中断号（128）
    
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
    
    ; 调用C函数处理系统调用并决定是否切换进程
    ; 参数1: 当前进程上下文指针（栈上的寄存器）
    mov rdi, rbp
    call syscall_yield_handler
    
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
    ; 恢复所有通用寄存器
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
    
    ; 跳过中断号和错误码
    add rsp, 16
    
    ; 返回（CPU会恢复RIP, CS, RFLAGS, RSP, SS）
    iretq

