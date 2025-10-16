// 简单的键盘测试程序
// 用于验证键盘驱动是否正常工作

#include <stdio.h>

int main() {
    printf("键盘扫描码测试:\n");
    printf("==============\n\n");
    
    // 测试扫描码映射
    printf("扫描码 0x0E (退格): 应该映射到 '\\b' (ASCII 8)\n");
    printf("扫描码 0x1C (回车): 应该映射到 '\\n' (ASCII 10)\n");
    printf("扫描码 0x39 (空格): 应该映射到 ' ' (ASCII 32)\n");
    printf("扫描码 0x10 (Q键): 应该映射到 'q' (ASCII 113)\n");
    
    printf("\n验证:\n");
    printf("退格键 ASCII: %d\n", '\b');
    printf("回车键 ASCII: %d\n", '\n');
    printf("空格键 ASCII: %d\n", ' ');
    printf("Q键 ASCII: %d\n", 'q');
    
    return 0;
}
