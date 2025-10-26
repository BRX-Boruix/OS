; Boruix OS 进程上下文切换汇编实现
; x86_64架构

section .text
global switch_context

; 上下文切换函数
; void switch_context(ProcessContext* from, const ProcessContext* to, uint64_t from_cr3, uint64_t to_cr3)
; 参数:
;   rdi = from (源进程上下文指针)
;   rsi = to   (目标进程上下文指针)
;   rdx = from_cr3 (源进程CR3，用于保存)
;   rcx = to_cr3   (目标进程CR3，用于加载)
switch_context:
    ; 保存当前进程的上下文到from
    ; 按照ProcessContext结构体的顺序保存寄存器
    
    ; 保存通用寄存器
    mov [rdi + 0],   rax
    mov [rdi + 8],   rbx
    mov [rdi + 16],  rcx
    mov [rdi + 24],  rdx
    mov [rdi + 32],  rsi
    mov [rdi + 40],  rdi
    mov [rdi + 48],  rbp
    mov [rdi + 56],  r8
    mov [rdi + 64],  r9
    mov [rdi + 72],  r10
    mov [rdi + 80],  r11
    mov [rdi + 88],  r12
    mov [rdi + 96],  r13
    mov [rdi + 104], r14
    mov [rdi + 112], r15
    
    ; int_no和err_code设为0
    mov qword [rdi + 120], 0
    mov qword [rdi + 128], 0
    
    ; 保存RIP (返回地址)
    mov rax, [rsp]
    mov [rdi + 136], rax
    
    ; 保存CS
    mov ax, cs
    movzx rax, ax
    mov [rdi + 144], rax
    
    ; 保存RFLAGS
    pushfq
    pop rax
    mov [rdi + 152], rax
    
    ; 保存RSP (调整后的值，跳过返回地址)
    lea rax, [rsp + 8]
    mov [rdi + 160], rax
    
    ; 保存SS
    mov ax, ss
    movzx rax, ax
    mov [rdi + 168], rax
    
    ; 切换页表(如果to_cr3不为0)
    test rcx, rcx
    jz .skip_cr3_switch
    
    ; 加载新的CR3
    mov cr3, rcx
    
.skip_cr3_switch:
    ; 恢复目标进程的上下文从to
    
    ; 恢复通用寄存器
    mov rax, [rsi + 0]
    mov rbx, [rsi + 8]
    mov rcx, [rsi + 16]
    mov rdx, [rsi + 24]
    ; rsi和rdi稍后恢复
    mov rbp, [rsi + 48]
    mov r8,  [rsi + 56]
    mov r9,  [rsi + 64]
    mov r10, [rsi + 72]
    mov r11, [rsi + 80]
    mov r12, [rsi + 88]
    mov r13, [rsi + 96]
    mov r14, [rsi + 104]
    mov r15, [rsi + 112]
    
    ; 恢复RFLAGS
    push qword [rsi + 152]
    popfq
    
    ; 恢复RSP
    mov rsp, [rsi + 160]
    
    ; 将RIP压栈，用于ret返回
    push qword [rsi + 136]
    
    ; 最后恢复rsi和rdi
    push qword [rsi + 40]  ; 保存目标rdi
    mov rdi, [rsi + 32]    ; 恢复rsi到rdi临时
    pop rsi                ; 恢复目标rdi到rsi
    xchg rsi, rdi          ; 交换，现在rsi和rdi都正确了
    
    ; 返回到新进程
    ret

; Idle进程入口点
global idle_process_entry
idle_process_entry:
.loop:
    hlt
    jmp .loop

