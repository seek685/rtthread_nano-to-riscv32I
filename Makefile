#
# RISC-V 32I RT-Thread Nano 构建文件（无 Flash 平台）
#
# 用法:
#   make                    - 构建 ELF 文件
#   make clean              - 清理构建产物
#   make bin                - 生成原始二进制文件
#   make hex                - 生成 Intel Hex 文件
#   make dump               - 生成反汇编文件
#   make size               - 查看段大小统计
#
# 示例:
#   make CROSS_COMPILE=riscv-none-embed- ARCH_FLAGS='-march=rv32i -mabi=ilp32'
#

# ============== 工具链 ==============

CROSS_COMPILE ?= riscv32-unknown-elf-

CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

# ============== 架构与 CPU 选项 ==============

# 根据你的 RISC-V 核心指令集扩展调整 -march:
#   rv32i      - 仅基础整数指令集
#   rv32im     - 基础整数 + 乘除法
#   rv32imac   - 基础整数 + 乘除法 + 原子操作 + 压缩指令
#   rv32imafc  - 全功能（含单精度浮点）
ARCH_FLAGS  = -march=rv32imac -mabi=ilp32

# 禁用栈溢出保护（裸机程序不需要）
SSP_FLAGS   = -fno-stack-protector

# ============== 编译选项 ==============

INC_DIRS    = -I include \
              -I libcpu \
              -I bsp \
              -I applications

CFLAGS      = $(ARCH_FLAGS) $(SSP_FLAGS) \
              -Wall -Wextra \
              -Os -g \
              -ffunction-sections -fdata-sections \
              -fno-builtin \
              $(INC_DIRS) \
              -D__RTTHREAD__

ASFLAGS     = $(ARCH_FLAGS) -x assembler-with-cpp $(INC_DIRS)

LDFLAGS     = $(ARCH_FLAGS) \
              -nostartfiles \
              -Wl,--gc-sections \
              -Wl,-Map=rtthread.map \
              -T link.ld

# ============== 源文件 ==============

# 启动文件
S_SRC       = start.S

# libcpu（汇编 + C）
S_SRC      += libcpu/context_gcc.S \
              libcpu/trap_entry.S

C_SRC       = libcpu/cpuport.c \
              libcpu/interrupt.c

# RT-Thread 内核源码
C_SRC      += src/clock.c \
              src/components.c \
              src/cpu.c \
              src/idle.c \
              src/ipc.c \
              src/irq.c \
              src/kservice.c \
              src/mem.c \
              src/memheap.c \
              src/mempool.c \
              src/object.c \
              src/scheduler.c \
              src/slab.c \
              src/thread.c \
              src/timer.c

# 板级支持包
C_SRC      += bsp/board.c

# 应用程序源文件
C_SRC      += applications/main.c

# ============== 目标文件 ==============

S_OBJ       = $(S_SRC:.S=.o)
C_OBJ       = $(C_SRC:.c=.o)
OBJS        = $(S_OBJ) $(C_OBJ)

# ============== 输出文件 ==============

TARGET      = rtthread.elf
TARGET_BIN  = rtthread.bin
TARGET_HEX  = rtthread.hex

# ============== 构建规则 ==============

.PHONY: all clean bin hex dump size help

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "  链接    $@"
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo ""
	$(SIZE) $@
	@echo "构建完成: $@"

%.o: %.S
	@echo "  汇编    $<"
	$(AS) $(ASFLAGS) -c $< -o $@

%.o: %.c
	@echo "  编译    $<"
	$(CC) $(CFLAGS) -c $< -o $@

# ============== 辅助目标 ==============

bin: $(TARGET)
	$(OBJCOPY) -O binary $(TARGET) $(TARGET_BIN)
	@echo "二进制文件: $(TARGET_BIN)"

hex: $(TARGET)
	$(OBJCOPY) -O ihex $(TARGET) $(TARGET_HEX)
	@echo "Hex 文件:   $(TARGET_HEX)"

dump: $(TARGET)
	$(OBJDUMP) -D -S $(TARGET) > rtthread.dump
	@echo "反汇编:     rtthread.dump"

size: $(TARGET)
	$(SIZE) $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET_BIN) $(TARGET_HEX) rtthread.map rtthread.dump
	@echo "清理完成。"

# ============== 依赖生成 ==============

# 生成 .d 依赖文件，确保头文件变更后自动重新编译
DEPS = $(OBJS:.o=.d)
-include $(DEPS)

%.d: %.S
	@$(AS) $(ASFLAGS) -MM $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

%.d: %.c
	@$(CC) $(CFLAGS) -MM $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

# ============== 帮助 ==============

help:
	@echo "RISC-V 32I RT-Thread Nano 构建系统"
	@echo ""
	@echo "目标:"
	@echo "  all      - 构建 ELF（默认）"
	@echo "  bin      - 生成原始二进制文件"
	@echo "  hex      - 生成 Intel Hex 文件"
	@echo "  dump     - 生成反汇编文件"
	@echo "  size     - 打印段大小统计"
	@echo "  clean    - 清理构建产物"
	@echo ""
	@echo "变量:"
	@echo "  CROSS_COMPILE - 工具链前缀（默认: riscv32-unknown-elf-）"
	@echo "  ARCH_FLAGS    - ISA 字符串（默认: rv32imac）"
	@echo ""
	@echo "示例:"
	@echo "  make CROSS_COMPILE=riscv-none-embed-"
	@echo "  make CROSS_COMPILE=riscv-none-embed- ARCH_FLAGS='-march=rv32i -mabi=ilp32'"
