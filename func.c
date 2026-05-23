/* ============================================
 * func.c - Fluxus OS 图形函数库
 * 功能：VGA 13h 像素/字符/图形绘制、字体数据
 * 运行环境：x86 保护模式（无标准库依赖）
 * ============================================ */

/* ---------- VGA 13h 图形模式常量 ---------- */
#define VGA_BASE      ((volatile unsigned char *)0xA0000) /* VGA 显存基址 */
#define SCREEN_WIDTH  320                                  /* 屏幕宽度 */
#define SCREEN_HEIGHT 200                                  /* 屏幕高度 */
#define FONT_WIDTH    8                                    /* 字符点阵宽度 */
#define FONT_HEIGHT   8                                    /* 字符点阵高度 */

/* ---------- VGA 端口号 ---------- */
#define VGA_MISC_PORT      0x03C2   /* 杂项输出寄存器（写） */
#define VGA_SEQ_INDEX      0x03C4   /* 定序器索引寄存器 */
#define VGA_SEQ_DATA       0x03C5   /* 定序器数据寄存器 */
#define VGA_CRTC_INDEX     0x03D4   /* CRT 控制器索引寄存器 */
#define VGA_CRTC_DATA      0x03D5   /* CRT 控制器数据寄存器 */
#define VGA_GC_INDEX       0x03CE   /* 图形控制器索引寄存器 */
#define VGA_GC_DATA        0x03CF   /* 图形控制器数据寄存器 */
#define VGA_AC_INDEX       0x03C0   /* 属性控制器索引/数据寄存器 */
#define VGA_AC_READ        0x03C1   /* 属性控制器读寄存器 */
#define VGA_DAC_MASK       0x03C6   /* DAC 掩码寄存器 */
#define VGA_INPUT_STATUS   0x03DA   /* 输入状态寄存器 1 */

/* ---------- I/O 端口读写辅助宏（内联汇编） ---------- */
#define outb(port, value) \
    __asm__ volatile ("outb %0, %1" :: "a"((unsigned char)(value)), "Nd"((unsigned short)(port)))

#define inb(port) ({ \
    unsigned char __v; \
    __asm__ volatile ("inb %1, %0" : "=a"(__v) : "Nd"((unsigned short)(port))); \
    __v; \
})

/* ============================================
 * 8×8 点阵字体位图数据
 * ASCII 32 (空格) 到 ASCII 126 (~) 共 95 个可打印字符
 * 每个字符 8 字节，每字节一行（bit=1 表示点亮该像素）
 * ============================================ */
