// Boruix OS Shell - clear命令实现
// 清屏功能

#include "clear.h"
#include "drivers/display.h"

void cmd_clear(int argc, char* argv[]) {
    (void)argc; (void)argv;
    clear_screen();
    // 清理历史缓冲区
    terminal_history_init();
}
