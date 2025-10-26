//! Boruix OS 进程管理系统 - Zig实现
//! 作为Rust实现的补充，提供高性能的进程管理功能

const std = @import("std");

// 进程ID类型
pub const ProcessId = u32;
pub const INVALID_PID: ProcessId = 0;
pub const MAX_PROCESSES: usize = 256;

// 进程状态枚举
pub const ProcessState = enum(u8) {
    Created = 0,
    Ready = 1,
    Running = 2,
    Blocked = 3,
    Zombie = 4,
    Terminated = 5,
};

// 调度优先级
pub const Priority = enum(u8) {
    Realtime = 0,
    High = 1,
    Normal = 2,
    Low = 3,
    Idle = 4,
};

// 进程上下文结构（与Rust保持一致）
pub const ProcessContext = extern struct {
    rax: u64,
    rbx: u64,
    rcx: u64,
    rdx: u64,
    rsi: u64,
    rdi: u64,
    rbp: u64,
    r8: u64,
    r9: u64,
    r10: u64,
    r11: u64,
    r12: u64,
    r13: u64,
    r14: u64,
    r15: u64,
    int_no: u64,
    err_code: u64,
    rip: u64,
    cs: u64,
    rflags: u64,
    rsp: u64,
    ss: u64,

    pub fn init() ProcessContext {
        return ProcessContext{
            .rax = 0,
            .rbx = 0,
            .rcx = 0,
            .rdx = 0,
            .rsi = 0,
            .rdi = 0,
            .rbp = 0,
            .r8 = 0,
            .r9 = 0,
            .r10 = 0,
            .r11 = 0,
            .r12 = 0,
            .r13 = 0,
            .r14 = 0,
            .r15 = 0,
            .int_no = 0,
            .err_code = 0,
            .rip = 0,
            .cs = 0x08, // 内核代码段
            .rflags = 0x202, // IF=1, 保留位=1
            .rsp = 0,
            .ss = 0x10, // 内核数据段
        };
    }

    pub fn initKernelContext(self: *ProcessContext, entry_point: usize, stack_ptr: usize) void {
        self.rip = entry_point;
        self.rsp = stack_ptr;
        self.cs = 0x08; // 内核代码段
        self.ss = 0x10; // 内核数据段
        self.rflags = 0x202; // IF=1
    }

    pub fn initUserContext(self: *ProcessContext, entry_point: usize, stack_ptr: usize) void {
        self.rip = entry_point;
        self.rsp = stack_ptr;
        self.cs = 0x1B; // 用户代码段 (GDT[3] | RPL=3)
        self.ss = 0x23; // 用户数据段 (GDT[4] | RPL=3)
        self.rflags = 0x202; // IF=1
    }
};

// 进程控制块（简化版）
pub const ProcessControlBlock = extern struct {
    pid: ProcessId,
    parent_pid: ProcessId,
    name: [32]u8,
    state: ProcessState,
    priority: Priority,
    context: ProcessContext,
    kernel_stack_ptr: ?usize,
    kernel_stack_base: ?usize,
    user_stack_ptr: ?usize,
    user_stack_base: ?usize,
    entry_point: ?usize,
    exit_code: ?i32,
    time_slice: u32,
    cpu_time: u64,
    created_at: u64,

    pub fn init(pid: ProcessId, name: []const u8) ProcessControlBlock {
        var pcb = ProcessControlBlock{
            .pid = pid,
            .parent_pid = 0,
            .name = [_]u8{0} ** 32,
            .state = ProcessState.Created,
            .priority = Priority.Normal,
            .context = ProcessContext.init(),
            .kernel_stack_ptr = null,
            .kernel_stack_base = null,
            .user_stack_ptr = null,
            .user_stack_base = null,
            .entry_point = null,
            .exit_code = null,
            .time_slice = 0,
            .cpu_time = 0,
            .created_at = getTimestamp(),
        };

        // 设置进程名称
        const len = @min(name.len, 31);
        @memcpy(pcb.name[0..len], name[0..len]);
        pcb.name[len] = 0;

        return pcb;
    }
};

