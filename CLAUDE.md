# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

RISC-V 32I 平台上的 RT-Thread Nano 移植，目标为 QEMU virt 机器，无 Flash、纯 RAM 运行（所有段在 RAM 中）。运行在机器模式（Machine Mode），无 MMU，单 hart。

## 构建与运行

```bash
mingw32-make                    # 编译 ELF
mingw32-make clean              # 清理
mingw32-make dump               # 生成反汇编 rtthread.dump
mingw32-make size               # 查看段大小

# QEMU 运行
qemu-system-riscv32 -machine virt -nographic -bios none -kernel rtthread.elf
```

## 架构分层

```
applications/main.c    ← 用户入口 (rt_application_init → main)
finsh/                 ← FinSH 命令行 Shell
device/                ← RT-Thread 设备框架
src/                   ← RT-Thread 内核源码
bsp/                   ← 板级支持包（board.c, board.h, rtconfig.h）
libcpu/                ← RISC-V 架构移植层
  ├── interrupt.c      ← CLINT + PLIC 初始化 + trap C 分发
  ├── cpuport.c        ← 栈帧初始化 + 中断上下文切换
  ├── cpuport.h        ← REGBYTES/LOAD/STORE 宏
  ├── trap_entry.S     ← 汇编级 trap 入口
  ├── context_gcc.S    ← 上下文切换汇编 + mret 恢复
  ├── riscv-ops.h      ← CSR 读写宏
  └── riscv-plic.h     ← PLIC 寄存器操作内联函数
start.S                ← 启动代码
link.ld                ← 链接脚本（RAM @ 0x80000000, 64KB）
```

## 关键设计决策

### 中断两级架构: CLINT + PLIC
- **CLINT** (`0x0200_0000`): per-hart 定时器，mtimecmp 触发 mcause=7
- **PLIC** (`0x0C00_0000`): 平台级外设中断，仲裁后触发 mcause=11
- PLIC 流程: Claim(read) → Handle(ISR) → Complete(write source ID back)
- RV32 写 mtimecmp 三步安全序列: 低半=~0 → 高半=目标 → 低半=目标

### 上下文切换
- ISR 中只设标志，trap 出口处实际切换（延迟执行）
- 栈帧: 32×4 字节，sp[0]=epc, sp[1]=ra, sp[2]=mstatus

### BSP 配置
- CPU 50MHz, tick=100Hz, MAX_IRQ_SOURCES=32, UART0=IRQ10
- 主栈 8KB, 堆 16KB

## 常见修改点

- **更换平台**: 修改 `board.h` 中 CLINT/PLIC/UART 基地址和 `CPU_FREQ`
- **调整 ISA**: 修改 `Makefile` 中 `ARCH_FLAGS`
- **添加外设中断**: 定义 `PLIC_IRQ_XXX` → `rt_hw_interrupt_install()` → `rt_hw_interrupt_umask()`
- **调整内存**: 修改 `link.ld` 中 `_stack_size`/`_heap_size`

---

## 行为准则

Behavioral guidelines from [andrej-karpathy-skills](https://github.com/forrestchang/andrej-karpathy-skills).

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

### 1. Think Before Coding
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them — don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### 2. Simplicity First
- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

### 3. Surgical Changes
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it — don't delete it.
- Remove imports/variables/functions that YOUR changes made unused.

### 4. Goal-Driven Execution
- Transform tasks into verifiable goals with clear success criteria.
- For multi-step tasks, state a brief plan with verification per step.
- Strong success criteria let you loop independently.
