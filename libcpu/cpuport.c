/*
 * 版权所有 (c) 2006-2018，RT-Thread 开发团队
 *
 * SPDX 许可证标识符: Apache-2.0
 *
 * 修改日志:
 * 日期           作者          说明
 * 2018/10/28     Bernard      统一 RISC-V 移植代码
 * 2026/06/10     RISC-V32I    中文注释
 */

#include <rthw.h>
#include <rtthread.h>

#include "cpuport.h"

#ifndef RT_USING_SMP
/* 非 SMP 版本的中断上下文切换变量 */
volatile rt_ubase_t  rt_interrupt_from_thread = 0;   /* 被切换走的线程 sp 字段地址  */
volatile rt_ubase_t  rt_interrupt_to_thread   = 0;   /* 被切换到的线程 sp 字段地址  */
volatile rt_uint32_t rt_thread_switch_interrupt_flag = 0; /* 中断上下文切换标志     */
#endif

/*
 * 硬件栈帧结构（与 context_gcc.S 中保存的 32 条目帧布局一致）
 *
 *   sp[0]  : epc      — 程序计数器（异常返回地址 / 线程入口）
 *   sp[1]  : ra (x1)  — 返回地址
 *   sp[2]  : mstatus  — 机器状态寄存器
 *   sp[3]  : gp (x3)  — 全局指针
 *   sp[4]  : tp (x4)  — 线程指针
 *   sp[5]  : t0 (x5)  — 临时寄存器 0
 *   sp[6]  : t1 (x6)  — 临时寄存器 1
 *   sp[7]  : t2 (x7)  — 临时寄存器 2
 *   sp[8]  : s0/fp(x8)— 被调用者保存寄存器 0 / 帧指针
 *   sp[9]  : s1 (x9)  — 被调用者保存寄存器 1
 *   sp[10] : a0 (x10) — 返回值 / 函数参数 0
 *   sp[11] : a1 (x11) — 返回值 / 函数参数 1
 *   sp[12] : a2 (x12) — 函数参数 2
 *   sp[13] : a3 (x13) — 函数参数 3
 *   sp[14] : a4 (x14) — 函数参数 4
 *   sp[15] : a5 (x15) — 函数参数 5
 *   sp[16] : a6 (x16) — 函数参数 6
 *   sp[17] : a7 (x17) — 函数参数 7
 *   sp[18] : s2 (x18) — 被调用者保存寄存器 2
 *   sp[19] : s3 (x19) — 被调用者保存寄存器 3
 *   sp[20] : s4 (x20) — 被调用者保存寄存器 4
 *   sp[21] : s5 (x21) — 被调用者保存寄存器 5
 *   sp[22] : s6 (x22) — 被调用者保存寄存器 6
 *   sp[23] : s7 (x23) — 被调用者保存寄存器 7
 *   sp[24] : s8 (x24) — 被调用者保存寄存器 8
 *   sp[25] : s9 (x25) — 被调用者保存寄存器 9
 *   sp[26] : s10(x26) — 被调用者保存寄存器 10
 *   sp[27] : s11(x27) — 被调用者保存寄存器 11
 *   sp[28] : t3 (x28) — 临时寄存器 3
 *   sp[29] : t4 (x29) — 临时寄存器 4
 *   sp[30] : t5 (x30) — 临时寄存器 5
 *   sp[31] : t6 (x31) — 临时寄存器 6
 */
