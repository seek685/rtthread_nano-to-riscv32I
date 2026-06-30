/*
 * bsp/uart_irq.c — NS16550A UART 中断驱动接收
 *
 * 思路:
 *   1. UART 收到一个字节 → PLIC 中断 10 触发
 *   2. ISR 从 RBR 读走字节 → 放入 ring buffer → 释放信号量
 *   3. finsh 线程被信号量唤醒 → 从 ring buffer 取出 → 返回给 shell
 *
 * 涉及的寄存器（基地址 UART_BASE_ADDR = 0x10000000，定义在 board.h）:
 *   - IER (偏移 0x01): 写 0x01 使能 RX 中断
 *   - RBR (偏移 0x00): 读 = 收到的字节
 *   - LSR (偏移 0x05): bit0 = 有数据, bit5 = 可发送
 *
 * PLIC 相关:
 *   - 中断号: PLIC_IRQ_UART0 = 10（定义在 board.h）
 *   - rt_hw_interrupt_install() / rt_hw_interrupt_umask()（定义在 rthw.h）
 */

#include <rtthread.h>
#include <rthw.h>
#include "board.h"
#include "uart_irq.h"

/* ============== 全局数据 ============== */

struct uart_rx_buf  g_uart_rx;
struct rt_semaphore g_uart_rx_sem;


//  ISR 写入一个字符
static void rb_put(char c){
    if(g_uart_rx.read_idx==((g_uart_rx.write_idx + 1) % UART_RX_BUF_SIZE)){
        return;
    }//当写入追上读取 那么数据就覆盖了还没有被读走的字符
    g_uart_rx.buf[g_uart_rx.write_idx] = c;
    g_uart_rx.write_idx = (g_uart_rx.write_idx + 1) % UART_RX_BUF_SIZE;
}    
static char rb_get(void){    
    char c=g_uart_rx.buf[g_uart_rx.read_idx];
    g_uart_rx.read_idx=(g_uart_rx.read_idx+1)%UART_RX_BUF_SIZE;
    return c;
}      // getchar 拿走一个字符


/* ============== UART 中断服务例程 ============== */

static void uart_isr(int vector, void *param)
{
    /*  检查 LSR bit0：真的有数据吗 */
    volatile unsigned char *lsr = (volatile unsigned char *)(UART_BASE_ADDR + UART_LSR);
    if ((*lsr & LSR_RX_READY) == 0)
        return;

    /* 读走字节 */
    volatile unsigned char *rbr = (volatile unsigned char *)(UART_BASE_ADDR + UART_RBR);
    char ch = (char)(*rbr);

    /* 放入 ring buffer */
    rb_put(ch);

    /* 唤醒 finsh */
    rt_sem_release(&g_uart_rx_sem);
}

/* ============== 初始化 ============== */


 
void uart_irq_init(void)
{
    // 1. 初始化 ring buffer（索引清零）
    for(int i=0;i<UART_RX_BUF_SIZE;i++){
        g_uart_rx.buf[i] = 0;
    }
      //2. 初始化信号量: 
      rt_sem_init(&g_uart_rx_sem, "urx", 0, RT_IPC_FLAG_FIFO);
         //初值为 0 表示缓冲区一开始是空的
 
     // 3. 注册 UART ISR 到中断号 PLIC_IRQ_UART0 (10)
         rt_hw_interrupt_install(PLIC_IRQ_UART0, uart_isr, RT_NULL, "uart0");
 
   // 4. 使能 PLIC 中断源
        rt_hw_interrupt_umask(PLIC_IRQ_UART0);
 
     // 5. 配置 UART IER 寄存器，使能 RX 中断
         volatile unsigned char *ier = (volatile unsigned char *)(UART_BASE_ADDR + UART_IER);
         *ier = IER_RX_INT;   // bit0 = 1
}




char rt_hw_console_getchar(void)
{
    // 1. 阻塞等待信号量（ISR 收到字符后会 release）
        rt_sem_take(&g_uart_rx_sem, RT_WAITING_FOREVER);

    // 2. 从 ring buffer 取走一个字符并返回
        return rb_get();
}


/* ============== 你需要改动的其他文件 ==============
 *
 * 1. board.c → rt_hw_board_init() 中调用 uart_irq_init()
 *
 *    void rt_hw_board_init(void)
 *    {
 *        rt_hw_interrupt_init();
 *        rt_system_heap_init(HEAP_BEGIN, HEAP_END);
 *        uart_irq_init();          // ← 加这行
 *        // ...
 *    }
 *
 * 2. Makefile → C_SRC 中添加 bsp/uart_irq.c
 *
 *    C_SRC += bsp/board.c \
 *             bsp/uart_irq.c         ← 加这行
 *
 * 3. 确保 board.c 里的旧 getchar 是 RT_WEAK（当前已经是）
 *    你的新强符号会自动覆盖它
 */
