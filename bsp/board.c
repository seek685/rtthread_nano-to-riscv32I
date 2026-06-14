/*
 * bsp/board.c — 无 Flash RISC-V 32I 板级支持
 *
 * 职责:
 *   1. rt_hw_board_init() — 硬件初始化入口
 *   2. 桩函数 — 为 rthw.h 声明的可选函数提供默认实现
 */

#include <rtthread.h>
#include <rthw.h>
#include "board.h"

/* 链接脚本 (link.ld) 中定义的符号 */
extern unsigned char __heap_start;
extern unsigned char __heap_end;

/* ==================================================================
 *  板级初始化
 * ================================================================== */

/**
 * 硬件初始化 — 由 rtthread_startup() 调用
 *
 * 执行顺序:
 *   1. 初始化中断控制器（mtvec、CLINT、PLIC）
 *   2. 初始化系统堆（rt_malloc 可用）
 *   3. 初始化 UART（如开启 RT_USING_CONSOLE）
 */
void rt_hw_board_init(void)
{
    /* ---- 1. 中断控制器 ---- */
    rt_hw_interrupt_init();

    /* ---- 2. 系统堆 ---- */
    rt_system_heap_init(HEAP_BEGIN, HEAP_END);

#ifdef RT_USING_CONSOLE
    /* ---- 3. UART ---- */
    rt_hw_uart_init();
#endif

#ifdef RT_DEBUG
    rt_kprintf("RISC-V 32I RT-Thread Nano 启动。\n");
    rt_kprintf("CPU: %lu Hz  堆: %lu 字节\n",
               (unsigned long)CPU_FREQ,
               (unsigned long)(HEAP_END - HEAP_BEGIN + 1));
#endif
}

/* ==================================================================
 *  CPU 复位
 * ================================================================== */

/**
 * 软件复位 CPU
 * 关中断 → 跳转到 _start 复位向量
 */
RT_WEAK void rt_hw_cpu_reset(void)
{
    extern void _start(void);

    rt_hw_interrupt_disable();
    _start();
}

/* ==================================================================
 *  Cache 控制桩函数（无 Cache 的核心使用以下空实现）
 * ================================================================== */

RT_WEAK void rt_hw_cpu_icache_enable(void)  {}
RT_WEAK void rt_hw_cpu_icache_disable(void) {}
RT_WEAK rt_base_t rt_hw_cpu_icache_status(void) { return 0; }
RT_WEAK void rt_hw_cpu_icache_ops(int ops, void *addr, int size) { (void)ops; (void)addr; (void)size; }

RT_WEAK void rt_hw_cpu_dcache_enable(void)  {}
RT_WEAK void rt_hw_cpu_dcache_disable(void) {}
RT_WEAK rt_base_t rt_hw_cpu_dcache_status(void) { return 0; }
RT_WEAK void rt_hw_cpu_dcache_ops(int ops, void *addr, int size) { (void)ops; (void)addr; (void)size; }

/* ==================================================================
 *  控制台输出
 * ================================================================== */

#if defined(RT_USING_CONSOLE) && (UART_BASE_ADDR != 0)

/**
 * 向 NS16550A 兼容 UART 输出字符串（轮询模式）
 * 如使用其他 UART 型号，请在 BSP 层覆盖此函数
 */
void rt_hw_console_output(const char *str)
{
    while (*str)
    {
        /* 等待发送保持寄存器为空 (LSR bit 5) */
        while ((*(volatile unsigned int *)(UART_BASE_ADDR + 0x14) & 0x20) == 0) {}

        *(volatile unsigned int *)(UART_BASE_ADDR) = (unsigned int)(*str);

        /* \n → \r\n */
        if (*str == '\n')
        {
            while ((*(volatile unsigned int *)(UART_BASE_ADDR + 0x14) & 0x20) == 0) {}
            *(volatile unsigned int *)(UART_BASE_ADDR) = '\r';
        }
        str++;
    }
}

#else

RT_WEAK void rt_hw_console_output(const char *str)
{
    (void)str;
}

#endif /* RT_USING_CONSOLE */

/* ==================================================================
 *  微秒级延时（忙等待）
 * ================================================================== */

RT_WEAK void rt_hw_us_delay(rt_uint32_t us)
{
    unsigned long cycles = (unsigned long)us * (CPU_FREQ / 1000000UL);
    while (cycles--)
    {
        __asm__ volatile("nop");
    }
}

/* ==================================================================
 *  调试辅助
 * ================================================================== */

RT_WEAK void rt_hw_show_memory(rt_uint32_t addr, rt_uint32_t size)
{
    (void)addr; (void)size;
    rt_kprintf("rt_hw_show_memory: 未实现\n");
}

RT_WEAK void rt_hw_backtrace(rt_uint32_t *fp, rt_uint32_t thread_entry)
{
    (void)fp; (void)thread_entry;
    rt_kprintf("rt_hw_backtrace: 未实现\n");
}