struct rt_hw_stack_frame
{
    rt_ubase_t epc;        /* epc — 程序计数器                          */
    rt_ubase_t ra;         /* x1  — ra    — 跳转返回地址                 */
    rt_ubase_t mstatus;    /*      — 机器状态寄存器                      */
    rt_ubase_t gp;         /* x3  — gp    — 全局指针                     */
    rt_ubase_t tp;         /* x4  — tp    — 线程指针                     */
    rt_ubase_t t0;         /* x5  — t0    — 临时寄存器 0                 */
    rt_ubase_t t1;         /* x6  — t1    — 临时寄存器 1                 */
    rt_ubase_t t2;         /* x7  — t2    — 临时寄存器 2                 */
    rt_ubase_t s0_fp;      /* x8  — s0/fp — 被调用者保存寄存器 0 / 帧指针 */
    rt_ubase_t s1;         /* x9  — s1    — 被调用者保存寄存器 1          */
    rt_ubase_t a0;         /* x10 — a0    — 返回值 / 函数参数 0           */
    rt_ubase_t a1;         /* x11 — a1    — 返回值 / 函数参数 1           */
    rt_ubase_t a2;         /* x12 — a2    — 函数参数 2                    */
    rt_ubase_t a3;         /* x13 — a3    — 函数参数 3                    */
    rt_ubase_t a4;         /* x14 — a4    — 函数参数 4                    */
    rt_ubase_t a5;         /* x15 — a5    — 函数参数 5                    */
    rt_ubase_t a6;         /* x16 — a6    — 函数参数 6                    */
    rt_ubase_t a7;         /* x17 — a7    — 函数参数 7                    */
    rt_ubase_t s2;         /* x18 — s2    — 被调用者保存寄存器 2          */
    rt_ubase_t s3;         /* x19 — s3    — 被调用者保存寄存器 3          */
    rt_ubase_t s4;         /* x20 — s4    — 被调用者保存寄存器 4          */
    rt_ubase_t s5;         /* x21 — s5    — 被调用者保存寄存器 5          */
    rt_ubase_t s6;         /* x22 — s6    — 被调用者保存寄存器 6          */
    rt_ubase_t s7;         /* x23 — s7    — 被调用者保存寄存器 7          */
    rt_ubase_t s8;         /* x24 — s8    — 被调用者保存寄存器 8          */
    rt_ubase_t s9;         /* x25 — s9    — 被调用者保存寄存器 9          */
    rt_ubase_t s10;        /* x26 — s10   — 被调用者保存寄存器 10         */
    rt_ubase_t s11;        /* x27 — s11   — 被调用者保存寄存器 11         */
    rt_ubase_t t3;         /* x28 — t3    — 临时寄存器 3                  */
    rt_ubase_t t4;         /* x29 — t4    — 临时寄存器 4                  */
    rt_ubase_t t5;         /* x30 — t5    — 临时寄存器 5                  */
    rt_ubase_t t6;         /* x31 — t6    — 临时寄存器 6                  */
};

/**
 * 初始化线程栈
 *
 * 在线程栈顶构建一个 32 条目的硬件栈帧，使新线程被调度时
 * 能通过 rt_hw_context_switch_exit 正确"返回"到入口函数。
 *
 * @param tentry     线程入口函数
 * @param parameter  传递给入口函数的参数
 * @param stack_addr 线程栈起始地址（低地址）
 * @param texit      线程退出函数（入口函数返回时自动调用）
 *
 * @return 初始化后的栈指针（指向已构建的帧）
 */
rt_uint8_t *rt_hw_stack_init(void       *tentry,
                             void       *parameter,
                             rt_uint8_t *stack_addr,
                             void       *texit)
{
    struct rt_hw_stack_frame *frame;
    rt_uint8_t         *stk;
    int                i;

    /* 栈向下增长，将帧放在栈顶区域 */
    stk  = stack_addr + sizeof(rt_ubase_t);
    stk  = (rt_uint8_t *)RT_ALIGN_DOWN((rt_ubase_t)stk, REGBYTES);
    stk -= sizeof(struct rt_hw_stack_frame);

    frame = (struct rt_hw_stack_frame *)stk;

    /* 用魔数填充未初始化的寄存器槽位，便于调试 */
    for (i = 0; i < sizeof(struct rt_hw_stack_frame) / sizeof(rt_ubase_t); i++)
    {
        ((rt_ubase_t *)frame)[i] = 0xdeadbeef;
    }

    /* 设置关键寄存器:
     *   ra  = texit      → 入口函数返回时自动调用退出函数
     *   a0  = parameter  → 传递给入口函数的参数
     *   epc = tentry     → mret 后的第一条指令 = 入口函数
     */
    frame->ra      = (rt_ubase_t)texit;
    frame->a0      = (rt_ubase_t)parameter;
    frame->epc     = (rt_ubase_t)tentry;

    /* 强制设置为机器模式 (MPP=11)，并使能中断 (MPIE=1)
     * 0x00007880 = MPP=3 | MPIE=1 | 保留位(0x7800) */
    frame->mstatus = 0x00007880;

    return stk;
}

/*
 * 中断上下文切换（仅设置标志，实际切换由 trap 出口完成）
 *
 * #ifdef RT_USING_SMP
 * void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from,
 *                                      rt_ubase_t to, struct rt_thread *to_thread);
 * #else
 * void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to);
 * #endif
 */
#ifndef RT_USING_SMP
void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to)
{
    /* 首次设置 from（保留最先被中断的线程）*/
    if (rt_thread_switch_interrupt_flag == 0)
        rt_interrupt_from_thread = from;

    /* 更新 to（可能被多次调度，最后一次的目标线程生效）*/
    rt_interrupt_to_thread = to;
    rt_thread_switch_interrupt_flag = 1;

    return ;
}
#endif /* RT_USING_SMP */

/**
 * 关闭 CPU（致命错误处理）
 *
 * 打印关闭信息后死循环，等待外部复位。
 */
void rt_hw_cpu_shutdown()
{
    rt_uint32_t level;
    rt_kprintf("关机...\n");

    level = rt_hw_interrupt_disable();
    while (level)
    {
        RT_ASSERT(0);
    }
}
