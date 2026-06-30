/*
 * bsp/uart_irq.h — NS16550A UART 中断驱动接收
 *
 * 职责: ring buffer + 信号量 + ISR，替代 board.c 中的轮询 getchar
 */

#ifndef __UART_IRQ_H__
#define __UART_IRQ_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============== NS16550A 寄存器偏移（补充 IER） ============== */

#define UART_RBR        0x00  /* 接收缓冲寄存器（读）          */
#define UART_THR        0x00  /* 发送保持寄存器（写）          */
#define UART_IER        0x01  /* 中断使能寄存器                */
#define UART_LSR        0x05  /* 线路状态寄存器                */

/* IER 寄存器位 */
#define IER_RX_INT      0x01  /* bit0: 接收数据就绪中断        */

/* LSR 寄存器位 */
#define LSR_RX_READY    0x01  /* bit0: 接收数据就绪            */
#define LSR_TX_EMPTY    0x20  /* bit5: 发送保持寄存器为空       */

/* ============== Ring Buffer（你自己定义大小） ============== */

#define UART_RX_BUF_SIZE  64    /* 环形缓冲区大小               */

/* ============== 环形缓冲区 ============== */

struct uart_rx_buf
{
    rt_uint8_t   buf[UART_RX_BUF_SIZE];
    volatile int write_idx;   /* ISR 写入端 */
    int          read_idx;    /* 线程读取端 */
};

extern struct uart_rx_buf  g_uart_rx;
extern struct rt_semaphore g_uart_rx_sem;

/* ============== 函数声明 ============== */

/* 初始化 UART 中断接收（注册 ISR + 配置 UART IER + 使能 PLIC） */
void uart_irq_init(void);

/* 替换 board.c 中的弱符号版本 —— 阻塞等待数据 */
char rt_hw_console_getchar(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_IRQ_H__ */
