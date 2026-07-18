/*
 * core_portme.c — CoreMark 平台实现文件
 *
 * 提供平台相关的函数实现。
 */

#include "coremark.h"
#include "core_portme.h"

#if VALIDATION_RUN
volatile ee_s32 seed1_volatile = 0x3415;
volatile ee_s32 seed2_volatile = 0x3415;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PERFORMANCE_RUN
volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PROFILE_RUN
volatile ee_s32 seed1_volatile = 0x8;
volatile ee_s32 seed2_volatile = 0x8;
volatile ee_s32 seed3_volatile = 0x8;
#endif
volatile ee_s32 seed4_volatile = ITERATIONS;
volatile ee_s32 seed5_volatile = 0;

/* ================================================================
 *  计时函数移植
 *
 *  如何捕获时间并转换为秒必须根据平台支持进行移植。
 *  例如: 读取板载 RTC、读取 CPU 时钟周期性能计数器等。
 *  示例实现使用了标准 time.h 和 windows.h 定义。
 * ================================================================ */

CORETIMETYPE
barebones_clock()
{
    return __read_mtime64();    // 返回 64 位 mtime 值（CLINT MMIO）
    /* 你必须实现一个测量时间的方法!
     * 此函数应返回当前时间。 */
}

/* TIMER_RES_DIVIDER — 用于在计时器分辨率和可测量的总时间之间做权衡的分频器。
 *
 *   使用较低的值可以提高分辨率，但需确保不会发生溢出。
 *   如果返回值存在溢出问题，请增大此值。 */
//#define GETMYTIME(_t)              (*_t = barebones_clock())
//#define MYTIMEDIFF(fin, ini)       ((fin) - (ini))
//#define TIMER_RES_DIVIDER          1
//#define SAMPLE_TIME_IMPLEMENTATION 1


/* 定义平台相关的全局时间变量 */
static CORETIMETYPE start_time_val, stop_time_val;

/* start_time — 在基准测试计时部分开始前调用。
 *
 *   实现可以是捕获系统计时器 (如示例代码所示)
 *   或者清零某些系统参数——例如将 CPU 时钟周期计数置零。 */
void
start_time(void)
{
    GETMYTIME(&start_time_val);
}

/* stop_time — 在基准测试计时部分结束后调用。
 *
 *   实现可以是捕获系统计时器 (如示例代码所示)
 *   或其他系统参数——例如读取 CPU 周期计数器的当前值。 */
void
stop_time(void)
{
    GETMYTIME(&stop_time_val);
}

/* get_time — 返回一个抽象的"ticks"数，表示系统上的时间。
 *
 *   实际返回值可以是 CPU 周期、毫秒或任何其他值，
 *   只要它能通过 <time_in_secs> 转换为秒即可。
 *   这种方法是为了适应任何硬件或模拟平台。
 *   示例实现默认返回毫秒，分辨率由 <TIMER_RES_DIVIDER> 控制。 */
CORE_TICKS
get_time(void)
{
    CORE_TICKS elapsed
        = (CORE_TICKS)(MYTIMEDIFF(stop_time_val, start_time_val));
    return elapsed;
}

/* time_in_secs — 将 get_time 返回的值转换为秒。
 *
 *   <secs_ret> 类型用于适应不支持浮点的系统。
 *   默认实现使用上面定义的 EE_TICKS_PER_SEC 宏。 */
secs_ret
time_in_secs(CORE_TICKS ticks)
{
    secs_ret retval = ((secs_ret)ticks) / (secs_ret)EE_TICKS_PER_SEC;
    return retval;
}

ee_u32 default_num_contexts = 1;

/* portable_init — 平台相关的初始化代码
 *   测试一些常见错误。 */
void
portable_init(core_portable *p, int *argc, char *argv[])
{

    //"请在 portable_init 中调用板级初始化例程 (如果需要)，特别是初始化 UART!\n"

    (void)argc; // 防止未使用警告
    (void)argv; // 防止未使用警告

    if (sizeof(ee_ptr_int) != sizeof(ee_u8 *))
    {
        ee_printf(
            "错误! 请将 ee_ptr_int 定义为能容纳指针的类型!\n");
    }
    if (sizeof(ee_u32) != 4)
    {
        ee_printf("错误! 请将 ee_u32 定义为 32 位无符号类型!\n");
    }
    p->portable_id = 1;
}

/* portable_fini — 平台相关的清理代码 */
void
portable_fini(core_portable *p)
{
    p->portable_id = 0;
}
