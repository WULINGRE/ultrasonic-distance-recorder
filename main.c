/*
 * @Author       : WXj
 * @Date         : 2026-09-25 23:00:11
 * @LastEditors  : Wangxiaojie
 * @LastEditTime : 2026-09-26 22:21:54
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\main.c
 */
#include "stm32f10x.h"                  // Device header
#include "st7735.h"
#include "Delay.h"

int main(void)
{
	
	LCD_Init();
	/* Power-on color test; each solid frame remains visible for 300 ms. */
	LCD_Clear(LCD_RED);
	Delay_ms(300);
	LCD_Clear(LCD_WHITE);
	Delay_ms(300);
	LCD_ShowStringUTF8(0, 0, "Hello, 超声波!", LCD_BLUE, LCD_WHITE, 1);
	LCD_ShowChar(0, 16, 'W', LCD_BLUE, LCD_WHITE, 1);
	LCD_ShowChinese(0, 32, 0x8D85, LCD_BLUE, LCD_WHITE, 1);
	LCD_ShowStringUTF8(0, 48, "STM32F103C8T6", LCD_BLUE, LCD_WHITE, 1);
	
	while(1)
	{
		
	}
	
}
