/*
 * applications/main.c — 用户应用程序入口
 *
 * RT-Thread 启动后自动创建 main 线程并调用此函数。
 * 在此编写你的业务逻辑。
 */

#include <rtthread.h>

int main(void)
{
    /* 示例: 闪烁 LED 或打印信息 */
    rt_kprintf("timer init!\n");
    rt_kprintf("system init!\n");
    rt_kprintf("app init!\n");
    rt_kprintf("timer thread init!\n");
    rt_kprintf("idle init!\n");
    rt_kprintf("schedule start!\n");
    while (1)
    {
        rt_thread_mdelay(1000);
    }

    return 0;
}
