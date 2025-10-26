; Boruix OS 首次进程切换
; 从内核初始化上下文切换到第一个进程

section .text
global switch_to_first_process
extern rust_get_process_context

; 切换到第一个进程
; 参数：rdi = 进程PID
switch_to_first_process:
    ; 保存PID
    push rdi
    
    ; 获取进程上下文
    call rust_get_process_context
    
    ; 检查返回值
    test rax, rax
    jz .error
    
    ; rax现在指向进程上下文
    ; ProcessContext布局：
    ; 0: rax, 8: rbx, 16: rcx, 24: rdx, 32: rsi, 40: rdi, 48: rbp
    ; 56: r8, 64: r9, 72: r10, 80: r11, 88: r12, 96: r13, 104: r14, 112: r15
    ; 120: int_no, 128: err_code
    ; 136: rip, 144: cs, 152: rflags, 160: rsp, 168: ss
    
    ; 先保存rax指针到rbx
    mov rbx, rax
    
    ; 获取进程的栈指针
    mov rsp, [rbx + 160]
    
    ; 在栈上构建iretq栈帧（从高到低：SS, RSP, RFLAGS, CS, RIP）
    push qword [rbx + 168]  ; SS
    push qword [rbx + 160]  ; RSP
    push qword [rbx + 152]  ; RFLAGS
    push qword [rbx + 144]  ; CS
    push qword [rbx + 136]  ; RIP
    
    ; 恢复所有通用寄存器（除了rax和rbx，最后恢复）
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
    
    ; 最后恢复rax和rbx
    mov rax, [rbx + 0]
    mov rbx, [rbx + 8]
    
    ; 执行中断返回，切换到进程
    iretq
    
.error:
    ; 错误：无法获取进程上下文
    pop rdi
    mov rax, -1
    ret

