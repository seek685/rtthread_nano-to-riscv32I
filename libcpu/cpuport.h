/*
 * 版权所有 (c) 2006-2018，RT-Thread 开发团队
 *
 * SPDX 许可证标识符: Apache-2.0
 *
 * 修改日志:
 * 日期           作者          说明
 * 2018-10-03     Bernard      初始版本
 * 2026-06-10     RISC-V32I     增加 __STACKSIZE__ 默认定义
 */

#ifndef CPUPORT_H__
#define CPUPORT_H__

#include <rtconfig.h>

/* 按寄存器位宽选择加载/存储指令 */
#ifdef ARCH_CPU_64BIT
#define STORE                   sd      /* 64 位存储 */
#define LOAD                    ld      /* 64 位加载 */
#define REGBYTES                8       /* 寄存器宽度 8 字节 */
#else
#define STORE                   sw      /* 32 位存储 */
#define LOAD                    lw      /* 32 位加载 */
#define REGBYTES                4       /* 寄存器宽度 4 字节 */
#endif

/* 每个 hart 的中断栈大小（用于上下文切换和信号处理） */
#ifndef __STACKSIZE__
#define __STACKSIZE__           2048
#endif

#endif /* CPUPORT_H__ */
