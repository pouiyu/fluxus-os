# ============================================
# Makefile - Fluxus OS 构建系统
# 功能：编译 boot.asm、kernel.c 和 func.c，
#       生成可启动软盘镜像 fluxus.img
# 需求：nasm, gcc (i386 交叉编译), ld, objcopy, dd
# ============================================

# ---------- 工具链定义 ----------
NASM       = nasm
CC         = gcc
LD         = ld
OBJCOPY    = objcopy
DD         = dd
RM         = rm -f
RMDIR      = rm -rf

# ---------- 源文件 ----------
BOOT_SRC   = boot.asm
BOOT_BIN   = boot.bin
KERNEL_SRC = kernel.c
KERNEL_OBJ = kernel.o
FUNC_SRC   = func.c
FUNC_OBJ   = func.o
ISR_SRC    = isr.asm
ISR_OBJ    = isr.o
KERNEL_BIN = kernel.bin
LINKER_LD  = linker.ld

# ---------- 输出镜像 ----------
IMG_OUTPUT = fluxus.img
ISO_OUTPUT = fluxus.iso

# ---------- 编译标志 ----------
# NASM：输出原始二进制 (512 字节引导扇区)
NASM_FLAGS = -f bin

# GCC：32 位、独立环境、无标准库、优化体积
CFLAGS     = -m32 -nostdlib -nostdinc -fno-builtin           \
             -fno-stack-protector -nostartfiles -nodefaultlibs \
             -Wall -Wextra -c -std=c99 -ffreestanding -Os

# LD：32 位 ELF 输出，平坦二进制
#     链接 kernel.o + func.o 后由 objcopy 提取为 binary
LDFLAGS    = -m elf_i386 -T $(LINKER_LD) -nostdlib

# ---------- 默认目标 ----------
.PHONY: all
all: $(IMG_OUTPUT)

# ---------- 编译引导扇区 (NASM → 512 字节原始二进制) ----------
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) $(NASM_FLAGS) -o $@ $<

# ---------- 编译 kernel.c 为目标文件 ----------
$(KERNEL_OBJ): $(KERNEL_SRC)
	$(CC) $(CFLAGS) -o $@ $<

# ---------- 编译 func.c 为目标文件 ----------
$(FUNC_OBJ): $(FUNC_SRC)
	$(CC) $(CFLAGS) -o $@ $<

# ---------- 编译 isr.asm 为 ELF32 目标文件 ----------
$(ISR_OBJ): $(ISR_SRC)
	$(NASM) -f elf32 -o $@ $<

# ---------- 链接 kernel.o + func.o + isr.o 并提取为平坦二进制 ----------
$(KERNEL_BIN): $(KERNEL_OBJ) $(FUNC_OBJ) $(ISR_OBJ) $(LINKER_LD)
	$(LD) $(LDFLAGS) -o kernel.elf $(KERNEL_OBJ) $(FUNC_OBJ) $(ISR_OBJ)
	$(OBJCOPY) -O binary kernel.elf $@
	$(RM) kernel.elf

# ---------- 制作 1.44MB 软盘镜像 ----------
$(IMG_OUTPUT): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "==> Creating 1.44MB floppy disk image..."
	# 创建空白 1.44MB 软盘镜像 (2880 扇区 × 512 字节)
	$(DD) if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	# 写入引导扇区到第 0 扇区
	$(DD) if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	# 写入内核二进制到后续扇区（从第 1 扇区开始）
	$(DD) if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "==> Done: $@"

# ---------- 制作 ISO 光盘镜像（可选） ----------
$(ISO_OUTPUT): $(IMG_OUTPUT)
	@echo "==> Creating ISO image..."
	@cp $(IMG_OUTPUT) $(ISO_OUTPUT)
	@echo "==> Done: $@"

# ---------- 使用 QEMU 运行 ----------
.PHONY: run
run: $(IMG_OUTPUT)
	qemu-system-i386 -drive file=$(IMG_OUTPUT),format=raw,if=floppy -m 128

# ---------- 使用 QEMU 运行（无图形窗口，串口重定向） ----------
.PHONY: run-nographic
run-nographic: $(IMG_OUTPUT)
	qemu-system-i386 -drive file=$(IMG_OUTPUT),format=raw,if=floppy -m 128 -nographic

# ---------- 使用 QEMU 运行 ISO ----------
.PHONY: run-iso
run-iso: $(ISO_OUTPUT)
	qemu-system-i386 -cdrom $(ISO_OUTPUT) -m 128

# ---------- 清理构建产物 ----------
.PHONY: clean
clean:
	$(RM) $(BOOT_BIN) $(KERNEL_OBJ) $(FUNC_OBJ) $(ISR_OBJ) $(KERNEL_BIN)
	$(RM) $(IMG_OUTPUT) $(ISO_OUTPUT) kernel.elf
	@echo "==> Clean complete."

# ---------- 完全重新构建 ----------
.PHONY: rebuild
rebuild: clean all

# ---------- 帮助信息 ----------
.PHONY: help
help:
	@echo "============================================"
	@echo "  Fluxus OS 构建系统"
	@echo "============================================"
	@echo ""
	@echo "可用目标："
	@echo "  make          - 构建启动软盘镜像 fluxus.img"
	@echo "  make run      - 构建并在 QEMU 中运行"
	@echo "  make clean    - 清理所有构建产物"
	@echo "  make rebuild  - 清理后重新构建"
	@echo "  make help     - 显示此帮助信息"
	@echo ""
	@echo "依赖工具：nasm, gcc (multilib), ld (binutils), objcopy, dd"
	@echo "QEMU 运行需求：qemu-system-i386"
