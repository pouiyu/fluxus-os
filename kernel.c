/* ============================================
 * kernel.c - Fluxus OS 内核入口
 * 功能：启动画面 → 调试桌面 + 实时鼠标光标
 * 运行环境：x86 保护模式（无标准库依赖）
 * ============================================ */

/* ---------- VGA 常量（与 func.c 保持一致） ---------- */
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200
#define FONT_WIDTH    8
#define FONT_HEIGHT   8

/* ---------- 外部函数声明（定义于 func.c） ---------- */
extern void put_pixel(int x, int y, int color);
extern void draw_char(int x, int y, char c, int fg, int bg);
extern void draw_string(int x, int y, const char *str, int fg, int bg);
extern void clear_screen(int color);
extern int  strlen_simple(const char *str);
extern int  get_mouse_pos_x(void);
extern int  get_mouse_pos_y(void);
extern int  get_mouse_btn(void);
extern void init_input(void);
extern void update_mouse_cursor(void);

/* ============================================
 * int_to_str - 将整数转为十进制字符串（自实现，无标准库）
 * @num:    待转换的整数
 * @buf:    输出缓冲区（至少 12 字节）
 * 返回：    字符串指针（同 buf）
 * ============================================ */
static char *int_to_str(int num, char *buf)
{
    int i = 0, j, is_neg = 0;
    char tmp;

    if (num == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    if (num < 0)
    {
        is_neg = 1;
        num = -num;
    }

    while (num > 0)
    {
        buf[i++] = (char)('0' + (num % 10));
        num /= 10;
    }

    if (is_neg)
        buf[i++] = '-';

    buf[i] = '\0';

    /* 反转字符串 */
    for (j = 0; j < i / 2; j++)
    {
        tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }

    return buf;
}

/* ============================================
 * kernel_main - 内核主入口
 * ============================================ */
__attribute__((section(".text.entry")))
void kernel_main(void)
{
    const char *message;
    int text_width, center_x, center_y;

    /* ---- 第 1 步：黑色背景 + 启动信息 ---- */
    clear_screen(0);                                        /* 黑色 */

    message = "Fluxus Starting...";
    text_width = strlen_simple(message) * FONT_WIDTH;
    center_x   = (SCREEN_WIDTH  - text_width)  / 2;
    center_y   = (SCREEN_HEIGHT - FONT_HEIGHT) / 2;
    draw_string(center_x, center_y, message, 15, 0);        /* 白色居中 */

    /* ---- 第 2 步：初始化中断 + 鼠标 ---- */
    init_input();

    /* ---- 第 3 步：青色桌面 ---- */
    clear_screen(3);                                        /* 青色 */

    /* ---- 第 4 步：调试信息标题 ---- */
    draw_string(0, 0, "Fluxus Debug Console", 15, 1);       /* 白字蓝底标题 */
    draw_string(0, 10, "--------------------", 7, 3);        /* 分隔线 */

    /* ---- 第 5 步：静态调试标签 ---- */
    draw_string(0, 20, "Mouse X:", 15, 3);
    draw_string(0, 30, "Mouse Y:", 15, 3);
    draw_string(0, 40, "Buttons:", 15, 3);
    draw_string(0, 50, "Status:  OK", 7, 3);

    /* ---- 第 6 步：主循环 ---- */
    while (1)
    {
        char buf[12];
        int mx, my, btn;

        /* 刷新鼠标光标（擦旧 → 保存背景 → 绘新） */
        update_mouse_cursor();

        /* 读取当前鼠标状态 */
        mx  = get_mouse_pos_x();
        my  = get_mouse_pos_y();
        btn = get_mouse_btn();

        /* 更新调试数值（覆盖上一帧的旧数字） */
        draw_string(64, 20, "    ", 15, 3);                 /* 擦旧值 */
        draw_string(64, 20, int_to_str(mx, buf), 15, 3);    /* 写新 X */

        draw_string(64, 30, "    ", 15, 3);
        draw_string(64, 30, int_to_str(my, buf), 15, 3);    /* 写新 Y */

        draw_string(64, 40, "   ", 15, 3);
        draw_string(64, 40, int_to_str(btn, buf), 15, 3);   /* 写按键 */

        __asm__ volatile ("hlt");   /* CPU 休眠，等待中断唤醒 */
    }
}
