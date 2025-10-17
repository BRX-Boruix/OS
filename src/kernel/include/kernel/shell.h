#ifndef BORUIX_SHELL_H
#define BORUIX_SHELL_H

#include "kernel/types.h"

// Shell配置
#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define SHELL_PROMPT "boruix> "

// Shell函数声明
void shell_main(void);
void shell_init(void);
void shell_print_prompt(void);

#endif // BORUIX_SHELL_H
