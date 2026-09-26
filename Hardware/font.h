/*
 * @Author       : WXj
 * @Date         : 2026-09-26 16:54:05
 * @LastEditors  : Wangxiaojie
 * @LastEditTime : 2026-09-26 17:05:36
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\Hardware\font.h
 */
#ifndef __FONT_H
#define __FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * ASCII font
 *
 * Character range : 0x20 (' ') ~ 0x7E ('~')
 * Glyph size      : 8 x 16 pixels
 * Storage         : 16 bytes / glyph, one byte per row
 * Bit order       : bit7 = left-most pixel, bit0 = right-most
 * ============================================================ */

#define FONT_ASCII_FIRST          0x20U
#define FONT_ASCII_LAST           0x7EU
#define FONT_ASCII_COUNT          95U
#define FONT_ASCII_WIDTH          8U
#define FONT_ASCII_HEIGHT         16U

extern const uint8_t ASCII8x16[FONT_ASCII_COUNT][FONT_ASCII_HEIGHT];


/* ============================================================
 * Chinese font
 *
 * Glyph size      : 16 x 16 pixels
 * Storage         : 32 bytes / glyph
 *
 * Every row occupies two bytes:
 *     data[row * 2 + 0] : x = 0  ~ 7
 *     data[row * 2 + 1] : x = 8  ~ 15
 *
 * Bit order:
 *     bit7 = left-most pixel in that byte
 *
 * unicode is the Unicode BMP code point, e.g.
 *     '距' = U+8DDD
 * ============================================================ */

#define FONT_CHINESE_WIDTH        16U
#define FONT_CHINESE_HEIGHT       16U
#define FONT_CHINESE_BYTES        32U

typedef struct
{
    uint16_t unicode;
    uint8_t  data[FONT_CHINESE_BYTES];
} ChineseFont16;

extern const ChineseFont16 Chinese16_Table[];
extern const uint16_t Chinese16_Count;


/* ============================================================
 * Font access functions
 * ============================================================ */

/**
 * @brief Get the bitmap of one printable ASCII character.
 *
 * @param ch Printable ASCII character, normally ' ' ~ '~'.
 *
 * @return Pointer to 16-byte glyph bitmap.
 *         Unsupported character automatically falls back to '?'.
 */
const uint8_t *Font_GetASCII8x16(char ch);


/**
 * @brief Find one 16x16 Chinese glyph by Unicode code point.
 *
 * @param unicode Unicode BMP code point.
 *
 * @return Pointer to the 32-byte glyph bitmap.
 *         Returns 0 if this character is not present in the table.
 */
const uint8_t *Font_GetChinese16(uint16_t unicode);


/**
 * @brief Test whether a 16x16 Chinese glyph exists.
 *
 * @param unicode Unicode BMP code point.
 *
 * @return 1 if present, otherwise 0.
 */
uint8_t Font_HasChinese16(uint16_t unicode);


#ifdef __cplusplus
}
#endif

#endif /* __FONT_H */
