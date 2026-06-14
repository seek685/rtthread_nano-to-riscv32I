/*
 * context_gcc.S - RISC-V 上下文切换与中断操作
 * 配合无 Flash 纯 RAM 启动文件 start.S 和 trap_entry.S
 *
 * 帧格式:
 *  0*REGBYTES : mepc
 *  1*REGBYTES : ra
 *  2*REGBYTES : mstatus (完整)
 *  3*REGBYTES : gp
 *  4*REGBYTES : tp
 *  ...         (x5~x31)
 * 31*REGBYTES : t6
 */

#include "cpuport.h"

#ifdef RT_USING_SMP
#define rt_hw_interrupt_disable rt_hw_local_irq_disable
#define rt_hw_interrupt_enable  rt_hw_local_irq_enable
#endif

    .globl rt_hw_interrupt_disable
rt_hw_interrupt_disable:
    csrrci a0, mstatus, 8          /* 清 MIE，返回原值 */
    ret

    .globl rt_hw_interrupt_enable
rt_hw_interrupt_enable:
    csrw mstatus, a0
    ret

/* 首次调度，sp 直接从目标线程栈恢复 */
    .globl rt_hw_context_switch_to
rt_hw_context_switch_to:
    LOAD sp, (a0)                  /* sp = to_thread->sp */
#ifdef RT_USING_SMP
    mv   a0, a1
    call rt_cpus_lock_status_restore
#endif
    j    rt_hw_context_switch_exit

/* 线程到线程切换 */
    .globl rt_hw_context_switch
rt_hw_context_switch:
    /* 在栈上开辟帧 */
    addi  sp, sp, -32 * REGBYTES
    STORE sp, (a0)                 /* 保存当前 sp 到 from_thread->sp */

    /* 保存 mepc（用 ra 冒充，因为函数返回后继续执行点就是 ra */
    STORE ra, 0 * REGBYTES(sp)
    STORE ra, 1 * REGBYTES(sp)     /* 冗余但无害 */

    /* 构造并保存 mstatus */
    csrr   t0, mstatus
    /* 保留 MPP 和 MPIE 相关位，设置 MPP=Machine, MPIE=MIE */
    li     t1, 0x00001800          /* MPP=3 */
    or     t0, t0, t1             /* 置 MPP */
    andi   t2, t0, 8              /* 提取 MIE */
    slli   t2, t2, 4              /* MIE → MPIE 位 (bit 7 = bit 3 << 4) */
    li     t1, ~0x80
    and    t0, t0, t1             /* 清 MPIE */
    or     t0, t0, t2             /* 写入 MPIE */
    STORE  t0, 2 * REGBYTES(sp)

    /* 保存所有 caller-saved + callee-saved 寄存器 */
    STORE x3,   3 * REGBYTES(sp)   /* gp */
    STORE x4,   4 * REGBYTES(sp)
    STORE x5,   5 * REGBYTES(sp)
    STORE x6,   6 * REGBYTES(sp)
    STORE x7,   7 * REGBYTES(sp)
    STORE x8,   8 * REGBYTES(sp)
    STORE x9,   9 * REGBYTES(sp)
    STORE x10, 10 * REGBYTES(sp)
    STORE x11, 11 * REGBYTES(sp)
    STORE x12, 12 * REGBYTES(sp)
    STORE x13, 13 * REGBYTES(sp)
    STORE x14, 14 * REGBYTES(sp)
    STORE x15, 15 * REGBYTES(sp)
    STORE x16, 16 * REGBYTES(sp)
    STORE x17, 17 * REGBYTES(sp)
    STORE x18, 18 * REGBYTES(sp)
    STORE x19, 19 * REGBYTES(sp)
    STORE x20, 20 * REGBYTES(sp)
    STORE x21, 21 * REGBYTES(sp)
    STORE x22, 22 * REGBYTES(sp)
    STORE x23, 23 * REGBYTES(sp)
    STORE x24, 24 * REGBYTES(sp)
    STORE x25, 25 * REGBYTES(sp)
    STORE x26, 26 * REGBYTES(sp)
    STORE x27, 27 * REGBYTES(sp)
    STORE x28, 28 * REGBYTES(sp)
    STORE x29, 29 * REGBYTES(sp)
    STORE x30, 30 * REGBYTES(sp)
    STORE x31, 31 * REGBYTES(sp)

    /* 切换到目标线程 */
    LOAD sp, (a1)
#ifdef RT_USING_SMP
    mv   a0, a2
    call rt_cpus_lock_status_restore
#endif
    j    rt_hw_context_switch_exit

#ifdef RT_USING_SMP
    .globl rt_hw_context_switch_interrupt
rt_hw_context_switch_interrupt:
    STORE a0, 0(a1)
    LOAD  sp, 0(a2)
    mv    a0, a3
    call  rt_cpus_lock_status_restore
    j     rt_hw_context_switch_exit
#endif

/*
 * 公共恢复出口 —— 所有上下文切换和中断/异常最终汇聚于此
 * sp → 线程的 32 字帧
 */
.global rt_hw_context_switch_exit
rt_hw_context_switch_exit:
    /* 恢复 mepc */
    LOAD a0, 0 * REGBYTES(sp)
    csrw mepc, a0

    /* 恢复 ra */
    LOAD ra, 1 * REGBYTES(sp)

    /* 恢复完整的 mstatus（已包含 MPP 和 MPIE 的正确设置） */
    LOAD a0, 2 * REGBYTES(sp)
    csrw mstatus, a0

    /* 恢复 x3~x31 */
    LOAD x3,   3 * REGBYTES(sp)
    LOAD x4,   4 * REGBYTES(sp)
    LOAD x5,   5 * REGBYTES(sp)
    LOAD x6,   6 * REGBYTES(sp)
    LOAD x7,   7 * REGBYTES(sp)
    LOAD x8,   8 * REGBYTES(sp)
    LOAD x9,   9 * REGBYTES(sp)
    LOAD x10, 10 * REGBYTES(sp)
    LOAD x11, 11 * REGBYTES(sp)
    LOAD x12, 12 * REGBYTES(sp)
    LOAD x13, 13 * REGBYTES(sp)
    LOAD x14, 14 * REGBYTES(sp)
    LOAD x15, 15 * REGBYTES(sp)
    LOAD x16, 16 * REGBYTES(sp)
    LOAD x17, 17 * REGBYTES(sp)
    LOAD x18, 18 * REGBYTES(sp)
    LOAD x19, 19 * REGBYTES(sp)
    LOAD x20, 20 * REGBYTES(sp)
    LOAD x21, 21 * REGBYTES(sp)
    LOAD x22, 22 * REGBYTES(sp)
    LOAD x23, 23 * REGBYTES(sp)
    LOAD x24, 24 * REGBYTES(sp)
    LOAD x25, 25 * REGBYTES(sp)
    LOAD x26, 26 * REGBYTES(sp)
    LOAD x27, 27 * REGBYTES(sp)
    LOAD x28, 28 * REGBYTES(sp)
    LOAD x29, 29 * REGBYTES(sp)
    LOAD x30, 30 * REGBYTES(sp)
    LOAD x31, 31 * REGBYTES(sp)

    /* 释放栈帧 */
    addi sp, sp, 32 * REGBYTES

    mret