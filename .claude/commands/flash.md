---
name: flash
description: 编译项目并在 QEMU 上运行
---

编译 RISC-V 32I RT-Thread Nano 项目并在 QEMU virt 机器上运行。

## 步骤

1. 先执行 `mingw32-make` 编译项目
2. 如果编译成功，执行 `qemu-system-riscv32 -machine virt -nographic -bios none -kernel rtthread.elf` 运行
3. 如果编译失败，报告错误信息并停止
