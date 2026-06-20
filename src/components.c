/* * 版权所有 (c) 2006-2018，RT-Thread 开发团队
 *
 * SPDX 许可证标识符: Apache-2.0
 *
 * 修改日志:
 * 日期           作者          说明
 * 2012-09-20     Bernard      重命名为 components.c，统一组件初始化
 * 2012-12-23     Bernard      修复 pthread 初始化问题
 * 2013-06-23     Bernard      增加组件自动初始化机制 init_call
 * 2013-07-05     Bernard      移除 MS VC++ 编译器支持
 * 2015-02-06     Bernard      移除 MS VC++ 支持，回归纯内核
 * 2015-05-04     Bernard      为避免部分 IDE 编译问题重命名为 components.c
 * 2015-07-29     Arda.Fu      增加 IAR 的 RT_USING_USER_MAIN 支持
 */

#include <rthw.h>
#include <rtthread.h>

#ifdef RT_USING_USER_MAIN
#ifndef RT_MAIN_THREAD_STACK_SIZE
#define RT_MAIN_THREAD_STACK_SIZE     2048
#endif
#ifndef RT_MAIN_THREAD_PRIORITY
#define RT_MAIN_THREAD_PRIORITY       (RT_THREAD_PRIORITY_MAX / 3)
#endif
#endif

#ifdef RT_USING_COMPONENTS_INIT
/*
 * 组件自动初始化顺序（通过 INIT_EXPORT 宏将函数指针放入指定段）:
 *
 *   rti_start         --> 0        (初始化起始标记)
 *   BOARD_EXPORT      --> 1        (板级硬件初始化)
 *   rti_board_end     --> 1.end    (板级初始化结束标记)
 *
 *   DEVICE_EXPORT     --> 2        (设备驱动初始化)
 *   COMPONENT_EXPORT  --> 3        (组件初始化: dfs, lwip 等)
 *   FS_EXPORT         --> 4        (文件系统初始化)
 *   ENV_EXPORT        --> 5        (环境初始化: mount 等)
 *   APP_EXPORT        --> 6        (应用程序初始化)
 *
 *   rti_end           --> 6.end    (初始化结束标记)
 *
 * 使用方式:
 *   INIT_BOARD_EXPORT(fn);       // 板级初始化
 *   INIT_DEVICE_EXPORT(fn);      // 设备初始化
 *   INIT_COMPONENT_EXPORT(fn);   // 组件初始化
 *   INIT_ENV_EXPORT(fn);         // 环境初始化
 *   INIT_APP_EXPORT(fn);         // 应用初始化
 */

/* ---- 初始化序列的边界标记 ---- */

static int rti_start(void)
{
    return 0;
}
INIT_EXPORT(rti_start, "0");            /* 0 段起始 */

static int rti_board_start(void)
{
    return 0;
}
INIT_EXPORT(rti_board_start, "0.end");  /* 0 段结束 */

static int rti_board_end(void)
{
    return 0;
}
INIT_EXPORT(rti_board_end, "1.end");    /* 1 段结束 */

static int rti_end(void)
{
    return 0;
}
INIT_EXPORT(rti_end, "6.end");          /* 6 段结束 */

/**
 * 执行板级初始化序列
 *
 * 依次调用注册在 ".rti_fn.1" 段中的所有初始化函数。
 * 由 rt_components_board_init 宏展开为遍历 rti_board_start 到 rti_board_end。
 */
void rt_components_board_init(void)
{
#if RT_DEBUG_INIT
    int result;
    const struct rt_init_desc *desc;
    for (desc = &__rt_init_desc_rti_board_start; desc < &__rt_init_desc_rti_board_end; desc ++)
    {
        rt_kprintf("初始化 %s", desc->fn_name);
        result = desc->fn();
        rt_kprintf(":%d 完成\n", result);
    }
#else
    const init_fn_t *fn_ptr;

    for (fn_ptr = &__rt_init_rti_board_start; fn_ptr < &__rt_init_rti_board_end; fn_ptr++)
    {
        (*fn_ptr)();
    }
#endif
}

/**
 * 执行剩余组件初始化序列
 *
 * 依次调用注册在 ".rti_fn.2" 到 ".rti_fn.6" 段中的所有初始化函数。
 * 由 rt_components_init 宏展开为遍历 rti_board_end 到 rti_end。
 */
void rt_components_init(void)
{
#if RT_DEBUG_INIT
    int result;
    const struct rt_init_desc *desc;

    rt_kprintf("开始组件初始化...\n");
    for (desc = &__rt_init_desc_rti_board_end; desc < &__rt_init_desc_rti_end; desc ++)
    {
        rt_kprintf("初始化 %s", desc->fn_name);
        result = desc->fn();
        rt_kprintf(":%d 完成\n", result);
    }
#else
    const init_fn_t *fn_ptr;

    for (fn_ptr = &__rt_init_rti_board_end; fn_ptr < &__rt_init_rti_end; fn_ptr ++)
    {
        (*fn_ptr)();
    }
#endif
}

