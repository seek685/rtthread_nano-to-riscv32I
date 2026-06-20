/*
 * 版权所有 (c) 2006-2026，RT-Thread 开发团队
 *
 * SPDX 许可证标识符: Apache-2.0
 *
 * 修改日志:
 * 日期           作者          说明
 * 2026-06-10     RISC-V32I     RISC-V 32I 中断控制器驱动
 */

#include <rtthread.h>
#include <rthw.h>
#include "board.h"
#include "cpuport.h"
#include "riscv-ops.h"
#include "riscv-plic.h"

/* ============== 中断描述符表 ============== */

/** 中断服务例程描述符数组，PLIC 外部中断按源编号索引 */
static struct rt_irq_desc irq_desc_table[MAX_IRQ_SOURCES];

/* ============== 系统节拍配置 ============== */

/** 每个系统节拍对应的 CPU 时钟周期数 */
#define TICK_CYCLES     (CPU_FREQ / RT_TICK_PER_SECOND)

/* ============== 前向声明 ============== */

static void rt_hw_timer_isr(int vector, void *param);

/* ============== 中断控制器初始化 ============== */

/**
 * 初始化中断控制器（CLINT + PLIC）
 *
 * 在系统启动阶段由 rt_hw_board_init() 调用。
 * 完成以下工作:
 *   1. 清空中断描述符表
 *   2. 初始化 PLIC（禁用所有源，设置优先级阈值）
 *   3. 设置机器定时器首次触发时间
 *   4. 使能 mie 中的定时器和外部中断
 *   5. 设置 mtvec 指向 trap 入口
 */
void rt_hw_interrupt_init(void)
{
    int i;

    /* ---- 初始化中断描述符表 ---- */
    for (i = 0; i < MAX_IRQ_SOURCES; i++)
    {
        irq_desc_table[i].handler = RT_NULL;
        irq_desc_table[i].param   = RT_NULL;
    }

    /* ---- 配置 PLIC ---- */

    /* 初始禁用所有 PLIC 中断源 */
    for (i = 1; i < MAX_IRQ_SOURCES; i++)
    {
        __plic_irq_disable(i);
        __plic_set_priority(i, 0);
    }

    /* PLIC 优先级阈值设为 0（接受所有优先级的中断）*/
    __plic_set_threshold(0);

    /* ---- 配置机器定时器 ---- */

    /*
     * 设置 mtimecmp 以产生首次定时中断。
     *
     * RV32 上 mtime 是 64 位计数器，拆分为两个 32 位寄存器:
     *   低 32 位: CLINT_MTIME_OFFSET
     *   高 32 位: CLINT_MTIME_OFFSET + 4
     *
     * 写入 mtimecmp 的安全顺序（RV32）:
     *   1. 先写 mtimecmp 低 32 位为 ~0（防止写入高半时意外触发）
     *   2. 写入 mtimecmp 高 32 位
     *   3. 写入 mtimecmp 低 32 位目标值
     */
    {
        volatile unsigned long *mtime_lo =
            (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIME_OFFSET);
        volatile unsigned long *mtime_hi =
            (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIME_OFFSET + 4);
        volatile unsigned long *mtimecmp_lo =
            (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIMECMP_OFFSET);
        volatile unsigned long *mtimecmp_hi =
            (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIMECMP_OFFSET + 4);

        unsigned long lo = *mtime_lo;
        unsigned long hi = *mtime_hi;
        unsigned long next_lo = lo + TICK_CYCLES;

        /* 低 32 位溢出时进位到高 32 位 */
        if (next_lo < lo)
            hi++;

        /* 安全写入: 先设低半为 ~0 防止误触发，再写高半，最后写低半 */
        *mtimecmp_lo = ~0UL;
        *mtimecmp_hi = hi;
        *mtimecmp_lo = next_lo;
    }

    /* ---- 使能中断 ---- */

    /* 使能机器定时器中断 (mie.MTIE = bit 7) */
    set_csr(mie, 0x80);

    /* 使能机器外部中断 (mie.MEIE = bit 11) */
    set_csr(mie, 0x800);

    /* 使能机器软件中断 (mie.MSIE = bit 3) —— Nano 中未使用，按需开启 */
    /* set_csr(mie, 0x8); */

    /* 设置 trap 向量指向汇编入口 trap_entry（定义在 trap_entry.S） */
    extern void trap_entry(void);
    write_csr(mtvec, trap_entry);
}

