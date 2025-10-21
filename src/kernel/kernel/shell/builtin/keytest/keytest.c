// Boruix OS 键盘测试命令
// 用于测试键盘输入和扫描码

#include "keytest.h"
#include "drivers/display.h"
#include "drivers/keyboard.h"
#include "../../utils/string.h"

// 键盘测试命令
void cmd_keytest(int argc, char* argv[]) {
    // 检查是否有重置参数
    if (argc > 1 && shell_strcmp(argv[1], "reset") == 0) {
        print_string("Resetting keyboard...\n");
        keyboard_reset();
        print_string("Keyboard reset complete.\n");
        return;
    }
    
    print_string("Keyboard Test Mode\n");
    print_string("Press any key to see its scancode. Press Ctrl+C to exit.\n");
    print_string("Use 'keytest reset' to reset keyboard if it's not working.\n");
    print_string("Testing Page Up/Page Down keys...\n");
    
    // 启用键盘调试模式
    int key_count = 0;
    
    while (key_count < 20) {  // 测试20个按键
        if (keyboard_has_char()) {
            char c = keyboard_get_char();
            print_string("Key received: 0x");
            print_hex(c);
            print_string(" (");
            if (c >= 32 && c <= 126) {
                print_char(c);
            } else {
                print_string("non-printable");
            }
            print_string(")\n");
            key_count++;
        }
        
        // 简单的延迟
        for (volatile int i = 0; i < 100000; i++);
    }
    
    print_string("Keyboard test completed.\n");
}
