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
    
    /* 设置全局指针 gp  */
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop

    la      sp, __stack_top

    /*  清零 .bss 段 */
    la      t0, _bss_start
    la      t1, _bss_end
    bgeu    t0, t1, bss_done

bss_loop:
    STORE   zero, 0(t0)
    addi    t0, t0, REGBYTES
    bltu    t0, t1, bss_loop

bss_done:
    /*  跳转到 RT-Thread 内核初始化 */
    call    rtthread_startup

    /* 如果 rtthread_startup 意外返回，在此死循环 */
die:
    j       die

    .size   _start, . - _start
