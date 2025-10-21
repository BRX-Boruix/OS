// Boruix OS Shell - great命令实现
// 验证yang borui参数并打印英文难崩话语

#include "great.h"
#include "kernel/shell.h"
#include "drivers/display.h"
#include "drivers/cmos.h"
#include "../../utils/string.h"

// 大小写不敏感的字符串比较函数
static int shell_strcasecmp(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        char c1 = *str1;
        char c2 = *str2;
        
        // 转换为小写进行比较
        if (c1 >= 'A' && c1 <= 'Z') c1 = c1 - 'A' + 'a';
        if (c2 >= 'A' && c2 <= 'Z') c2 = c2 - 'A' + 'a';
        
        if (c1 != c2) return c1 - c2;
        
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

// 难崩
static const char* crazy_quotes[] = {
    "Try Minecraft",
    "I know it's difficult, but this is an order!!!!",
    "Two slices of bread with cheese in between.",
    "So (), Fuck you!",
    "Hakimi O Nan Bei Lv Dou",
    "Back! Back! Back!",
    "GLC Good Die!",
    "Be far ahead",
    "LONELY LONELY~",
    "Only Shit",
    "EpieiKeia216 Best",
    "What does it mean?",
    "No swinging on the swing in the operating system.",
    "33550336",
    NULL  // 结束标记
};

// 获取随机数（简单的线性同余生成器）
static unsigned int simple_rand(unsigned int* seed) {
    *seed = *seed * 1103515245 + 12345;
    return *seed;
}

void cmd_great(int argc, char* argv[]) {
    // 使用安全的参数检查
    if (!ARGC_CHECK(argc, 2)) {
        // 神秘提示，若参数不足，则给出谜题式英文提示
        print_string("To unlock the words of wisdom, provide the name in two pieces: surname and given name.\n");
        return;
    }
    
    // 使用安全的参数访问
    const char* first_arg = SAFE_ARGV_STR(argc, argv, 1, "");
    if (shell_strcasecmp(first_arg, "help") == 0) {
        print_string("Try yang borui \n");
        return;
    }

    if (!ARGC_CHECK(argc, 3)) {
        // 神秘提示，若参数不足，则给出谜题式英文提示
        print_string("To unlock the words of wisdom, provide the name in two pieces: surname and given name.\n");
        return;
    }

    // 验证第一个参数是否为 "yang"（大小写不敏感）
    if (shell_strcasecmp(first_arg, "yang") != 0) {
        print_string("What's his surname? \n");
        return;
    }
    
    // 验证第二个参数是否为 "borui"（大小写不敏感）
    const char* second_arg = SAFE_ARGV_STR(argc, argv, 2, "");
    if (shell_strcasecmp(second_arg, "borui") != 0) {
        print_string(" What's his given name?\n");
        return;
    }
    
    // 计算难崩话语的数量
    int quote_count = 0;
    while (crazy_quotes[quote_count] != NULL) {
        quote_count++;
    }
    
    // 生成随机索引（使用CMOS主板时间作为种子）
    unsigned char hour, minute, second;
    get_current_time(&hour, &minute, &second);
    
    // 使用当前时间组合作为种子
    unsigned int seed = (hour * 3600) + (minute * 60) + second;
    unsigned int rand_val = simple_rand(&seed);
    int random_index = rand_val % quote_count;
    
    // 打印格式化的输出
    print_string("=======Yang Borui=======\n");
    print_string(crazy_quotes[random_index]);
    print_char('\n');
}
