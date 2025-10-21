// Boruix OS Shell - license命令实现
// 显示第三方项目列表和许可证信息

#include "license.h"
#include "kernel/shell.h"
#include "drivers/display.h"
#include "../../utils/string.h"

// 第三方项目信息结构
typedef struct {
    const char* name;
    const char* version;
    const char* license;
    const char* description;
    const char* url;
} third_party_project_t;

// 第三方项目列表
static const third_party_project_t third_party_projects[] = {
    {
        .name = "Limine Bootloader",
        .version = "v8.x-binary",
        .license = "BSD Zero Clause License",
        .description = "Modern bootloader supporting BIOS and UEFI",
        .url = "https://github.com/limine-bootloader/limine.git"
    },
    {
        .name = "Flanterm",
        .version = "2022-2025",
        .license = "BSD License",
        .description = "Fast terminal emulator with framebuffer backend",
        .url = "https://codeberg.org/Mintsuki/Flanterm.git"
    },
    {
        .name = "freestnd-c-hdrs-0bsd",
        .version = "2022-2024",
        .license = "0BSD License",
        .description = "Collection of 0BSD-licensed freestanding C headers for GCC and Clang",
        .url = "https://codeberg.org/OSDev/freestnd-c-hdrs-0bsd.git"
    },
    {NULL, NULL, NULL, NULL, NULL}  // 结束标记
};

// 显示第三方项目信息
static void show_third_party_projects(void) {
    print_string("THIRD-PARTY PROJECTS\n");
    print_string("===================\n\n");
    
    for (int i = 0; third_party_projects[i].name != NULL; i++) {
        print_string("Project: ");
        print_string(third_party_projects[i].name);
        print_string("\n");
        
        print_string("Version: ");
        print_string(third_party_projects[i].version);
        print_string("\n");
        
        print_string("License: ");
        print_string(third_party_projects[i].license);
        print_string("\n");
        
        print_string("Description: ");
        print_string(third_party_projects[i].description);
        print_string("\n");
        
        print_string("URL: ");
        print_string(third_party_projects[i].url);
        print_string("\n\n");
    }
}

// 显示本项目许可证信息
static void show_project_license(void) {
    print_string("BORUIX OS LICENSE\n");
    print_string("================\n\n");
    
    print_string("MIT License\n\n");
    print_string("Copyright (c) 2025 BRX-Boruix\n\n");
    print_string("Permission is hereby granted, free of charge, to any person obtaining a copy\n");
    print_string("of this software and associated documentation files (the \"Software\"), to deal\n");
    print_string("in the Software without restriction, including without limitation the rights\n");
    print_string("to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n");
    print_string("copies of the Software, and to permit persons to whom the Software is\n");
    print_string("furnished to do so, subject to the following conditions:\n\n");
    print_string("The above copyright notice and this permission notice shall be included in all\n");
    print_string("copies or substantial portions of the Software.\n\n");
    print_string("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n");
    print_string("IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n");
    print_string("FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n");
    print_string("AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n");
    print_string("LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n");
    print_string("OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n");
    print_string("SOFTWARE.\n\n");
}

// 显示Limine许可证信息
static void show_limine_license(void) {
    print_string("LIMINE BOOTLOADER LICENSE\n");
    print_string("=========================\n\n");
    
    print_string("Copyright (C) 2019-2025 mintsuki and contributors.\n\n");
    print_string("Redistribution and use in source and binary forms, with or without\n");
    print_string("modification, are permitted provided that the following conditions are met:\n\n");
    print_string("1. Redistributions of source code must retain the above copyright notice, this\n");
    print_string("   list of conditions and the following disclaimer.\n\n");
    print_string("2. Redistributions in binary form must reproduce the above copyright notice,\n");
    print_string("   this list of conditions and the following disclaimer in the documentation\n");
    print_string("   and/or other materials provided with the distribution.\n\n");
    print_string("THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\n");
    print_string("ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n");
    print_string("WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n");
    print_string("DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n");
    print_string("FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n");
    print_string("DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n");
    print_string("SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n");
    print_string("CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n");
    print_string("OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    print_string("OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\n");
}

// 显示Flanterm许可证信息
static void show_flanterm_license(void) {
    print_string("FLANTERM LICENSE\n");
    print_string("===============\n\n");
    
    print_string("Copyright (C) 2022-2025 mintsuki and contributors.\n\n");
    print_string("Redistribution and use in source and binary forms, with or without\n");
    print_string("modification, are permitted provided that the following conditions are met:\n\n");
    print_string("1. Redistributions of source code must retain the above copyright notice, this\n");
    print_string("   list of conditions and the following disclaimer.\n\n");
    print_string("2. Redistributions in binary form must reproduce the above copyright notice,\n");
    print_string("   this list of conditions and the following disclaimer in the documentation\n");
    print_string("   and/or other materials provided with the distribution.\n\n");
    print_string("THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\n");
    print_string("ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n");
    print_string("WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n");
    print_string("DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n");
    print_string("FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n");
    print_string("DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n");
    print_string("SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n");
    print_string("CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n");
    print_string("OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    print_string("OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\n");
}

// 显示freestnd-c-hdrs-0bsd许可证信息
static void show_freestnd_license(void) {
    print_string("FREESTND-C-HDRS-0BSD LICENSE\n");
    print_string("============================\n\n");
    
    print_string("Copyright (C) 2022-2024 mintsuki and contributors.\n\n");
    print_string("Permission to use, copy, modify, and/or distribute this software for any\n");
    print_string("purpose with or without fee is hereby granted.\n\n");
    print_string("THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH\n");
    print_string("REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND\n");
    print_string("FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,\n");
    print_string("INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM\n");
    print_string("LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR\n");
    print_string("OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\n");
    print_string("PERFORMANCE OF THIS SOFTWARE.\n\n");
}


// 显示帮助信息
static void show_help(void) {
    print_string("LICENSE COMMAND USAGE\n");
    print_string("====================\n\n");
    print_string("license [option]\n\n");
    print_string("Options:\n");
    print_string("  (no args)    - Show all license information\n");
    print_string("  projects     - Show third-party projects list\n");
    print_string("  project      - Show Boruix OS license\n");
    print_string("  limine       - Show Limine bootloader license\n");
    print_string("  flanterm     - Show Flanterm terminal license\n");
    print_string("  freestnd     - Show freestnd-c-hdrs-0bsd license\n");
    print_string("  help         - Show this help message\n\n");
}

void cmd_license(int argc, char* argv[]) {
    // 如果没有参数，显示boruix os所有许可证信息，并提示help
    if (argc == 1) {
        show_third_party_projects();
        print_string("========================================\n\n");
        show_project_license();
        print_string("========================================\n\n");
        print_string("Type 'license help' for usage information.\n");
        print_string("and 'license projects' to show third-party projects list.\n");
        return;
    }
    
    // 检查第一个参数
    const char* option = SAFE_ARGV_STR(argc, argv, 1, "");
    
    if (shell_strcmp(option, "projects") == 0) {
        show_third_party_projects();
    } else if (shell_strcmp(option, "project") == 0) {
        show_project_license();
    } else if (shell_strcmp(option, "limine") == 0) {
        show_limine_license();
    } else if (shell_strcmp(option, "flanterm") == 0) {
        show_flanterm_license();
    } else if (shell_strcmp(option, "freestnd") == 0) {
        show_freestnd_license();
    } else if (shell_strcmp(option, "help") == 0) {
        show_help();
    } else {
        print_string("Unknown option: ");
        print_string(option);
        print_string("\n");
        print_string("Type 'license help' for usage information.\n");
    }
}
