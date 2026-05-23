/* ============================================
 * linker.ld - Fluxus OS 内核链接脚本
 * 生成 ELF 中间文件，由 objcopy 提取扁平二进制
 * 内核加载到物理地址 0x8000
 * ============================================ */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(kernel_main)

SECTIONS
{
    /* 内核加载到物理地址 0x8000 */
    . = 0x8000;

    /* 代码段：.text.entry 必须在最前面以确保 kernel_main 作为入口 */
    .text : {
        *(.text.entry)         /* kernel_main 入口函数 */
        *(.text*)              /* 其余代码 */
    }

    /* 只读数据段（含字体数据） */
    .rodata : {
        *(.rodata*)
    }

    /* 已初始化数据段 */
    .data : {
        *(.data*)
    }

    /* BSS 段（未初始化数据） */
    .bss : {
        *(.bss*)
        *(COMMON)
    }

    /* 丢弃不需要的段 */
    /DISCARD/ : {
        *(.comment*)
        *(.note*)
        *(.eh_frame*)
        *(.debug*)
    }
}