// 进程队列（环形缓冲区实现）
pub const ProcessQueue = struct {
    const MAX_QUEUE_SIZE = 256;

    data: [MAX_QUEUE_SIZE]ProcessId,
    head: usize,
    tail: usize,
    size: usize,

    pub fn init() ProcessQueue {
        return ProcessQueue{
            .data = [_]ProcessId{0} ** MAX_QUEUE_SIZE,
            .head = 0,
            .tail = 0,
            .size = 0,
        };
    }

    pub fn enqueue(self: *ProcessQueue, pid: ProcessId) !void {
        if (self.size >= MAX_QUEUE_SIZE) {
            return error.QueueFull;
        }

        self.data[self.tail] = pid;
        self.tail = (self.tail + 1) % MAX_QUEUE_SIZE;
        self.size += 1;
    }

    pub fn dequeue(self: *ProcessQueue) ?ProcessId {
        if (self.size == 0) {
            return null;
        }

        const pid = self.data[self.head];
        self.head = (self.head + 1) % MAX_QUEUE_SIZE;
        self.size -= 1;
        return pid;
    }

    pub fn peek(self: *const ProcessQueue) ?ProcessId {
        if (self.size == 0) {
            return null;
        }
        return self.data[self.head];
    }

    pub fn isEmpty(self: *const ProcessQueue) bool {
        return self.size == 0;
    }

    pub fn isFull(self: *const ProcessQueue) bool {
        return self.size >= MAX_QUEUE_SIZE;
    }

    pub fn len(self: *const ProcessQueue) usize {
        return self.size;
    }

    pub fn clear(self: *ProcessQueue) void {
        self.head = 0;
        self.tail = 0;
        self.size = 0;
    }

    pub fn contains(self: *const ProcessQueue, pid: ProcessId) bool {
        if (self.size == 0) return false;

        var index = self.head;
        var i: usize = 0;
        while (i < self.size) : (i += 1) {
            if (self.data[index] == pid) {
                return true;
            }
            index = (index + 1) % MAX_QUEUE_SIZE;
        }
        return false;
    }

    pub fn remove(self: *ProcessQueue, pid: ProcessId) bool {
        if (self.size == 0) return false;

        var index = self.head;
        var i: usize = 0;
        while (i < self.size) : (i += 1) {
            if (self.data[index] == pid) {
                // 找到了，移除元素
                var current = index;
                var j = i;
                while (j < self.size - 1) : (j += 1) {
                    const next = (current + 1) % MAX_QUEUE_SIZE;
                    self.data[current] = self.data[next];
                    current = next;
                }

                self.tail = if (self.tail == 0) MAX_QUEUE_SIZE - 1 else self.tail - 1;
                self.size -= 1;
                return true;
            }
            index = (index + 1) % MAX_QUEUE_SIZE;
        }
        return false;
    }
};

// 调度器统计信息
pub const SchedulerStats = extern struct {
    total_schedules: u64,
    context_switches: u64,
    preemptions: u64,
    idle_time: u64,
    priority_schedules: [5]u64,

    pub fn init() SchedulerStats {
        return SchedulerStats{
            .total_schedules = 0,
            .context_switches = 0,
            .preemptions = 0,
            .idle_time = 0,
            .priority_schedules = [_]u64{0} ** 5,
        };
    }
};

// 调度策略
pub const SchedulingPolicy = enum(u8) {
    RoundRobin = 0,
    Priority = 1,
    MultilevelFeedback = 2,
};

// 轮转调度器
pub const RoundRobinScheduler = struct {
    policy: SchedulingPolicy,
    current_process: ?ProcessId,
    stats: SchedulerStats,
    time_slice_counter: u32,
    enabled: bool,

    const TIME_SLICE_MS: u32 = 10;

    pub fn init() RoundRobinScheduler {
        return RoundRobinScheduler{
            .policy = SchedulingPolicy.RoundRobin,
            .current_process = null,
            .stats = SchedulerStats.init(),
            .time_slice_counter = TIME_SLICE_MS,
            .enabled = false,
        };
    }

    pub fn enable(self: *RoundRobinScheduler) void {
        self.enabled = true;
        self.time_slice_counter = TIME_SLICE_MS;
    }

    pub fn disable(self: *RoundRobinScheduler) void {
        self.enabled = false;
    }

    pub fn tick(self: *RoundRobinScheduler) bool {
        if (!self.enabled) return false;

        if (self.time_slice_counter > 0) {
            self.time_slice_counter -= 1;
        }

        // 检查是否需要抢占
        if (self.time_slice_counter == 0) {
            self.stats.preemptions += 1;
            return true; // 需要重新调度
        }

        return false;
    }

    pub fn schedule(self: *RoundRobinScheduler, ready_queue: *ProcessQueue) ?ProcessId {
        if (!self.enabled) return null;

        self.stats.total_schedules += 1;

        if (ready_queue.dequeue()) |next_pid| {
            // 如果有当前进程且不是同一个进程，将其放回就绪队列
            if (self.current_process) |current_pid| {
                if (current_pid != next_pid) {
                    ready_queue.enqueue(current_pid) catch {};
                }
            }

            self.current_process = next_pid;
            self.time_slice_counter = TIME_SLICE_MS;
            return next_pid;
        } else {
            // 没有就绪进程，运行idle进程
            self.current_process = 1; // idle进程PID为1
            return 1;
        }
    }

    pub fn yieldCpu(self: *RoundRobinScheduler, ready_queue: *ProcessQueue) ?ProcessId {
        if (self.current_process) |current_pid| {
            ready_queue.enqueue(current_pid) catch {};
        }
        return self.schedule(ready_queue);
    }

    pub fn blockCurrent(self: *RoundRobinScheduler, blocked_queue: *ProcessQueue) void {
        if (self.current_process) |current_pid| {
            blocked_queue.enqueue(current_pid) catch {};
            self.current_process = null;
        }
    }

    pub fn wakeupProcess(self: *RoundRobinScheduler, pid: ProcessId, ready_queue: *ProcessQueue, blocked_queue: *ProcessQueue) void {
        _ = self;
        if (blocked_queue.remove(pid)) {
            ready_queue.enqueue(pid) catch {};
        }
    }
};

