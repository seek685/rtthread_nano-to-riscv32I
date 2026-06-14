/*
 * 版权所有 (c) 2006-2026，RT-Thread 开发团队
 *
 * SPDX 许可证标识符: Apache-2.0
 *
 * 修改日志:
 * 日期           作者          说明
 * 2026-06-10     RISC-V32I     无 Flash RISC-V 32I 平台 BSP 头文件
 *
 * ⚠️ 请根据你的实际硬件平台修改下面的基地址和频率！
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============== 时钟配置 ============== */

/** CPU 主频 (Hz) —— 根据你的平台修改 */
#define CPU_FREQ                    50000000UL  /* 50 MHz */

/** 外设总线频率 (Hz) —— 通常与 CPU_FREQ 相同 */
#define BUS_FREQ                    CPU_FREQ

/* ============== CLINT (核心本地中断控制器) ============== */

/**
 * CLINT 基地址
 * 提供: mtime (64 位定时器计数器) 和 mtimecmp (64 位定时器比较)
 *
 * 各平台典型地址（请根据你的平台修改）:
 *   - SiFive FE310:  0x02000000
 *   - QEMU virt:     0x02000000 (mtime), 0x02004000 (mtimecmp)
 *   - NEORV32:       0xFFFFFF60 (mtime), 0xFFFFFF68 (mtimecmp)
 */
#define CLINT_BASE_ADDR             0x02000000UL

/** mtime 寄存器偏移（RV32 上为低 32 位） */
#define CLINT_MTIME_OFFSET          0x0000BFF8UL

/** mtimecmp 寄存器偏移 */
#define CLINT_MTIMECMP_OFFSET       0x00004000UL

/* ============== PLIC (平台级中断控制器) ============== */

/**
 * PLIC 基地址
 *
 * 各平台典型地址:
 *   - SiFive FE310:  0x0C000000
 *   - QEMU virt:     0x0C000000
 */
#define PLIC_BASE_ADDR              0x0C000000UL

/* ============== UART (NS16550A / SiFive 兼容) ============== */

/**
 * 控制台 UART 基地址
 *
 * 各平台典型地址:
 *   - SiFive FE310:  0x10013000
 *   - QEMU virt:     0x10000000
 *   - NEORV32:       0xFFFFFFA0
 *
 * 如无 UART 可用，设为 0
 */
#define UART_BASE_ADDR              0x10000000UL

/* ============== RAM 与 堆 ============== */

/** 以下符号由链接脚本 (link.ld) 定义 */
extern unsigned char __heap_start;
extern unsigned char __heap_end;
extern unsigned char __stack_top;
extern unsigned char __stack_start;

/** 堆的起止地址（传递给 rt_system_heap_init） */
#define HEAP_BEGIN  ((void *)&__heap_start)
#define HEAP_END    ((void *)&__heap_end)

/* ============== 中断向量定义 ============== */

/**
 * PLIC 外部中断源最大数量
 * 根据你的 PLIC 配置修改
 */
#define MAX_IRQ_SOURCES             32

/**
 * RISC-V 机器模式中断源编号
 */
#define IRQ_S_SOFT                  1   /* 机器软件中断              */
#define IRQ_M_TIMER                 7   /* 机器定时器中断            */
#define IRQ_M_EXT                   11  /* 机器外部中断 (PLIC)       */

/** PLIC 外部中断源编号 —— 根据你的硬件定义 */
#define PLIC_IRQ_UART0              10  /* 示例: UART0 中断线       */

/* ============== 函数声明 ============== */

void rt_hw_board_init(void);
void rt_hw_trap_handler(void *regs, unsigned long mcause, unsigned long mtval);

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H__ */