static const unsigned char font_bitmap[95][8] = {
    /* 32 空格 */   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 33 ! */      {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
    /* 34 " */      {0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00},
    /* 35 # */      {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
    /* 36 $ */      {0x10,0x7C,0xD0,0x7C,0x16,0x7C,0x10,0x00},
    /* 37 % */      {0x62,0x66,0x0C,0x18,0x30,0x66,0x46,0x00},
    /* 38 & */      {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},
    /* 39 ' */      {0x30,0x30,0x60,0x00,0x00,0x00,0x00,0x00},
    /* 40 ( */      {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
    /* 41 ) */      {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
    /* 42 * */      {0x00,0x10,0x54,0x38,0x54,0x10,0x00,0x00},
    /* 43 + */      {0x00,0x10,0x10,0x7C,0x10,0x10,0x00,0x00},
    /* 44 , */      {0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00},
    /* 45 - */      {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
    /* 46 . */      {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    /* 47 / */      {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00},
    /* 48 0 */      {0x3C,0x66,0xCE,0xD6,0xE6,0x66,0x3C,0x00},
    /* 49 1 */      {0x18,0x38,0x78,0x18,0x18,0x18,0x7E,0x00},
    /* 50 2 */      {0x7C,0xC6,0x06,0x1C,0x70,0xC0,0xFE,0x00},
    /* 51 3 */      {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00},
    /* 52 4 */      {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00},
    /* 53 5 */      {0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00},
    /* 54 6 */      {0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00},
    /* 55 7 */      {0xFE,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00},
    /* 56 8 */      {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00},
    /* 57 9 */      {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00},
    /* 58 : */      {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},
    /* 59 ; */      {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00},
    /* 60 < */      {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},
    /* 61 = */      {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
    /* 62 > */      {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},
    /* 63 ? */      {0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00},
    /* 64 @ */      {0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7C,0x00},
    /* 65 A */      {0x10,0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0x00},
    /* 66 B */      {0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00},
    /* 67 C */      {0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00},
    /* 68 D */      {0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00},
    /* 69 E */      {0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xFE,0x00},
    /* 70 F */      {0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xC0,0x00},
    /* 71 G */      {0x3C,0x66,0xC0,0xDE,0xC6,0x66,0x3E,0x00},
    /* 72 H */      {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
    /* 73 I */      {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},
    /* 74 J */      {0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00},
    /* 75 K */      {0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00},
    /* 76 L */      {0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00},
    /* 77 M */      {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},
    /* 78 N */      {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},
    /* 79 O */      {0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
    /* 80 P */      {0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x00},
    /* 81 Q */      {0x7C,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x06},
    /* 82 R */      {0xFC,0xC6,0xC6,0xFC,0xD8,0xCC,0xC6,0x00},
    /* 83 S */      {0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00},
    /* 84 T */      {0xFE,0x10,0x10,0x10,0x10,0x10,0x10,0x00},
    /* 85 U */      {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
    /* 86 V */      {0xC6,0xC6,0xC6,0x6C,0x6C,0x38,0x10,0x00},
    /* 87 W */      {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0x44,0x00},
    /* 88 X */      {0xC6,0x6C,0x38,0x10,0x38,0x6C,0xC6,0x00},
    /* 89 Y */      {0xC6,0x6C,0x38,0x10,0x10,0x10,0x10,0x00},
    /* 90 Z */      {0xFE,0x0C,0x18,0x30,0x60,0xC0,0xFE,0x00},
    /* 91 [ */      {0x7C,0x60,0x60,0x60,0x60,0x60,0x7C,0x00},
    /* 92 \ */      {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00},
    /* 93 ] */      {0x7C,0x0C,0x0C,0x0C,0x0C,0x0C,0x7C,0x00},
    /* 94 ^ */      {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00},
    /* 95 _ */      {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    /* 96 ` */      {0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00},
    /* 97 a */      {0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00},
    /* 98 b */      {0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xFC,0x00},
    /* 99 c */      {0x00,0x00,0x7C,0xC6,0xC0,0xC6,0x7C,0x00},
    /* 100 d */     {0x06,0x06,0x7E,0xC6,0xC6,0xC6,0x7E,0x00},
    /* 101 e */     {0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00},
    /* 102 f */     {0x1C,0x36,0x30,0xFC,0x30,0x30,0x30,0x00},
    /* 103 g */     {0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x7C},
    /* 104 h */     {0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x00},
    /* 105 i */     {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
    /* 106 j */     {0x0C,0x00,0x1C,0x0C,0x0C,0xCC,0xCC,0x78},
    /* 107 k */     {0xC0,0xC0,0xCC,0xD8,0xF0,0xD8,0xCC,0x00},
    /* 108 l */     {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    /* 109 m */     {0x00,0x00,0xEC,0xFE,0xD6,0xC6,0xC6,0x00},
    /* 110 n */     {0x00,0x00,0xFC,0xC6,0xC6,0xC6,0xC6,0x00},
    /* 111 o */     {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00},
    /* 112 p */     {0x00,0x00,0xFC,0xC6,0xC6,0xFC,0xC0,0xC0},
    /* 113 q */     {0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x06},
    /* 114 r */     {0x00,0x00,0xDC,0xE6,0xC0,0xC0,0xC0,0x00},
    /* 115 s */     {0x00,0x00,0x7C,0xC0,0x7C,0x06,0x7C,0x00},
    /* 116 t */     {0x30,0x30,0xFC,0x30,0x30,0x36,0x1C,0x00},
    /* 117 u */     {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x7E,0x00},
    /* 118 v */     {0x00,0x00,0xC6,0xC6,0x6C,0x38,0x10,0x00},
    /* 119 w */     {0x00,0x00,0xC6,0xD6,0xD6,0xFE,0x6C,0x00},
    /* 120 x */     {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00},
    /* 121 y */     {0x00,0x00,0xC6,0xC6,0xC6,0x7E,0x06,0x7C},
    /* 122 z */     {0x00,0x00,0xFE,0x0C,0x38,0x60,0xFE,0x00},
    /* 123 { */     {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00},
    /* 124 | */     {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
    /* 125 } */     {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00},
    /* 126 ~ */     {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00},
};

/* ============================================
 * put_pixel - 在指定坐标绘制一个像素点
 * @x:     水平坐标 (0~319)
 * @y:     垂直坐标 (0~199)
 * @color: 颜色索引 (0~255)
 * ============================================ */
void put_pixel(int x, int y, int color)
{
    /* 边界检查，防止越界写入显存 */
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
    {
        /* VGA 13h 模式下每像素 1 字节，行优先存储 */
        VGA_BASE[y * SCREEN_WIDTH + x] = (unsigned char) color;
    }
}

/* ============================================
 * draw_char - 在指定坐标绘制一个 8×8 点阵字符
 * @x:   字符左上角水平坐标
 * @y:   字符左上角垂直坐标
 * @c:   要绘制的 ASCII 字符
 * @fg:  前景色索引 (0~255)
 * @bg:  背景色索引 (0~255)
 * ============================================ */
void draw_char(int x, int y, char c, int fg, int bg)
{
    int row, col;
    const unsigned char *bitmap;

    /* 仅处理可打印字符（空格 32 ~ 126），其他字符忽略 */
    if ((unsigned char)c < 32 || (unsigned char)c > 126)
        return;

    /* 从字体位图数组中获取该字符的 8 字节行数据 */
    bitmap = font_bitmap[(unsigned char)c - 32];

    /* 逐行逐列绘制 8×8 像素块 */
    for (row = 0; row < FONT_HEIGHT; row++)
    {
        for (col = 0; col < FONT_WIDTH; col++)
        {
            /* 检查位图中该像素是否点亮（高位在左） */
            if (bitmap[row] & (0x80 >> col))
                put_pixel(x + col, y + row, fg);  /* 点亮的前景色 */
            else
                put_pixel(x + col, y + row, bg);  /* 熄灭的背景色 */
        }
    }
}

/* ============================================
 * draw_string - 在指定坐标绘制字符串
 * @x:   字符串起始水平坐标
 * @y:   字符串起始垂直坐标
 * @str: 以 '\0' 结尾的 ASCII 字符串指针
 * @fg:  前景色索引
 * @bg:  背景色索引
 * ============================================ */
void draw_string(int x, int y, const char *str, int fg, int bg)
{
    int cx = x;
    int cy = y;

    /* 逐个字符绘制，支持换行符 '\n' */
    while (*str)
    {
        if (*str == '\n')
        {
            /* 换行：重置 x 坐标，向下移动一行 */
            cx = x;
            cy += FONT_HEIGHT;
        }
        else
        {
            /* 绘制当前字符，然后 x 坐标右移 8 像素 */
            draw_char(cx, cy, *str, fg, bg);
            cx += FONT_WIDTH;
        }
        str++;
    }
}

/* ============================================
 * clear_screen - 用指定颜色填充整个屏幕
 * @color: 填充色 (0~255)
 * ============================================ */
void clear_screen(int color)
{
    int i;

    /* VGA 13h 显存共 320×200 = 64000 字节，全部写入同一种颜色 */
    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
        VGA_BASE[i] = (unsigned char) color;
    }
}

/* ============================================
 * set_video_mode - 设置视频显示模式
 * @mode: 视频模式编号（如 0x13 = 320×200 256色）
 *
 * 说明：在保护模式下，无法直接调用 BIOS INT 10h。
 * 本函数通过直接操作 VGA 硬件端口来模拟模式切换。
 * 当前仅支持模式 0x13（VGA 13h）。
 * ============================================ */
void set_video_mode(int mode)
{
    if (mode != 0x13)
        return; /* 仅支持 VGA 13h 模式 */

    /* ---------- VGA 模式 13h 寄存器编程序列 ---------- */

    /* 1. 杂项输出寄存器 — 设置时钟源 */
    outb(VGA_MISC_PORT, 0x63);

    /* 2. 复位定序器 */
    outb(VGA_SEQ_INDEX, 0x00);
    outb(VGA_SEQ_DATA,  0x03);
    outb(VGA_SEQ_INDEX, 0x01);
    outb(VGA_SEQ_DATA,  0x01);
    /* 链式模式：取消奇偶平面映射，使用线性寻址 */
    outb(VGA_SEQ_INDEX, 0x02);
    outb(VGA_SEQ_DATA,  0x0F);
    outb(VGA_SEQ_INDEX, 0x03);
    outb(VGA_SEQ_DATA,  0x00);
    outb(VGA_SEQ_INDEX, 0x04);
    outb(VGA_SEQ_DATA,  0x0E);

    /* 3. 解锁 CRTC 保护寄存器 */
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA,  inb(VGA_CRTC_DATA) & 0x7F);

    /* 4. 编程 CRT 控制器寄存器 (R00~R18) */
    static const unsigned char crtc_13h[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF
    };
    int i;
    for (i = 0; i < 25; i++)
    {
        outb(VGA_CRTC_INDEX, (unsigned char)i);
        outb(VGA_CRTC_DATA,  crtc_13h[i]);
    }

    /* 5. 编程图形控制器寄存器 */
    outb(VGA_GC_INDEX, 0x00);
    outb(VGA_GC_DATA,  0x00);
    outb(VGA_GC_INDEX, 0x01);
    outb(VGA_GC_DATA,  0x00);
    outb(VGA_GC_INDEX, 0x02);
    outb(VGA_GC_DATA,  0x00);
    outb(VGA_GC_INDEX, 0x03);
    outb(VGA_GC_DATA,  0x00);
    outb(VGA_GC_INDEX, 0x04);
    outb(VGA_GC_DATA,  0x00);
    outb(VGA_GC_INDEX, 0x05);
    outb(VGA_GC_DATA,  0x40);
    outb(VGA_GC_INDEX, 0x06);
    outb(VGA_GC_DATA,  0x05);
    outb(VGA_GC_INDEX, 0x07);
    outb(VGA_GC_DATA,  0x0F);
    outb(VGA_GC_INDEX, 0x08);
    outb(VGA_GC_DATA,  0xFF);

    /* 6. 编程属性控制器寄存器 */
    static const unsigned char ac_13h[] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
        0x41,0x00,0x0F,0x00,0x00
    };
    /* 先读取输入状态寄存器以复位属性控制器 flip-flop */
    inb(VGA_INPUT_STATUS);
    for (i = 0; i < 21; i++)
    {
        outb(VGA_AC_INDEX, (unsigned char)i);
        outb(VGA_AC_INDEX, ac_13h[i]);
    }
    /* 重新启用属性控制器显示 */
    inb(VGA_INPUT_STATUS);
    outb(VGA_AC_INDEX, 0x20);
}

/* ============================================
 * strlen_simple - 朴素计算字符串长度（无标准库依赖）
 * @str: 以 '\0' 结尾的字符串
 * 返回：字符串字符数（不含结尾 '\0'）
 * ============================================ */
int strlen_simple(const char *str)
{
    int len = 0;
    while (str[len])
        len++;
    return len;
}

/* ============================================
 * draw_rect - 绘制填充矩形
 * @x:     矩形左上角水平坐标
 * @y:     矩形左上角垂直坐标
 * @w:     矩形宽度（像素）
 * @h:     矩形高度（像素）
 * @color: 填充颜色索引 (0~255)
 * ============================================ */
void draw_rect(int x, int y, int w, int h, int color)
{
    int row, col;

    /* 逐行逐列填充矩形区域 */
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            put_pixel(x + col, y + row, color);
        }
    }
}

/* ============================================
 * draw_line - 使用 Bresenham 算法绘制直线（支持线宽）
 * @x1:    起点水平坐标
 * @y1:    起点垂直坐标
 * @x2:    终点水平坐标
 * @y2:    终点垂直坐标
 * @size:  线宽（像素，1=单像素细线, >=2=粗线）
 * @color: 颜色索引 (0~255)
 *
 * 说明：基于 Bresenham 中点画线算法。对于粗线
 *       (size > 1)，在每个线段像素处绘制一个
 *       size×size 的方形笔刷块，实现变宽效果。
 *       size 为偶数时块偏移 -0.5 像素，奇数时居中。
 * ============================================ */
void draw_line(int x1, int y1, int x2, int y2, int size, int color)
{
    int dx, dy;
    int sx, sy;          /* 步进方向 */
    int err, e2;         /* Bresenham 误差项 */
    int half;            /* 线宽笔刷半尺寸 */
    int i, j;

    /* 计算水平和垂直的绝对距离 */
    dx  = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    dy  = (y2 > y1) ? (y2 - y1) : (y1 - y2);

    /* 确定步进方向（±1） */
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;

    /* Bresenham 初始误差 */
    err = dx - dy;

    /* 线宽笔刷半尺寸：奇数居中，偶数向负方向偏移 1 */
    half = size / 2;

    /* 主循环：Bresenham 增量画线 */
    for (;;)
    {
        /* 在当前线段像素处绘制 size×size 的方形笔刷块 */
        for (j = -half; j < half + (size & 1); j++)
        {
            for (i = -half; i < half + (size & 1); i++)
            {
                put_pixel(x1 + i, y1 + j, color);
            }
        }

        /* 到达终点则退出 */
        if (x1 == x2 && y1 == y2)
            break;

        /* Bresenham 误差累积与坐标更新 */
        e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x1  += sx;    /* 水平步进 */
        }

        if (e2 < dx)
        {
            err += dx;
            y1  += sy;    /* 垂直步进 */
        }
    }
}

/* ============================================================
 *                     中断与鼠标子系统
 * ============================================================ */

/* ---------- PIC (可编程中断控制器) 端口 ---------- */
#define PIC1_CMD    0x20   /* 主 PIC 命令端口 */
#define PIC1_DATA   0x21   /* 主 PIC 数据端口 */
#define PIC2_CMD    0xA0   /* 从 PIC 命令端口 */
#define PIC2_DATA   0xA1   /* 从 PIC 数据端口 */
#define PIC_EOI     0x20   /* 中断结束命令 */

/* IRQ12 (鼠标) 在重映射后对应中断向量 */
#define IRQ_MOUSE   12

/* ---------- PS/2 控制器端口 ---------- */
#define PS2_DATA    0x60   /* 数据端口（读/写） */
#define PS2_STATUS  0x64   /* 状态端口（读）/ 命令端口（写） */
#define PS2_CMD_ENABLE_AUX  0xA8  /* 启用辅助设备（鼠标） */
#define PS2_CMD_READ_CFG    0x20  /* 读取配置字节 */
#define PS2_CMD_WRITE_CFG   0x60  /* 写入配置字节 */
#define PS2_CMD_MOUSE       0xD4  /* 下一字节发送给鼠标 */
#define MOUSE_CMD_ENABLE    0xF4  /* 启用鼠标数据报告 */
#define MOUSE_ACK           0xFA  /* 鼠标应答 ACK */

/* PS/2 状态寄存器标志 */
#define PS2_OUT_FULL   0x01  /* 输出缓冲区满（可以读） */
#define PS2_IN_FULL    0x02  /* 输入缓冲区满（不可写） */

/* ---------- IDT 描述符结构（每个 8 字节） ---------- */
typedef struct {
    unsigned short offset_low;   /* 处理函数地址低 16 位 */
    unsigned short selector;     /* 代码段选择子 (0x08) */
    unsigned char  zero;         /* 保留，必须为 0 */
    unsigned char  flags;        /* 标志: P=1, DPL=00, Type=0xE (32位中断门) */
    unsigned short offset_high;  /* 处理函数地址高 16 位 */
} __attribute__((packed)) idt_entry_t;

/* IDT 指针结构（lidt 使用） */
typedef struct {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed)) idt_ptr_t;

/* ---------- 中断描述符表（256 项，2048 字节） ---------- */
static idt_entry_t idt[256];
static idt_ptr_t   idt_ptr;

/* ---------- 鼠标状态变量 ---------- */
static int mouse_x      = 160;   /* 初始屏幕中央 X */
static int mouse_y      = 100;   /* 初始屏幕中央 Y */
static int mouse_btn    = 0;     /* 按键状态 */
static unsigned char mouse_cycle = 0;    /* 当前包字节序号 (0/1/2) */
static unsigned char mouse_packet[3];   /* 3 字节数据包缓冲 */

/* ---------- 来自 isr.asm 的外部中断桩符号 ---------- */
extern void isr_irq0(void);
extern void isr_irq1(void);
extern void isr_irq2(void);
extern void isr_irq3(void);
extern void isr_irq4(void);
extern void isr_irq5(void);
extern void isr_irq6(void);
extern void isr_irq7(void);
extern void isr_irq8(void);
extern void isr_irq9(void);
extern void isr_irq10(void);
extern void isr_irq11(void);
extern void isr_irq12(void);  /* 鼠标 */
extern void isr_irq13(void);
extern void isr_irq14(void);
extern void isr_irq15(void);

/* ============================================
 * idt_set_entry - 设置 IDT 中第 n 号中断的描述符
 * @n:        中断向量号
 * @handler:  处理函数地址
 * @selector: 代码段选择子（内核代码 = 0x08）
 * @flags:    中断门标志 (32位中断门 = 0x8E)
 * ============================================ */
static void idt_set_entry(int n, unsigned int handler,
                          unsigned short selector, unsigned char flags)
{
    idt[n].offset_low  = (unsigned short)(handler & 0xFFFF);
    idt[n].offset_high = (unsigned short)((handler >> 16) & 0xFFFF);
    idt[n].selector    = selector;
    idt[n].zero        = 0;
    idt[n].flags       = flags;
}

/* ============================================
 * init_pic - 重映射 PIC 中断向量
 *
 * 默认映射: IRQ0–7 → INT 0x08–0x0F (与 CPU 异常冲突)
 * 重映射后: IRQ0–7 → INT 0x20–0x27, IRQ8–15 → INT 0x28–0x2F
 * 鼠标 IRQ12 → INT 0x2C
 * ============================================ */
static void init_pic(void)
{
    /* 保存当前中断屏蔽字 */
    unsigned char mask1 = inb(PIC1_DATA);
    unsigned char mask2 = inb(PIC2_DATA);

    /* ICW1: 初始化 + 需要 ICW4 */
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);

    /* ICW2: 主 PIC 基向量 = 0x20, 从 PIC 基向量 = 0x28 */
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    /* ICW3: 主 PIC IRQ2 级联从 PIC, 从 PIC ID = 2 */
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    /* ICW4: 8086 模式 */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* 恢复中断屏蔽字（仅启用 IRQ1 键盘和 IRQ12 鼠标） */
    outb(PIC1_DATA, mask1 & ~0x02);   /* 键盘 IRQ1 */
    outb(PIC2_DATA, mask2 & ~0x10);   /* 鼠标 IRQ12 (从 PIC IRQ4) */
}

/* ============================================
 * init_idt - 初始化中断描述符表并加载
 *
 * 为 IRQ 0–15 安装中断门描述符，指向 isr.asm 的中断桩。
 * ============================================ */
static void init_idt(void)
{
    /* 中断桩函数指针数组（与 isr.asm 中 ISR_STUB 顺序一致） */
    static void (* const isr_stubs[])(void) = {
        isr_irq0,  isr_irq1,  isr_irq2,  isr_irq3,
        isr_irq4,  isr_irq5,  isr_irq6,  isr_irq7,
        isr_irq8,  isr_irq9,  isr_irq10, isr_irq11,
        isr_irq12, isr_irq13, isr_irq14, isr_irq15
    };
    int i;

    /* 为 IRQ 0–15 → 中断向量 0x20–0x2F 设置 IDT 条目 */
    for (i = 0; i < 16; i++)
    {
        idt_set_entry(0x20 + i, (unsigned int)isr_stubs[i],
                      0x08,      /* 内核代码段选择子 */
                      0x8E);     /* P=1, DPL=0, 32位中断门 */
    }

    /* 构造 IDT 指针并加载 */
    idt_ptr.limit = (unsigned short)(sizeof(idt) - 1);
    idt_ptr.base  = (unsigned int)&idt[0];

    __asm__ volatile ("lidt %0" :: "m"(idt_ptr));
}

/* ============================================
 * ps2_wait_read - 等待 PS/2 输出缓冲区有数据可读
 * ============================================ */
static void ps2_wait_read(void)
{
    int timeout = 100000;
    /* 轮询状态寄存器 Bit 0：1 = 可读 */
    while (--timeout && !(inb(PS2_STATUS) & PS2_OUT_FULL))
    {
        __asm__ volatile ("pause");  /* 降低 CPU 总线竞争 */
    }
}

/* ============================================
 * ps2_wait_write - 等待 PS/2 输入缓冲区空闲可写
 * ============================================ */
static void ps2_wait_write(void)
{
    int timeout = 100000;
    /* 轮询状态寄存器 Bit 1：0 = 可写 */
    while (--timeout && (inb(PS2_STATUS) & PS2_IN_FULL))
    {
        __asm__ volatile ("pause");
    }
}

/* ============================================
 * ps2_mouse_write - 向 PS/2 鼠标设备发送命令字节
 * @data: 要发送的命令
 * ============================================ */
static void ps2_mouse_write(unsigned char data)
{
    ps2_wait_write();
    outb(PS2_STATUS, PS2_CMD_MOUSE);   /* 通知控制器：下一字节是鼠标命令 */
    ps2_wait_write();
    outb(PS2_DATA, data);              /* 发送命令字节 */
}

/* ============================================
 * init_mouse - 初始化 PS/2 鼠标
 *
 * 流程：
 *   1. 启用 PS/2 辅助设备
 *   2. 读取并修改控制器配置字节（使能鼠标中断）
 *   3. 发送启用数据报告命令给鼠标
 * ============================================ */
static void init_mouse(void)
{
    unsigned char config;

    /* 1. 启用辅助 PS/2 设备（鼠标端口） */
    ps2_wait_write();
    outb(PS2_STATUS, PS2_CMD_ENABLE_AUX);

    /* 2. 读取当前控制器配置字节 */
    ps2_wait_write();
    outb(PS2_STATUS, PS2_CMD_READ_CFG);
    ps2_wait_read();
    config = inb(PS2_DATA);

    /* 3. 修改配置：启用鼠标中断 (Bit1) 和鼠标时钟 (Bit5) */
    config |= 0x02;    /* 设置 Bit1 = 启用 IRQ12 */
    config |= 0x20;    /* 设置 Bit5 = 启用鼠标时钟 */
    config &= ~0x10;   /* 清除 Bit4 禁止键盘（如果不需要键盘可禁用）... */
    config |= 0x10;    /* 重新设置 Bit4 = 也保持键盘启用 */

    ps2_wait_write();
    outb(PS2_STATUS, PS2_CMD_WRITE_CFG);
    ps2_wait_write();
    outb(PS2_DATA, config);

    /* 4. 发送"默认设置"和"启用数据报告"给鼠标 */
    ps2_mouse_write(0xF6);   /* 恢复默认设置 */
    ps2_wait_read();
    (void)inb(PS2_DATA);     /* 读取 ACK */

    ps2_mouse_write(MOUSE_CMD_ENABLE);  /* 启用数据报告 */
    ps2_wait_read();
    (void)inb(PS2_DATA);     /* 读取 ACK */

    /* 鼠标开始发送 3 字节移动数据包 */
}

/* ============================================
 * irq_handler - C 语言中断分发函数（isr.asm 中断桩调用）
 * @irq: 中断号 (0~15)
 * ============================================ */
void irq_handler(int irq)
{
    if (irq == IRQ_MOUSE)
    {
        /* ---------- 鼠标中断处理 ---------- */
        unsigned char data;

        /* 读取 PS/2 数据端口的一个字节 */
        ps2_wait_read();
        data = inb(PS2_DATA);

        /* 3 字节包的状态机 */
        switch (mouse_cycle)
        {
        case 0:
            /* 字节 1：标志字节，Bit3 必须为 1（同步位验证） */
            if (!(data & 0x08)) return;  /* 未对齐包，丢弃 */
            mouse_packet[0] = data;
            mouse_cycle = 1;
            break;

        case 1:
            /* 字节 2：X 轴移动增量（9位有符号：Bit8=符号, Bit7-0=量值） */
            mouse_packet[1] = data;
            mouse_cycle = 2;
            break;

        case 2:
        {
            /* 字节 3：Y 轴移动增量 + 更新全局坐标 */
            int dx, dy;
            mouse_packet[2] = data;
            mouse_cycle = 0;

            /*
             * PS/2 鼠标位移使用 9 位二补码（非原码+符号位）：
             *   右 +1: byte=0x01, sign=0 → 0x001 = +1
             *   左 -1: byte=0xFF, sign=1 → 0x1FF = -1 (二补码)
             *   下 +1: byte=0x01, sign=0 → 0x001 = +1
             *   上 -1: byte=0xFF, sign=1 → 0x1FF = -1 (二补码)
             *
             * 二补码符号扩展公式：若符号位=1，则 byte - 256 得到负值。
             *   例: 0xFF - 256 = -1 (而非原码的 -255)
             */
            dx = mouse_packet[1];                           /* 低 8 位 */
            if (mouse_packet[0] & 0x10) dx = dx - 256;      /* Bit4=X符号，二补码扩展 */

            dy = mouse_packet[2];                           /* 低 8 位 */
            if (mouse_packet[0] & 0x20) dy = dy - 256;      /* Bit5=Y符号，二补码扩展 */

            mouse_x += dx;
            mouse_y -= dy;  /* PS/2 Y 轴方向与屏幕坐标系相反，取反 */
            if (mouse_x < 0)               mouse_x = 0;
            if (mouse_x >= SCREEN_WIDTH)   mouse_x = SCREEN_WIDTH  - 1;
            if (mouse_y < 0)               mouse_y = 0;
            if (mouse_y >= SCREEN_HEIGHT)  mouse_y = SCREEN_HEIGHT - 1;

            /* 按键状态 */
            mouse_btn = mouse_packet[0] & 0x07;             /* Bit0=左, Bit1=右, Bit2=中 */
            break;
        }
        }
    }
}

/* ============================================
 * get_mouse_pos_x - 获取鼠标当前水平坐标
 * 返回：0~319 之间的 X 坐标
 * 用法：int mouse_pos_x; mouse_pos_x = get_mouse_pos_x();
 * ============================================ */
int get_mouse_pos_x(void)
{
    return mouse_x;
}

/* ============================================
 * get_mouse_pos_y - 获取鼠标当前垂直坐标
 * 返回：0~199 之间的 Y 坐标
 * 用法：int mouse_pos_y; mouse_pos_y = get_mouse_pos_y();
 * ============================================ */
int get_mouse_pos_y(void)
{
    return mouse_y;
}

/* ============================================
 * init_input - 输入子系统初始化（一站式调用）
 *
 * 执行顺序：
 *   1. 重映射 PIC（避免 IRQ 与 CPU 异常冲突）
 *   2. 设置 IDT（安装中断门描述符）
 *   3. 初始化 PS/2 鼠标
 *   4. 开启硬件中断（STI）
 * ============================================ */
void init_input(void)
{
    init_pic();    /* 重映射 PIC */
    init_idt();    /* 安装中断描述符表 */
    init_mouse();  /* 初始化 PS/2 鼠标 */

    /* 启用 CPU 中断响应 */
    __asm__ volatile ("sti");
}

/* ============================================
 * get_mouse_btn - 获取鼠标按键状态
 * 返回：Bit0=左键, Bit1=右键, Bit2=中键
 * ============================================ */
int get_mouse_btn(void)
{
    return mouse_btn;
}

/* ============================================================
 *                     鼠标光标绘制子系统
 * ============================================================ */

/* ---------- 光标箭头位图 (12×12 像素) ---------- */
#define CURSOR_W  12
#define CURSOR_H  12

static const unsigned char cursor_bmp[CURSOR_H][CURSOR_W] = {
    /* 1=前景(白色), 0=透明(保留背景) */
    { 1,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,1,0,0,0,0,0,0,0,0,0,0 },
    { 1,0,1,0,0,0,0,0,0,0,0,0 },
    { 1,0,0,1,0,0,0,0,0,0,0,0 },
    { 1,0,0,0,1,0,0,0,0,0,0,0 },
    { 1,0,0,0,0,1,0,0,0,0,0,0 },
    { 1,0,0,0,0,0,1,0,0,0,0,0 },
    { 1,1,1,1,1,1,1,1,0,0,0,0 },
    { 1,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,1,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,1,1,0,0,0,0,0,0,0,0 },
};

/* 光标状态：旧位置、背景保存缓冲、可见标记 */
static int cursor_old_x    = 0;
static int cursor_old_y    = 0;
static int cursor_visible  = 0;   /* 0=尚未绘制, 1=已绘制 */
static unsigned char cursor_bg[CURSOR_H][CURSOR_W];  /* 光标下方原始像素 */

/* ============================================
 * update_mouse_cursor - 刷新鼠标光标（擦旧绘新）
 *
 * 每帧调用：擦除旧光标位置的像素（恢复背景），
 *           保存新光标位置的背景，绘制箭头形状。
 * 若位置未变动则跳过，避免重复擦写。
 * ============================================ */
void update_mouse_cursor(void)
{
    int x = mouse_x;
    int y = mouse_y;
    int row, col;

    /* 若光标未移动且已绘制，跳过 */
    if (cursor_visible && x == cursor_old_x && y == cursor_old_y)
        return;

    /* 1. 擦除旧光标：将保存的背景像素写回旧位置 */
    if (cursor_visible)
    {
        for (row = 0; row < CURSOR_H; row++)
        {
            for (col = 0; col < CURSOR_W; col++)
            {
                int px = cursor_old_x + col;
                int py = cursor_old_y + row;
                if (px >= 0 && px < SCREEN_WIDTH &&
                    py >= 0 && py < SCREEN_HEIGHT)
                {
                    VGA_BASE[py * SCREEN_WIDTH + px] = cursor_bg[row][col];
                }
            }
        }
    }

    /* 2. 保存新光标位置的背景并绘制箭头 */
    for (row = 0; row < CURSOR_H; row++)
    {
        for (col = 0; col < CURSOR_W; col++)
        {
            int px = x + col;
            int py = y + row;
            if (px >= 0 && px < SCREEN_WIDTH &&
                py >= 0 && py < SCREEN_HEIGHT)
            {
                /* 保存原始背景像素 */
                cursor_bg[row][col] = VGA_BASE[py * SCREEN_WIDTH + px];

                /* 仅绘制位图中标记为 1 的前景像素（白色） */
                if (cursor_bmp[row][col])
                    VGA_BASE[py * SCREEN_WIDTH + px] = 15;
            }
        }
    }

    /* 记录新位置 */
    cursor_old_x = x;
    cursor_old_y = y;
    cursor_visible = 1;
}