// 高性能原子操作
pub const AtomicOps = struct {
    pub fn compareAndSwap(comptime T: type, ptr: *T, expected: T, new: T) bool {
        return @cmpxchgWeak(T, ptr, expected, new, .SeqCst, .SeqCst) == null;
    }

    pub fn atomicAdd(comptime T: type, ptr: *T, value: T) T {
        return @atomicRmw(T, ptr, .Add, value, .SeqCst);
    }

    pub fn atomicSub(comptime T: type, ptr: *T, value: T) T {
        return @atomicRmw(T, ptr, .Sub, value, .SeqCst);
    }

    pub fn atomicLoad(comptime T: type, ptr: *const T) T {
        return @atomicLoad(T, ptr, .SeqCst);
    }

    pub fn atomicStore(comptime T: type, ptr: *T, value: T) void {
        @atomicStore(T, ptr, value, .SeqCst);
    }
};

// 架构相关函数
pub fn getTimestamp() u64 {
    var low: u32 = undefined;
    var high: u32 = undefined;
    asm volatile ("rdtsc"
        : [low] "={eax}" (low),
          [high] "={edx}" (high)
    );
    return (@as(u64, high) << 32) | low;
}

pub fn disableInterrupts() bool {
    var flags: u64 = undefined;
    asm volatile (
        \\pushfq
        \\pop %[flags]
        \\cli
        : [flags] "=r" (flags)
    );
    return (flags & 0x200) != 0;
}

pub fn restoreInterrupts(enabled: bool) void {
    if (enabled) {
        asm volatile ("sti");
    }
}

pub fn memoryBarrier() void {
    asm volatile ("mfence");
}

pub fn cpuPause() void {
    asm volatile ("pause");
}

pub fn getStackPointer() usize {
    var sp: usize = undefined;
    asm volatile ("mov %%rsp, %[sp]"
        : [sp] "=r" (sp)
    );
    return sp;
}

pub fn setStackPointer(sp: usize) void {
    asm volatile ("mov %[sp], %%rsp"
        :
        : [sp] "r" (sp)
    );
}

// 导出C接口函数
export fn zig_create_process_queue() ?*ProcessQueue {
    // 简化实现，返回null表示不支持
    return null;
}

export fn zig_queue_enqueue(queue: *ProcessQueue, pid: ProcessId) c_int {
    queue.enqueue(pid) catch return -1;
    return 0;
}

export fn zig_queue_dequeue(queue: *ProcessQueue) ProcessId {
    return queue.dequeue() orelse INVALID_PID;
}

export fn zig_queue_size(queue: *const ProcessQueue) usize {
    return queue.len();
}

export fn zig_queue_is_empty(queue: *const ProcessQueue) bool {
    return queue.isEmpty();
}

export fn zig_create_scheduler() ?*RoundRobinScheduler {
    // 简化实现，返回null表示不支持
    return null;
}

export fn zig_scheduler_enable(scheduler: *RoundRobinScheduler) void {
    scheduler.enable();
}

export fn zig_scheduler_disable(scheduler: *RoundRobinScheduler) void {
    scheduler.disable();
}

export fn zig_scheduler_tick(scheduler: *RoundRobinScheduler) bool {
    return scheduler.tick();
}

export fn zig_scheduler_schedule(scheduler: *RoundRobinScheduler, ready_queue: *ProcessQueue) ProcessId {
    return scheduler.schedule(ready_queue) orelse INVALID_PID;
}

export fn zig_get_timestamp() u64 {
    return getTimestamp();
}

export fn zig_disable_interrupts() bool {
    return disableInterrupts();
}

export fn zig_restore_interrupts(enabled: bool) void {
    restoreInterrupts(enabled);
}

export fn zig_memory_barrier() void {
    memoryBarrier();
}

export fn zig_cpu_pause() void {
    cpuPause();
}

// 高性能内存操作
export fn zig_fast_memcpy(dest: [*]u8, src: [*]const u8, len: usize) void {
    @memcpy(dest[0..len], src[0..len]);
}

export fn zig_fast_memset(dest: [*]u8, value: u8, len: usize) void {
    @memset(dest[0..len], value);
}

export fn zig_fast_memcmp(a: [*]const u8, b: [*]const u8, len: usize) c_int {
    const slice_a = a[0..len];
    const slice_b = b[0..len];
    
    for (slice_a, slice_b, 0..) |byte_a, byte_b, i| {
        if (byte_a < byte_b) return -1;
        if (byte_a > byte_b) return 1;
        _ = i;
    }
    return 0;
}
