// 字体操作函数实现

#include "font.h"

// 获取字符的字形数据
const uint8_t* font_get_glyph(uint8_t c) {
    if (c >= FONT_CHARS) {
        c = '?'; // 未知字符显示为问号
    }
    return font_8x8[c];
}

// 获取字体宽度
uint8_t font_get_width(void) {
    return FONT_WIDTH;
}

// 获取字体高度
uint8_t font_get_height(void) {
    return FONT_HEIGHT;
}

