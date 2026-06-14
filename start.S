/*
 * start.S — RISC-V 32I 启动文件（无 Flash，纯 RAM 运行）
 *
 * 启动流程:
 *   上电/复位 → _start → 设置 gp / 清零 bss / 设栈指针
 *                      → rtthread_startup() → 调度器接管，永不返回
 */

#include "cpuport.h"

    .section .text.init, "ax"
    .align  2
    .globl  _start
_start:
    /* ====== 1. 设置全局指针 gp ====== */
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop

    /* ====== 2. 清零 .bss 段 ====== */
    la      t0, _bss_start
    la      t1, _bss_end
    bgeu    t0, t1, bss_done

bss_loop:
    STORE   zero, 0(t0)
    addi    t0, t0, REGBYTES
    bltu    t0, t1, bss_loop

bss_done:

    /* ====== 3. 设置栈指针 ====== */
    la      sp, __stack_top

    /* ====== 4. 跳转到 RT-Thread 内核初始化 ====== */
    /*
     * rtthread_startup() 完成:
     *   - rt_hw_board_init()   硬件初始化（中断控制器、堆、UART）
     *   - rt_system_timer_init / rt_system_scheduler_init
     *   - rt_application_init  创建 main 线程
     *   - rt_system_scheduler_start  启动调度器
     *     → rt_hw_context_switch_to → mret 进入第一个线程
     *
     * 此后 _start 的使命结束，调度器接管一切。
     */
    call    rtthread_startup

    /* 如果 rtthread_startup 意外返回，在此死循环 */
die:
    j       die

    .size   _start, . - _start
