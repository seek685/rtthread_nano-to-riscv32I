/*
 * applications/main.c — 用户应用程序入口
 *
 * RT-Thread 启动后自动创建 main 线程并调用此函数。
 * 在此编写你的业务逻辑。
 */

#include <rtthread.h>

int main(void)
{
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

static int cnt,sum;
static int k=100;
void rtthread1_entry(void *parameter){
    for(int j=0;j<10;j++){
        for(cnt=1;cnt<k;cnt++){
            sum+=cnt;
        }
        rt_kprintf("add 1 to %d,sum=%d\r\n",k-1,sum);
        k+=100;
        sum=0;
    }
}
int simple_add(void){
    rt_thread_t thread1;

    thread1=rt_thread_create("thread1",rtthread1_entry,RT_NULL,512,25,10);

    if(thread1!=RT_NULL){
        rt_thread_startup(thread1);
    }
}

MSH_CMD_EXPORT(simple_add,simple add);