#ifdef RT_USING_USER_MAIN

void rt_application_init(void);
void rt_hw_board_init(void);
int rtthread_startup(void);

/*
 * 各编译器的 main 入口适配
 *
 * 不同工具链的启动方式不同，此处统一将控制流导向 rtthread_startup()。
 */

#if defined(__CC_ARM) || defined(__CLANG_ARM)
/* ARMCC / ARMCLANG: 使用 $Sub / $Super 模式替代 main */
extern int $Super$$main(void);
int $Sub$$main(void)
{
    rtthread_startup();
    return 0;
}

#elif defined(__ICCARM__)
/* IAR: __low_level_init 由 cstartup 自动调用，先执行数据段拷贝 */
extern int main(void);
extern void __iar_data_init3(void);
int __low_level_init(void)
{
    __iar_data_init3();     /* IAR 的数据段拷贝函数 */
    rtthread_startup();
    return 0;
}

#elif defined(__GNUC__)
/* GCC: entry 作为链接入口（配合 ld 参数 -eentry） */
extern int main(void);
int entry(void)
{
    rtthread_startup();
    return 0;
}
#endif

#ifndef RT_USING_HEAP
/* 未开启堆时，main 线程使用静态分配的栈和控制块 */
ALIGN(8)
static rt_uint8_t main_stack[RT_MAIN_THREAD_STACK_SIZE];
struct rt_thread main_thread;
#endif

/**
 * main 线程入口函数
 *
 * 完成组件初始化后调用用户 main()。
 */
void main_thread_entry(void *parameter)
{
    extern int main(void);
    extern int $Super$$main(void);

    /* 执行组件初始化（设备、文件系统、网络等） */
    rt_components_init();

    /* 调用用户 main() 函数 */
#if defined(__CC_ARM) || defined(__CLANG_ARM)
    $Super$$main();
#elif defined(__ICCARM__) || defined(__GNUC__)
    main();
#endif
}

/**
 * 创建并启动 main 线程
 *
 * 根据是否开启 RT_USING_HEAP，使用动态或静态方式创建 main 线程。
 */
void rt_application_init(void)
{
    rt_thread_t tid;

#ifdef RT_USING_HEAP
    /* 动态创建: 从堆中分配栈空间 */
    tid = rt_thread_create("main", main_thread_entry, RT_NULL,
                           RT_MAIN_THREAD_STACK_SIZE, RT_MAIN_THREAD_PRIORITY, 20);
    RT_ASSERT(tid != RT_NULL);
#else
    /* 静态创建: 使用全局变量作为栈和控制块 */
    rt_err_t result;

    tid = &main_thread;
    result = rt_thread_init(tid, "main", main_thread_entry, RT_NULL,
                            main_stack, sizeof(main_stack), RT_MAIN_THREAD_PRIORITY, 20);
    RT_ASSERT(result == RT_EOK);

    (void)result;   /* 消除未使用变量的编译警告 */
#endif

    rt_thread_startup(tid);
}

/**
 * RT-Thread 系统启动入口
 *
 * 这是整个系统的初始化主函数。调用顺序:
 *   1. 关全局中断
 *   2. 板级硬件初始化（中断控制器、堆、UART 等）
 *   3. 打印版本信息
 *   4. 初始化系统定时器、调度器
 *   5. 创建 main 线程和 idle 线程
 *   6. 启动调度器 —— 此后永不返回
 */
int rtthread_startup(void)
{
    /* 关闭全局中断，确保初始化过程不被中断 */
    rt_hw_interrupt_disable();

    /* 板级初始化（请在 rt_hw_board_init 中初始化系统堆） */
    rt_hw_board_init();

    /* 打印 RT-Thread 版本 */
    rt_show_version();

    /* 初始化系统定时器 */
    rt_system_timer_init();

    /* 初始化调度器 */
    rt_system_scheduler_init();

#ifdef RT_USING_SIGNALS
    /* 初始化信号系统 */
    rt_system_signal_init();
#endif

    /* 创建 main 线程 */
    rt_application_init();

    /* 初始化定时器服务线程 */
    rt_system_timer_thread_init();

    /* 初始化 idle 线程 */
    rt_thread_idle_init();

    /* 启动调度器 —— 将控制权交给优先级最高的线程，永不返回 */
    rt_system_scheduler_start();

    /* 调度器永不返回；此处仅为消除编译器警告 */
    return 0;
}
#endif
#endif
