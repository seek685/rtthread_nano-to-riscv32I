/*
 * RISC-V 32I RT-Thread Nano 链接脚本
 * 适用于无 Flash、纯 RAM 运行的平台
 */

OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    /* 所有段都在 RAM 中（无 Flash 设计） */
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 64K
}

_stack_size = 0x2000;             /* 系统主栈大小 8KB   */
_heap_size  = 0x4000;             /* 动态内存堆大小 16KB */

SECTIONS
{
    . = ORIGIN(RAM);

    /* ---- 代码段 ---- */
    .text : ALIGN(4) {
        KEEP(*(.text.init))       /* 启动代码（start.S）        */
        KEEP(*(.text.trap))       /* trap 入口（trap_entry.S）  */
        *(.text .text.*)          /* 应用程序和内核代码          */

        /* RT-Thread 自动初始化段 */
        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;


    } > RAM

    /* ---- 只读数据段 ---- */
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)    /* RISC-V 小只读数据段 */

         /* RT-Thread Finsh/MSH 组件符号表（如开启了控制台） */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;

        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;
    } > RAM

    /* ---- 已初始化数据段 ---- */
    .data : ALIGN(4) {
        _data_start = .;
        *(.data .data.*)

        /* 全局指针符号，放在 .sdata 前面
         * 偏移 0x800 可覆盖前后各 2KB 的小数据区域 */
        PROVIDE( __global_pointer$ = . + 0x800 );

        *(.sdata .sdata.*)        /* RISC-V 小数据段 */
        _data_end = .;
    } > RAM

    /* ---- 未初始化数据段 (BSS) ---- */
    .bss (NOLOAD) : ALIGN(4) {
        _bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)          /* RISC-V 小 BSS 段 */
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    /* ---- RT-Thread 堆区域 ---- */
    .heap (NOLOAD) : ALIGN(8) {
        __heap_start = .;
        . += _heap_size;
        __heap_end = .;
    } > RAM

    /* ---- 系统主栈 ---- */
    .stack (NOLOAD) : ALIGN(16) {
        __stack_start = .;
        . += _stack_size;
        . = ALIGN(16);
        __stack_top = .;          /* 栈顶，sp 初始值设为此地址（栈向下增长） */
    } > RAM

    _system_ram_end = .;

    /* 丢弃 .eh_frame（异常处理帧，裸机不需要） */
    /DISCARD/ : {
        *(.eh_frame)
    }
}
