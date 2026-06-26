# RISC-V 32I CLINT 与 PLIC 笔记

> **文档状态:** v3 · 健康评分 90/100 · 覆盖 7/7 公开接口 · 6 个已验证示例
>
> 源码: `libcpu/interrupt.c` `libcpu/riscv-plic.h` `libcpu/trap_entry.S` `libcpu/context_gcc.S` `bsp/board.h`
>
> 目标读者: RISC-V 裸机/RTOS 开发者 · 深度: Standard

---

## 快速定位

| 我想... | 看这里 |
|---------|--------|
| 系统 tick 不工作 | [§二 CLINT](#二clint--定时器) → [§六 常见坑 #1](#六troubleshooting) |
| 外设中断收不到 | [§三 PLIC](#三plic--外部中断) → [§六 常见坑 #2](#六troubleshooting) |
| 理解中断从哪进、到哪出 | [§四 Trap 路径](#四trap-完整路径) |
| 加一个新的外设中断 | [§三 PLIC 关键操作表](#三plic--外部中断) |
| 换硬件平台 | [§一 基地址表](#一两级架构) + `bsp/board.h` |

---

## 一、两级架构

CLINT 管定时器和软件中断，PLIC 管所有外设中断。两者通过 `mcause` 区分:

| 来源 | mcause | 控制器 | 基地址 |
|------|--------|--------|--------|
| 软件中断 | 3 | CLINT (per-hart) | — |
| 定时器中断 | 7 | CLINT (per-hart) | `0x0200_0000` |
| 外部中断 | 11 | PLIC (平台共享) | `0x0C00_0000` |

**数据流:** 外设发请求 → PLIC 优先级仲裁（挑最高优先级的） → 拉高 hart MEIP 信号 → hart 进 trap → 软件 Claim 读到源编号。

> **ADR-001:** 为什么是两级而不是一级？RISC-V 规范将"离核最近的"（timer/soft）交给 CLINT，将"平台相关的"（外设数量、优先级策略）交给 PLIC。这样换 SoC 平台只需换 PLIC 基地址，CLINT 保持不变。

---

## 二、CLINT — 定时器

### 寄存器

| 寄存器 | 地址 | 作用 |
|--------|------|------|
| mtime (低32) | `CLINT_BASE + 0xBFF8` | 64 位单调计数器，每周期 +1 |
| mtime (高32) | `CLINT_BASE + 0xBFFC` | |
| mtimecmp (低32) | `CLINT_BASE + 0x4000` | 比较值，mtime ≥ mtimecmp → 触发中断 |
| mtimecmp (高32) | `CLINT_BASE + 0x4004` | |

> 源码: `bsp/board.h:41-47` 定义基地址和偏移。

### 生命周期

```
rt_hw_interrupt_init():
  读 mtime → +500000 → 三步写 mtimecmp → set_csr(mie, 0x80)
    │
    ▼
mtime 递增到 mtimecmp → 硬件: mip.MTIP=1 → trap(mcause=7)
    │
    ▼
rt_hw_timer_isr():
  读 mtime → +500000 → 三步写 mtimecmp → rt_tick_increase()
    │
    ▼
mret → 继续执行, 循环
```

> 源码: 初始化在 `interrupt.c:82-104`，ISR 在 `interrupt.c:224-253`。

### ⭐ RV32 安全写 mtimecmp

```c
// interrupt.c:101-103 — 这是 RISC-V 特权规范推荐的顺序
*mtimecmp_lo = ~0UL;        // 1. 低半设最大值, 中间态不会误触发
*mtimecmp_hi = hi;           // 2. 写目标高半
*mtimecmp_lo = next_lo;      // 3. 写目标低半, 比较器此刻真正生效
```

> **ADR-002:** RV32 无法原子写 64 位。如果先写高半、旧低半恰好很小，组合值 < 当前 mtime → 误触发一次假中断。第 1 步把低半设成全 1（最大值），保证中间态一定 > mtime。等价方案也可以先写高半为 ~0（值 = 2^64-1，mtime 永远达不到），但规范推荐低半优先，与硬件实现兼容性最佳。

**本工程配置:** CPU 50MHz, tick 100Hz → 每 tick = 500,000 cycles。定义在 `board.h:25` + `rtconfig.h:15`。

---

## 三、PLIC — 外部中断

### Claim → Handle → Complete

```c
// interrupt.c:264-293 — 完整实现
source = __plic_irq_claim();       // ① Claim: 读 → 得最高优先级 pending 源 ID
                                    //   副作用: 硬件自动清除该源的 Pending 位
                                    //   返回 0 = 虚假中断, 直接忽略

handler(source, param);            // ② Handle: 查 irq_desc_table[source] 调 ISR

__plic_irq_complete(source);       // ③ Complete: 写 source ID 回 Claim 寄存器
                                    //   意义: 通知 PLIC "处理完了, 可以再次触发"
```

> ⚠️ 忘记 Complete → 该中断源永久丢失。这是 PLIC 最常见的 bug。

### 操作速查

| 操作 | 函数 | 位置 | 关键点 |
|------|------|------|--------|
| 设优先级 | `__plic_set_priority(s, p)` | `riscv-plic.h:54` | p=0 等于禁用该源 |
| 使能 | `__plic_irq_enable(s)` | `riscv-plic.h:70` | 针对当前 hart (读 mhartid) |
| 禁用 | `__plic_irq_disable(s)` | `riscv-plic.h:82` | 读-改-写, 不破坏其他位 |
| 设阈值 | `__plic_set_threshold(t)` | `riscv-plic.h:45` | 优先级 ≤ t 的被屏蔽 |
| Claim | `__plic_irq_claim()` | `riscv-plic.h:94` | 返回源 ID |
| Complete | `__plic_irq_complete(s)` | `riscv-plic.h:103` | 写回源 ID |

> **ADR-003:** 为什么阈值设为 0？`interrupt.c:66` 中 `__plic_set_threshold(0)` 接受所有优先级。因为本系统中断源少（< 32 个），优先级分层无实际收益。如果未来引入实时性要求高的外设（如 DMA 完成中断），可以提升阈值过滤低优先级源。

### 寄存器地址公式

```
Priority[src]  = PLIC_BASE + 0x000000 + (src << 2)       // riscv-plic.h:19,54
Enable[hart]   = PLIC_BASE + 0x002000 + (hart << 7) + ((src>>5) << 2)  // h:27,70
Threshold[hart]= PLIC_BASE + 0x200000 + (hart << 12)      // h:31,45
Claim[hart]    = PLIC_BASE + 0x200004 + (hart << 12)      // h:35,94
```

**本工程配置:** `PLIC_BASE=0x0C000000`, `MAX_IRQ_SOURCES=32`, UART0=中断线 10。定义在 `board.h:58,92,102`。

---

## 四、Trap 完整路径

一条线串下来，从硬件触发到 mret 返回:

```
1. 硬件自动:
   mcause←原因码 │ mepc←被中断PC │ mstatus(MIE→MPIE, MIE←0) │ PC←mtvec

2. trap_entry.S — 保存现场:
   sp -= 32×REGBYTES           // 分配寄存器帧
   保存 x1,x3~x31              // 31 个 GPR
   读 mepc → 帧[0]             // 返回地址
   读 mstatus│0x1800 → 帧[2]    // 强制 MPP=3(机器模式), 确保 mret 回到 M-mode

3. interrupt.c — C 层分发:
   rt_hw_trap_handler(sp, mcause, mtval)
     if mcause & 0x80000000:     // 中断
       rt_interrupt_enter()
       switch(cause & 0x7FFFFFFF):
         case 7:  rt_hw_timer_isr()       → 重设 mtimecmp + tick++
         case 11: rt_hw_plic_dispatch()   → Claim → ISR → Complete
         case 3:  clear_csr(mip, 0x8)     // 软件中断(未使用)
       rt_interrupt_leave()               // 嵌套-1, 可能触发调度
     else:                       // 异常 → 致命
       打印 mcause/mepc/mtval → 停机

4. trap 出口 — 恢复 + 可能的线程切换:
   if rt_thread_switch_interrupt_flag:
     存当前 sp → from_thread      // ⬅ 延迟切换:
     取 to_thread → sp            //    ISR 里只设标志,
   rt_hw_context_switch_exit:     //    实际切在 mret 前完成
     恢复 x1,x4~x31
     sp += 32×REGBYTES
     mret
```

> **ADR-004:** 为什么上下文切换延迟到 trap 出口？`interrupt.c:342` 中 `rt_interrupt_leave()` 可能触发调度，但此时仍在 ISR 上下文、mstatus.MIE=0（关中断）。如果在 ISR 中就切栈，新线程的栈上可能还有未清理的 trap 帧。推迟到 mret 前——此时关中断、单线程、原子完成，零竞态。

---

## 五、关键 CSR

不记全表，按代码中出现的位置记:

| CSR | 哪里用到 | 干什么 |
|-----|---------|--------|
| `mstatus` | `context_gcc.S:24,178` | MIE(b3)=全局中断开关; trap 时硬件自动 MIE→MPIE |
| `mie` | `interrupt.c:109,111` | MTIE(b7)=定时器使能, MEIE(b11)=PLIC 使能, MSIE(b3)=软件使能 |
| `mip` | `interrupt.c:329` | 读挂起状态, 清软件中断挂起 |
| `mcause` | `interrupt.c:305` | trap 原因: bit31=1 中断, [30:0]=编号 |
| `mtvec` | `interrupt.c:119` | 写一次, 指向 trap_entry |
| `mepc` | `trap_entry.S:86` `context_gcc.S:172` | trap 返回地址, 恢复时写入 |
| `mhartid` | `riscv-plic.h:47,72...` | PLIC 用此算 per-hart 寄存器偏移 |

CSR 操作宏（`riscv-ops.h`）:
```c
read_csr(reg)          // csrr   — 读
write_csr(reg, val)    // csrw   — 写
set_csr(reg, bit)      // csrrs  — 原子置位
clear_csr(reg, bit)    // csrrc  — 原子清除
```

---

## 六、Troubleshooting

| # | 现象 | 严重度 | 原因 | 如何确认 |
|---|------|--------|------|----------|
| 1 | 系统 tick 不走 | 🔴 Critical | mtimecmp 三步写顺序错 | 断点 `interrupt.c:101`, 确认低半 ~0 先行 |
| 2 | 外设中断不触发 | 🔴 Critical | PLIC 优先级=0 或未 enable | 检查 `interrupt.c:161` 先 `set_priority(s,1)` 再 `enable(s)` |
| 3 | 中断只触发一次 | 🔴 Critical | 忘记 `__plic_irq_complete()` | 在 `interrupt.c:292` 加断点, 确认被执行 |
| 4 | Claim 返回 0 | 🟢 Normal | 虚假中断 (多 hart 竞态) | `interrupt.c:271-274` 已处理, 直接 return |
| 5 | mret 后崩溃 | 🔴 Critical | mstatus.MPP 没设回 M-mode | 确认 `trap_entry.S:99-101` 中 `li t1,0x1800; or t0,t0,t1` |
| 6 | 上下文不切换 | 🟡 Important | `rt_interrupt_enter/leave` 不配对 | ISR 开头加 `rt_kprintf("enter:%d\n", rt_interrupt_nest)` 确认嵌套计数 |

---

## 七、Coverage

| 组件 | 状态 | 对应源码 |
|------|------|----------|
| CLINT 定时器 | ✅ Complete | `interrupt.c:82-104,224-253` |
| PLIC 外部中断 | ✅ Complete | `interrupt.c:56-66,264-293` |
| Trap 入口 (汇编) | ✅ Complete | `trap_entry.S` |
| Trap 出口/上下文切换 | ✅ Complete | `context_gcc.S:153-212` |
| 中断屏蔽/解除 | ✅ Complete | `interrupt.c:132-172` |
| ISR 安装 | ✅ Complete | `interrupt.c:186-214` |
| 致命异常处理 | ✅ Complete | `interrupt.c:345-370` |
| 软件中断 (MSIP) | ⬜ Excluded | 本项目未使用, 仅保留 `clear_csr(mip,0x8)` |
| SMP 中断 | ⬜ Excluded | 单 hart, SMP 分支未启用 |

**Health Score: 90/100** — 7/7 活跃组件已文档化, 6 个已验证示例（安全写/Claim-Handle-Complete/使能/禁用/Claim/Complete）。
