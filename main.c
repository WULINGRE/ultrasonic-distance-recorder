/*
 * @Author       : WXj
 * @Date         : 2026-09-25 23:00:11
 * @LastEditors  : Wangxiaojie
 * @LastEditTime : 2026-09-26 18:44:06
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\main.c
 */
#include "stm32f10x.h"                  // Device header
#include "st7735.h"
#include "Delay.h"

int main(void)
{
	uint8_t heartbeat = 0;
	LCD_Init();
	/* Power-on color test; each solid frame remains visible for 300 ms. */
	LCD_Clear(LCD_RED);
	Delay_ms(300);
	LCD_Clear(LCD_GREEN);
	Delay_ms(300);
	LCD_Clear(LCD_BLUE);
	Delay_ms(300);
	LCD_Clear(LCD_WHITE);
	Delay_ms(300);
	LCD_Clear(LCD_BLUE);
	LCD_ShowChar(8,8,'A',LCD_RED,LCD_BLUE,1);
	LCD_ShowStringUTF8(8,32,"1234567890",LCD_GREEN,LCD_BLUE,1);
	LCD_ShowStringUTF8(8,56,"ST7735 OK",LCD_WHITE,LCD_BLUE,1);
	LCD_Fill(8,88,39,111,LCD_RED);
	LCD_Fill(48,88,79,111,LCD_GREEN);
	LCD_Fill(88,88,119,111,LCD_BLUE);
	/* One-pixel border exposes incorrect panel address offsets. */
	LCD_Fill(0,0,LCD_WIDTH-1,0,LCD_WHITE);
	LCD_Fill(0,LCD_HEIGHT-1,LCD_WIDTH-1,LCD_HEIGHT-1,LCD_WHITE);
	LCD_Fill(0,0,0,LCD_HEIGHT-1,LCD_WHITE);
	LCD_Fill(LCD_WIDTH-1,0,LCD_WIDTH-1,LCD_HEIGHT-1,LCD_WHITE);
	
	while(1)
	{
		heartbeat ^= 1;
		LCD_Fill(112,140,119,147,heartbeat ? LCD_YELLOW : LCD_BLUE);
		Delay_ms(500);
	}
	
}
