; ============================================
; isr.asm - Fluxus OS 中断服务例程（汇编桩）
; 功能：保护寄存器 → 调用 C 处理函数 → 发送 EOI → 返回
; 语法：NASM 32 位保护模式
; ============================================

[BITS 32]

; ---------- 外部 C 函数声明 ----------
[EXTERN irq_handler]          ; 定义于 func.c，void irq_handler(int irq)

; ---------- 导出中断桩符号（供 func.c 设置 IDT） ----------
[GLOBAL isr_irq0]
[GLOBAL isr_irq1]
[GLOBAL isr_irq2]
[GLOBAL isr_irq3]
[GLOBAL isr_irq4]
[GLOBAL isr_irq5]
[GLOBAL isr_irq6]
[GLOBAL isr_irq7]
[GLOBAL isr_irq8]
[GLOBAL isr_irq9]
[GLOBAL isr_irq10]
[GLOBAL isr_irq11]
[GLOBAL isr_irq12]           ; 鼠标中断 (IRQ12)
[GLOBAL isr_irq13]
[GLOBAL isr_irq14]
[GLOBAL isr_irq15]

; ============================================
; 通用中断处理宏
; 流程：保存全部寄存器 → 调用 irq_handler(n) → EOI → 恢复寄存器 → iret
; ============================================
%macro ISR_STUB 1
isr_irq%1:
    pushad                    ; 保存 EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
    push ds
    push es
    push fs
    push gs

    mov  ax, 0x10             ; 内核数据段选择子
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax

    push %1                   ; 参数：中断号
    call irq_handler          ; 调用 C 语言中断分发函数
    add  esp, 4               ; 清理栈上参数

    ; ---------- 发送 EOI (End Of Interrupt) ----------
    mov  al, 0x20
%if %1 >= 8                   ; IRQ 8~15 来自从 PIC，需向两片都发 EOI
    out  0xA0, al             ; 向从 PIC 发送 EOI
%endif
    out  0x20, al             ; 向主 PIC 发送 EOI

    pop  gs
    pop  fs
    pop  es
    pop  ds
    popad                     ; 恢复全部通用寄存器

    iret                      ; 中断返回
%endmacro

; ---------- 为 IRQ 0~15 生成中断桩 ----------
section .text
ISR_STUB 0
ISR_STUB 1
ISR_STUB 2
ISR_STUB 3
ISR_STUB 4
ISR_STUB 5
ISR_STUB 6
ISR_STUB 7
ISR_STUB 8
ISR_STUB 9
ISR_STUB 10
ISR_STUB 11
ISR_STUB 12                   ; 鼠标 (PS/2 IRQ12)
ISR_STUB 13
ISR_STUB 14
ISR_STUB 15