/* ============== 中断屏蔽/解除 ============== */

/**
 * 屏蔽（禁用）指定中断源
 *
 * @param vector  中断向量号
 *                >= 0 且 < MAX_IRQ_SOURCES: PLIC 外部中断
 *                IRQ_M_TIMER: 机器定时器中断
 *                IRQ_M_EXT:   机器外部中断（禁用整个 PLIC）
 */
void rt_hw_interrupt_mask(int vector)
{
    if (vector >= 0 && vector < MAX_IRQ_SOURCES)
    {
        /* PLIC 外部中断源 */
        __plic_irq_disable(vector);
    }
    else if (vector == IRQ_M_TIMER)
    {
        /* 机器定时器中断 */
        clear_csr(mie, 0x80);
    }
    else if (vector == IRQ_M_EXT)
    {
        /* 机器外部中断（禁用全部 PLIC）*/
        clear_csr(mie, 0x800);
    }
}

/**
 * 解除屏蔽（启用）指定中断源
 *
 * @param vector  中断向量号（同 rt_hw_interrupt_mask）
 */
void rt_hw_interrupt_umask(int vector)
{
    if (vector >= 0 && vector < MAX_IRQ_SOURCES)
    {
        /* PLIC 外部中断源 —— 先设置优先级再使能 */
        __plic_set_priority(vector, 1);
        __plic_irq_enable(vector);
    }
    else if (vector == IRQ_M_TIMER)
    {
        set_csr(mie, 0x80);
    }
    else if (vector == IRQ_M_EXT)
    {
        set_csr(mie, 0x800);
    }
}

/* ============== 中断服务例程安装 ============== */

/**
 * 安装中断服务例程
 *
 * @param vector  中断向量号
 * @param handler ISR 函数指针
 * @param param   传递给 ISR 的参数
 * @param name    ISR 名称（调试用，需启用 RT_USING_INTERRUPT_INFO）
 *
 * @return 之前安装的 handler（无则返回 RT_NULL）
 */
rt_isr_handler_t rt_hw_interrupt_install(int              vector,
                                          rt_isr_handler_t handler,
                                          void            *param,
                                          const char      *name)
{
    rt_isr_handler_t old_handler;

    if (vector < 0 || vector >= MAX_IRQ_SOURCES)
    {
        return RT_NULL;
    }

    old_handler = irq_desc_table[vector].handler;

    irq_desc_table[vector].handler = handler;
    irq_desc_table[vector].param   = param;

#ifdef RT_USING_INTERRUPT_INFO
    if (name != RT_NULL)
    {
        rt_strncpy(irq_desc_table[vector].name, name, RT_NAME_MAX);
        irq_desc_table[vector].counter = 0;
    }
#else
    (void)name;
#endif

    return old_handler;
}

/* ============== 定时器中断服务例程 ============== */

/**
 * 机器定时器中断处理
 *
 * 由主 trap 分发函数在 mcause == IRQ_M_TIMER 时调用。
 * 设置下一次定时中断并通知内核一个 tick 已过去。
 */
static void rt_hw_timer_isr(int vector, void *param)
{
    volatile unsigned long *mtime_lo =
        (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIME_OFFSET);
    volatile unsigned long *mtime_hi =
        (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIME_OFFSET + 4);
    volatile unsigned long *mtimecmp_lo =
        (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIMECMP_OFFSET);
    volatile unsigned long *mtimecmp_hi =
        (volatile unsigned long *)(CLINT_BASE_ADDR + CLINT_MTIMECMP_OFFSET + 4);

    (void)vector;
    (void)param;

    /* ---- 设置下一次定时器比较值 ---- */
    unsigned long lo = *mtime_lo;
    unsigned long hi = *mtime_hi;
    unsigned long next_lo = lo + TICK_CYCLES;

    /* 低 32 位溢出时进位到高 32 位 */
    if (next_lo < lo)
        hi++;

    /* 安全写入: 先设低半为 ~0 防止误触发，再写高半，最后写低半 */
    *mtimecmp_lo = ~0UL;
    *mtimecmp_hi = hi;
    *mtimecmp_lo = next_lo;

    /* ---- 通知 RT-Thread 内核一个 tick 已过去 ---- */
    rt_tick_increase();
}

