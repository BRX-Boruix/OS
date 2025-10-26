// Boruix OS Shell - 内置命令统一头文件
// 包含所有内置命令的声明

#ifndef BUILTIN_H
#define BUILTIN_H

// 包含所有内置命令的头文件
#include "help/help.h"
#include "clear/clear.h"
#include "echo/echo.h"
#include "time/time.h"
#include "info/info.h"
#include "reboot/reboot.h"
#include "test/test.h"
#include "uptime/uptime.h"
#include "irqstat/irqstat.h"
#include "irqinfo/irqinfo.h"
#include "irqprio/irqprio.h"
#include "irqtest/irqtest.h"
#include "great/great.h"
#include "crash/crash.h"
#include "license/license.h"

#ifdef ENABLE_TEST_COMMANDS
#include "vmtest/vmtest.h"
#endif

#endif // BUILTIN_H
