/*
 * trap_entry.S — RISC-V 机器模式异常/中断入口
 *
 * 硬件在 trap 发生时自动:
 *   mcause  ← trap 原因编码（最高位=1 中断，=0 异常）
 *   mepc    ← 被中断指令的地址
 *   mtval   ← 附加异常信息（如访存地址）
 *   mstatus.MIE → mstatus.MPIE  然后  mstatus.MIE ← 0（关全局中断）
 *   mstatus.MPP ← 进入 trap 前的特权模式
 *   PC      ← mtvec 指向的地址（即本入口）
 *
 * 寄存器保存帧布局（32 条目，与 context_gcc.S 一致）:
 *   0*REGBYTES  : mepc
 *   1*REGBYTES  : ra  (x1)
 *   2*REGBYTES  : mstatus（完整值，包含 MPP/MPIE）
 *   3*REGBYTES  : gp  (x3)
 *   4*REGBYTES  : tp  (x4)
 *   5*REGBYTES  : t0  (x5)
 *   ...
 *  31*REGBYTES  : t6  (x31)
 */

#include "cpuport.h"

    .section .text.trap, "ax"
    .align  2

    .globl  trap_entry
trap_entry:
    /* ====== 第 1 步: 在栈上分配 32 条目帧 ====== */
    addi    sp, sp, -32 * REGBYTES

    /*
     * ====== 第 2 步: 先保存全部通用寄存器到帧中 ======
     * STORE 指令只读源寄存器，不修改它。
     * 在读取任何 CSR 之前完成此步，确保 t0/t1/t2 的原始值不受破坏。
     */

    /* 槽位 0: 暂存 0，后面用 mepc 覆盖 */
    STORE   zero, 0 * REGBYTES(sp)

    /* 槽位 1: ra (x1) */
    STORE   ra,   1 * REGBYTES(sp)

    /* 槽位 2: 暂存 0，后面用 mstatus 覆盖 */
    STORE   zero, 2 * REGBYTES(sp)

    /* 槽位 3: gp (x3) — 与新版 context_gcc.S 保持一致，恢复时会恢复它 */
    STORE   gp,   3 * REGBYTES(sp)

    /* 槽位 4~31: x4~x31 —— 全部原始值，此时 t0/t1/t2 尚未被破坏 */
    STORE   tp,   4 * REGBYTES(sp)       /* x4  (tp)           */
    STORE   t0,   5 * REGBYTES(sp)       /* x5  (t0) ← 原始值  */
    STORE   t1,   6 * REGBYTES(sp)       /* x6  (t1) ← 原始值  */
    STORE   t2,   7 * REGBYTES(sp)       /* x7  (t2) ← 原始值  */
    STORE   s0,   8 * REGBYTES(sp)       /* x8  (s0/fp)        */
    STORE   s1,   9 * REGBYTES(sp)       /* x9  (s1)           */
    STORE   a0,  10 * REGBYTES(sp)       /* x10 (a0)           */
    STORE   a1,  11 * REGBYTES(sp)       /* x11 (a1)           */
    STORE   a2,  12 * REGBYTES(sp)       /* x12 (a2)           */
    STORE   a3,  13 * REGBYTES(sp)       /* x13 (a3)           */
    STORE   a4,  14 * REGBYTES(sp)       /* x14 (a4)           */
    STORE   a5,  15 * REGBYTES(sp)       /* x15 (a5)           */
    STORE   a6,  16 * REGBYTES(sp)       /* x16 (a6)           */
    STORE   a7,  17 * REGBYTES(sp)       /* x17 (a7)           */
    STORE   s2,  18 * REGBYTES(sp)       /* x18 (s2)           */
    STORE   s3,  19 * REGBYTES(sp)       /* x19 (s3)           */
    STORE   s4,  20 * REGBYTES(sp)       /* x20 (s4)           */
    STORE   s5,  21 * REGBYTES(sp)       /* x21 (s5)           */
    STORE   s6,  22 * REGBYTES(sp)       /* x22 (s6)           */
    STORE   s7,  23 * REGBYTES(sp)       /* x23 (s7)           */
    STORE   s8,  24 * REGBYTES(sp)       /* x24 (s8)           */
    STORE   s9,  25 * REGBYTES(sp)       /* x25 (s9)           */
    STORE   s10, 26 * REGBYTES(sp)       /* x26 (s10)          */
    STORE   s11, 27 * REGBYTES(sp)       /* x27 (s11)          */
    STORE   t3,  28 * REGBYTES(sp)       /* x28 (t3)           */
    STORE   t4,  29 * REGBYTES(sp)       /* x29 (t4)           */
    STORE   t5,  30 * REGBYTES(sp)       /* x30 (t5)           */
    STORE   t6,  31 * REGBYTES(sp)       /* x31 (t6)           */

    /*
     * ====== 第 3 步: 读取 CSR 并回填帧中的 CSR 槽位 ======
     * 此时全部通用寄存器已安全保存，可以随意使用 t0/t1/t2。
     *
     * 槽位 0 ← mepc（异常返回地址）
     */
    csrr    t0, mepc
    STORE   t0, 0 * REGBYTES(sp)

    /*
     * 槽位 2 ← mstatus
     *
     * 硬件在 trap 时已将 mstatus 设置为:
     *   MPP  = 进入 trap 前的特权模式（机器模式运行则为 3）
     *   MPIE = 进入 trap 前的 MIE（中断使能状态）
     *   MIE  = 0（trap 期间关中断）
     *
     * 为确保 mret 时正确返回机器模式，强制 MPP=3:
     */
    csrr    t0, mstatus
    li      t1, 0x00001800              /* MPP = 3 (机器模式) */
    or      t0, t0, t1
    STORE   t0, 2 * REGBYTES(sp)

    /* ====== 第 4 步: 调用 C 语言 trap 分发函数 ====== */
    /* a0 = 栈帧指针（C 函数可通过它读写任何寄存器） */
    mv      a0, sp

    /* a1 = mcause（trap 原因编码） */
    csrr    a1, mcause

    /* a2 = mtval（附加信息，如非法地址） */
    csrr    a2, mtval

    call    rt_hw_trap_handler

    /* ====== 第 5 步: 检查 ISR 是否触发了上下文切换 ====== */
    la      t0, rt_thread_switch_interrupt_flag
    LOAD    t1, 0(t0)
    beqz    t1, 1f                      /* 无切换 → 直接恢复 */

    /* ---- 上下文切换: 保存当前栈帧，加载目标线程栈帧 ---- */
    STORE   zero, 0(t0)                 /* 清除切换标志 */

    /* 保存当前 sp 到 from_thread->sp */
    la      t0, rt_interrupt_from_thread
    LOAD    t1, 0(t0)                   /* t1 = &from_thread->sp */
    STORE   sp, 0(t1)                   /* from_thread->sp = 当前帧指针 */

    /* 加载 to_thread->sp */
    la      t0, rt_interrupt_to_thread
    LOAD    t1, 0(t0)                   /* t1 = &to_thread->sp */
    LOAD    sp, 0(t1)                   /* sp = 目标线程的帧指针 */

    /*
     * 此时 sp 指向:
     *   - 被中断线程的帧（无切换）
     *   - 另一个线程的帧（切换发生）
     *   - 新线程的初始帧（rt_hw_stack_init 构建）
     *
     * 跳转到 context_gcc.S 中的公共恢复路径:
     * rt_hw_context_switch_exit 从 sp 指向的 32 条目帧中
     * 恢复全部寄存器，然后执行 mret。
     */
1:
    j       rt_hw_context_switch_exit

    .size   trap_entry, . - trap_entry
