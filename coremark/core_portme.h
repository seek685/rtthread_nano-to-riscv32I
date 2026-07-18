/*
 * core_portme.h — CoreMark 平台配置头文件
 *
 * 此文件包含在不同平台上运行 CoreMark 所需的配置常量。
 */

#ifndef CORE_PORTME_H
#define CORE_PORTME_H

/* ================================================================
 *  数据类型和设置
 * ================================================================ */

/* HAS_FLOAT — 如果平台支持浮点，设为 1 */
#ifndef HAS_FLOAT
#define HAS_FLOAT 0
#endif

/* HAS_TIME_H — 如果平台有 <time.h> 头文件及其函数实现，设为 1 */
#ifndef HAS_TIME_H
#define HAS_TIME_H 0
#endif

/* USE_CLOCK — 如果平台有 <time.h> 且实现了 clock() 函数，设为 1 */
#ifndef USE_CLOCK
#define USE_CLOCK 0
#endif

/* HAS_STDIO — 如果平台有 <stdio.h>，设为 1 */
#ifndef HAS_STDIO
#define HAS_STDIO 0
#endif

/* HAS_PRINTF — 如果平台有 <stdio.h> 且实现了 printf，设为 1 */
#ifndef HAS_PRINTF
#define HAS_PRINTF 0
#endif

/* COMPILER_VERSION / COMPILER_FLAGS / MEM_LOCATION
 *   根据平台初始化这些字符串 */
#ifndef COMPILER_VERSION
#ifdef __GNUC__
#define COMPILER_VERSION "GCC"__VERSION__
#else
#define COMPILER_VERSION   "GCC 15.2.0"   /** "请在此处填写编译器版本 (例如 gcc 4.1)"**/
#endif
#endif

#ifndef COMPILER_FLAGS
#define COMPILER_FLAGS \
    "-O2 -march=rv32imac_zicsr -mabi=ilp32"/* "请在此处填写编译器标志 (例如 -O3)" */
#endif

#ifndef MEM_LOCATION
#define MEM_LOCATION "STACK"
#endif

#include <stddef.h>
#include "board.h"
#include "riscv-ops.h"

/* 数据类型:
 *   为避免编译器问题，在此定义 8 位、16 位和 32 位数据类型。
 *
 *   重要:
 *   ee_ptr_int 必须是能容纳指针的数据类型，否则 CoreMark 可能失败！
 */
typedef signed short   ee_s16;
typedef unsigned short ee_u16;
typedef signed int     ee_s32;
typedef double         ee_f32;
typedef unsigned char  ee_u8;
typedef unsigned int   ee_u32;
typedef ee_u32         ee_ptr_int;
typedef size_t         ee_size_t;
#define NULL ((void *)0)

/* align_mem — 将偏移对齐到 32 位值。
 *   在 Matrix 算法中用于初始化输入内存块。 */
#define align_mem(x) (void *)(4 + (((ee_ptr_int)(x)-1) & ~3))

/* CORE_TICKS — 定义计时函数返回值的类型 */
typedef unsigned long long   CORETIMETYPE;
typedef CORETIMETYPE         CORE_TICKS;


/* 读取 CLINT 的 mtime（MMIO，非 CSR）。
 * RV32 上拆成高低两个 32 位寄存器，用 hi-lo-hi 三遍读防回卷。 */
static inline unsigned long long __read_mtime64(void) {
    volatile unsigned long *mtime_lo =
        (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIME_OFFSET);
    volatile unsigned long *mtime_hi =
        (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIME_OFFSET + 4);
    unsigned long hi1, lo, hi2;
    do {
        hi1 = *mtime_hi;
        lo  = *mtime_lo;
        hi2 = *mtime_hi;
    } while (hi1 != hi2);
    return ((unsigned long long)hi1 << 32) | lo;
}

#define GETMYTIME(_t)        (*_t = __read_mtime64())
#define MYTIMEDIFF(fin, ini) ((fin) - (ini))
#define EE_TICKS_PER_SEC     MTIME_FREQ       /* mtime 实际频率，不是 CPU 频率 */
#define TIMER_RES_DIVIDER    1
/* SEED_METHOD — 定义获取种子的方法 (种子无法在编译期计算)
 *
 *   有效值:
 *     SEED_ARG      — 从命令行获取
 *     SEED_FUNC     — 从系统函数获取
 *     SEED_VOLATILE — 从 volatile 变量获取
 */
#ifndef SEED_METHOD
#define SEED_METHOD SEED_VOLATILE
#endif

/* MEM_METHOD — 定义获取内存块的方法
 *
 *   有效值:
 *     MEM_MALLOC — 适用于实现了 malloc 且有 <malloc.h> 的平台
 *     MEM_STATIC — 使用静态内存数组
 *     MEM_STACK  — 在栈上分配数据块 (尚未实现)
 */
#ifndef MEM_METHOD
#define MEM_METHOD MEM_STACK
#endif

/* MULTITHREAD — 定义并行执行
 *
 *   有效值:
 *     1   — 仅一个上下文 (默认)
 *     N>1 — 将并行执行 N 个副本
 *
 *   注意:
 *     如果此标志被定义为大于 1 的值，则必须定义启动并行上下文的实现。
 *     提供了两个示例实现，使用 <USE_PTHREAD> 或 <USE_FORK> 来启用。
 *     也可以在 <core_portme.c> 中实现不同的 <core_start_parallel>
 *     和 <core_end_parallel> 来适配特定架构。
 */
#ifndef MULTITHREAD
#define MULTITHREAD 1
#define USE_PTHREAD 0
#define USE_FORK    0
#define USE_SOCKET  0
#endif

/* MAIN_HAS_NOARGC — 平台不支持向 main 传递参数时需要此宏
 *
 *   有效值:
 *     0 — 支持向 main 传递 argc/argv
 *     1 — 不支持向 main 传递 argc/argv
 *
 *   注意:
 *     此标志仅在 MULTITHREAD 被定义为大于 1 的值时才有意义。
 */
#define main coremark_main

#ifndef MAIN_HAS_NOARGC
#define MAIN_HAS_NOARGC 1
#endif

/* MAIN_HAS_NORETURN — 平台不支持从 main 返回值时需要此宏
 *
 *   有效值:
 *     0 — main 返回 int，返回值为 0
 *     1 — 平台不支持从 main 返回值
 */
#ifndef MAIN_HAS_NORETURN
#define MAIN_HAS_NORETURN 0
#endif

/* default_num_contexts — 此简单移植不使用，值必须为 1 */
extern ee_u32 default_num_contexts;

typedef struct CORE_PORTABLE_S
{
    ee_u8 portable_id;
} core_portable;

/* 平台相关的初始化 / 清理函数 */
void portable_init(core_portable *p, int *argc, char *argv[]);
void portable_fini(core_portable *p);

/* ===== 运行模式 ===== */
#ifndef ITERATIONS
#define ITERATIONS         0       /** 0 表示由 core_main.c 自动校准到约 10 秒 **/
#endif

#if !defined(PROFILE_RUN) && !defined(PERFORMANCE_RUN) \
    && !defined(VALIDATION_RUN)
#if (TOTAL_DATA_SIZE == 1200)
#define PROFILE_RUN 1
#elif (TOTAL_DATA_SIZE == 2000)
#define PERFORMANCE_RUN 1
#else
#define VALIDATION_RUN 1
#endif
#endif

int ee_printf(const char *fmt, ...);

#endif /* CORE_PORTME_H */