/* ============== 外部中断分发 ============== */

/**
 * 分发 PLIC 外部中断
 *
 * 由主 trap 分发函数在 mcause == IRQ_M_EXT 时调用。
 * 从 PLIC 读取中断源编号，查表调用已安装的 ISR。
 */
static void rt_hw_plic_dispatch(void)
{
    unsigned int source;

    /* 从 PLIC 认领（读取并清除）当前中断 */
    source = __plic_irq_claim();

    if (source == 0)
    {
        /* 虚假中断 —— 无需处理 */
        return;
    }

    if (source < MAX_IRQ_SOURCES)
    {
        struct rt_irq_desc *desc = &irq_desc_table[source];

#ifdef RT_USING_INTERRUPT_INFO
        desc->counter++;
#endif

        if (desc->handler != RT_NULL)
        {
            desc->handler((int)source, desc->param);
        }
    }

    /* 在 PLIC 中完成中断处理（允许再次触发）*/
    __plic_irq_complete(source);
}

/* ============== 主 Trap 分发函数（C 语言部分） ============== */

/**
 * C 语言级的主 trap 处理函数，由 trap_entry.S 调用
 *
 * @param regs    指向栈上寄存器保存帧的指针
 * @param mcause  机器 trap 原因寄存器（最高位=1 为中断，=0 为异常）
 * @param mtval   机器 trap 值寄存器（异常附加信息，如访存地址）
 */
void rt_hw_trap_handler(void *regs, unsigned long mcause, unsigned long mtval)
{
    unsigned long cause = mcause;

    /* 判断是中断（最高位=1）还是异常（最高位=0）*/
    if (cause & 0x80000000UL)
    {
        /* ---- 中断处理 ---- */
        cause &= 0x7FFFFFFFUL;  /* 提取异常码（去掉最高位）*/

        /* 进入中断上下文（嵌套计数 +1）*/
        rt_interrupt_enter();

        switch (cause)
        {
        case IRQ_M_TIMER:   /* 7: 机器定时器中断 */
            rt_hw_timer_isr(cause, RT_NULL);
            break;

        case IRQ_M_EXT:     /* 11: 机器外部中断（PLIC）*/
            rt_hw_plic_dispatch();
            break;

        case IRQ_S_SOFT:    /* 3: 机器软件中断（Nano 未使用）*/
            /* 清除软件中断挂起位 */
            clear_csr(mip, 0x8);
            break;

        default:
            /* 未处理的中断 —— 打印并忽略 */
            rt_kprintf("未处理的中断: mcause=%lu\n", mcause);
            break;
        }

        /*
         * 离开中断上下文。
         * 如果 ISR 中触发了上下文切换（设置了 rt_thread_switch_interrupt_flag），
         * 汇编级 trap 出口会完成实际的线程切换。
         */
        rt_interrupt_leave();
    }
    else
    {
        /* ---- 异常处理 ---- */
        /* 对于精简系统，任何异常都是致命的。打印诊断信息后停机。 */
        rt_kprintf("\n===== 致命异常 =====\n");
        rt_kprintf("mcause:  0x%08lx (%lu)\n", mcause, cause);
        rt_kprintf("mepc:    0x%08lx\n", read_csr(mepc));
        rt_kprintf("mtval:   0x%08lx\n", mtval);

#ifdef RT_DEBUG
        /* 调试模式下打印寄存器快照 */
        {
            unsigned long *frame = (unsigned long *)regs;
            rt_kprintf("--- 寄存器转储 ---\n");
            rt_kprintf("ra (x1):  0x%08lx\n", frame[1]);
            rt_kprintf("gp (x3):  0x%08lx\n", frame[3]);
            rt_kprintf("tp (x4):  0x%08lx\n", frame[4]);
            rt_kprintf("a0 (x10): 0x%08lx\n", frame[10]);
            rt_kprintf("a1 (x11): 0x%08lx\n", frame[11]);
            rt_kprintf("sp:       0x%08lx\n", (unsigned long)(frame + 32));
        }
#endif

        /* 停机 */
        rt_hw_cpu_shutdown();
    }
}
