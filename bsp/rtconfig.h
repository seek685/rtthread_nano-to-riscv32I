/*
 * bsp/rtconfig.h — 板级内核配置
 *
 * 此文件控制 RT-Thread 内核的功能裁剪。
 * 属于 BSP 层 —— 每块板子有自己的 rtconfig.h。
 */

#ifndef __RTCONFIG_H__
#define __RTCONFIG_H__

/* ============== RT-Thread 内核配置 ============== */

#define RT_NAME_MAX                    8   /* 内核对象名称最大长度          */
#define RT_ALIGN_SIZE                  4   /* 对齐大小（字节）              */
#define RT_THREAD_PRIORITY_MAX         32  /* 最大线程优先级                */
#define RT_TICK_PER_SECOND             100 /* 每秒系统节拍数                */

/* ============== 内存管理 ============== */

#define RT_USING_HEAP                     /* 使能动态堆内存 (rt_malloc)     */
#define RT_USING_SMALL_MEM                 /* 小内存分配算法                 */
/* #define RT_USING_SLAB */                /* slab 内存分配算法              */

/* ============== IPC 功能（按需开启以减小体积） ============== */

#define RT_USING_SEMAPHORE                 /* 信号量 —— 内存管理需要 */
/* #define RT_USING_MUTEX */
/* #define RT_USING_EVENT */
/* #define RT_USING_MAILBOX */
/* #define RT_USING_MESSAGEQUEUE */

/* ============== 设备与控制台 ============== */

#define RT_USING_DEVICE                     /* 设备框架 —— finsh 需要 */
#define RT_USING_CONSOLE                    /* 控制台输出 */
#define RT_CONSOLEBUF_SIZE           256     /* 控制台缓冲区大小 */

/* ============== FinSH 命令行 ============== */

#define RT_USING_FINSH                      /* 开启 FinSH 命令行 Shell */
/* #define FINSH_USING_SYMTAB */            /* 符号表支持     */
/* #define FINSH_USING_MSH */               /* MSH 模块 Shell */
/* #define FINSH_USING_HISTORY */           /* 命令历史       */
/* #define FINSH_USING_DESCRIPTION */       /* 命令描述信息   */

/* ============== 组件初始化 ============== */

#define RT_USING_COMPONENTS_INIT          /* 自动初始化机制                 */
#define RT_USING_USER_MAIN                /* 使用用户 main() 作为入口       */

#define RT_MAIN_THREAD_STACK_SIZE  2048   /* main 线程栈大小                */
#define RT_MAIN_THREAD_PRIORITY    10     /* main 线程优先级                */

/* ============== 钩子与调试 ============== */

/* #define RT_USING_HOOK */
#define RT_DEBUG

/* ============== 架构: RISC-V 32 位 ============== */

#define ARCH_RISCV32                      /* 32 位 RISC-V，机器模式         */
/* #undef  ARCH_CPU_64BIT */              /* 未定义 = 32 位                 */
#define __STACKSIZE__             2048    /* 每个 hart 的中断栈大小         */

#endif /* __RTCONFIG_H__ */
