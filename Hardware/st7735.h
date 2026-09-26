/*
 * @Author       : WXj
 * @Date         : 2026-09-25 23:26:13
 * @LastEditors  : Wangxiaojie
 * @LastEditTime : 2026-09-26 22:41:46
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\Hardware\st7735.h
 */
#ifndef __ST7735_H
#define __ST7735_H

#include <stdint.h>

#define LCD_WIDTH    160
#define LCD_HEIGHT   128

#define LCD_X_OFFSET  0
#define LCD_Y_OFFSET  0

/* Portrait panel profile. Adjust only if the module requires it. */
#define LCD_MADCTL          0xA8
#define LCD_INVERT_COLORS   0
#define LCD_BL_ACTIVE_HIGH  1




#define LCD_BLACK       0x0000
#define LCD_WHITE       0xFFFF
#define LCD_RED         0xF800
#define LCD_GREEN       0x07E0
#define LCD_BLUE        0x001F
#define LCD_YELLOW      0xFFE0
#define LCD_CYAN        0x07FF
#define LCD_MAGENTA     0xF81F
#define LCD_GRAY        0x8410




void LCD_Init(void);

void LCD_WriteCmd(uint8_t cmd);

void LCD_WriteData8(uint8_t data);

void LCD_WriteData16(uint16_t data);

void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

void LCD_Fill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

void LCD_Clear(uint16_t color);

void LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bgcolor, uint8_t size);


void LCD_ShowChinese(uint16_t x, uint16_t y, uint16_t ch, uint16_t color, uint16_t bgcolor, uint8_t size);

void LCD_ShowStringUTF8(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bgcolor, uint8_t size);


#endif
