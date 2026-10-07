/*
 * @Author       : thuanngo
 * @Date         : 2026-09-25 23:25:49
 * @LastEditors  : thuanngo
 * @LastEditTime : 2026-09-29 19:52:58
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\Hardware\st7735.c
 */
/*
 * @Author       : thuanngo
 * @Date         : 2026-09-25 23:25:49
 * @LastEditors  : thuanngo
 * @LastEditTime : 2026-09-26 22:39:22
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\Hardware\st7735.c
 */

#include "font.h"
#include "st7735.h"

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "Delay.h"


#define LCD_CS_low()     GPIO_ResetBits(GPIOB, GPIO_Pin_12)  // 片选有效，开始一次 LCD 传输
#define LCD_CS_high()    GPIO_SetBits(GPIOB, GPIO_Pin_12)

#define LCD_DC_low()     GPIO_ResetBits(GPIOB, GPIO_Pin_13)  // DC 低电平表示发送命令
#define LCD_DC_high()    GPIO_SetBits(GPIOB, GPIO_Pin_13)

#define LCD_RST_low()    GPIO_ResetBits(GPIOB, GPIO_Pin_14)  // 复位有效
#define LCD_RST_high()   GPIO_SetBits(GPIOB, GPIO_Pin_14)

#if LCD_BL_ACTIVE_HIGH
#define LCD_BL_off()     GPIO_ResetBits(GPIOB, GPIO_Pin_10)
#define LCD_BL_on()      GPIO_SetBits(GPIOB, GPIO_Pin_10)
#else
#define LCD_BL_off()     GPIO_SetBits(GPIOB, GPIO_Pin_10)
#define LCD_BL_on()      GPIO_ResetBits(GPIOB, GPIO_Pin_10)
#endif


static void LCD_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                          RCC_APB2Periph_AFIO | RCC_APB2Periph_SPI1, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SPI1, DISABLE);

    /* 配置输出模式前先写入空闲电平，避免 LCD 在上电瞬间误收数据。 */
    LCD_CS_high();
    LCD_DC_high();
    LCD_RST_high();
    LCD_BL_off();

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					// PA5 为 SCK，PA7 为 MOSI，均配置为 SPI 复用推挽输出
    /* LCD 只写不读，但全双工 SPI 每次发送仍会收到一个占位字节，
       因此将未接出的 MISO（PA6）配置为确定状态的下拉输入。 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    SPI_I2S_DeInit(SPI1);
    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);


}

void SPI_Start(void)
{
    /* 拉低片选，选中 LCD。 */
    LCD_CS_low();
}

void SPI_Stop(void)
{
    /* 拉高片选，结束当前 LCD 传输。 */
    LCD_CS_high();
}
uint8_t SPI_SwapByte(uint8_t data)
{ 
    /* 发送一个字节，并等待对应的接收字节完成。 */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);
    /* RXNE 表示 8 个时钟已经完成；仅检查 BSY 可能早于本次传输启动。 */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

void LCD_Init(void)
{
    /* 初始化 SPI 和控制引脚，然后按 ST7735 的上电时序启动 LCD。 */
    LCD_SPI_Init();
    Delay_ms(20);
    LCD_RST_low();
    Delay_ms(20);
    LCD_RST_high();
    Delay_ms(120);

    LCD_WriteCmd(0x01); // 软件复位
    Delay_ms(150);
    LCD_WriteCmd(0x11); // 退出睡眠模式
    Delay_ms(120);

    /* 常见 ST7735 128x160 面板的电源、帧率和 VCOM 参数。 */
    LCD_WriteCmd(0xB1);
    LCD_WriteData8(0x01); LCD_WriteData8(0x2C); LCD_WriteData8(0x2D);
    LCD_WriteCmd(0xB2);
    LCD_WriteData8(0x01); LCD_WriteData8(0x2C); LCD_WriteData8(0x2D);
    LCD_WriteCmd(0xB3);
    LCD_WriteData8(0x01); LCD_WriteData8(0x2C); LCD_WriteData8(0x2D);
    LCD_WriteData8(0x01); LCD_WriteData8(0x2C); LCD_WriteData8(0x2D);
    LCD_WriteCmd(0xB4); LCD_WriteData8(0x07);
    LCD_WriteCmd(0xC0);
    LCD_WriteData8(0xA2); LCD_WriteData8(0x02); LCD_WriteData8(0x84);
    LCD_WriteCmd(0xC1); LCD_WriteData8(0xC5);
    LCD_WriteCmd(0xC2); LCD_WriteData8(0x0A); LCD_WriteData8(0x00);
    LCD_WriteCmd(0xC3); LCD_WriteData8(0x8A); LCD_WriteData8(0x2A);
    LCD_WriteCmd(0xC4); LCD_WriteData8(0x8A); LCD_WriteData8(0xEE);
    LCD_WriteCmd(0xC5); LCD_WriteData8(0x0E);

    LCD_WriteCmd(0x3A);
    LCD_WriteData8(0x05); // 参数：RGB565 像素格式，此处必须作为数据发送
    LCD_WriteCmd(0x36);
    LCD_WriteData8(LCD_MADCTL);
#if LCD_INVERT_COLORS
    LCD_WriteCmd(0x21);
#else
    LCD_WriteCmd(0x20);
#endif
    LCD_WriteCmd(0x13); // 切换到正常显示模式
    Delay_ms(10);
    LCD_Clear(LCD_BLACK);
    LCD_WriteCmd(0x29); // 打开显示
    Delay_ms(100);
    LCD_BL_on();
}

void LCD_WriteCmd(uint8_t cmd)
{
    /* DC 低电平时，SPI 字节会被 ST7735 解释为控制命令。 */
    LCD_DC_low();
    SPI_Start();
    SPI_SwapByte(cmd);
    SPI_Stop();
}

void LCD_WriteData8(uint8_t data)
{
    /* DC 高电平时，SPI 字节会被解释为命令参数或像素数据。 */
    LCD_DC_high();
    SPI_Start();
    SPI_SwapByte(data);
    SPI_Stop();
}

void LCD_WriteData16(uint16_t data)
{ 
    /* RGB565 按高字节在前的顺序发送一个像素。 */
    LCD_DC_high();
    SPI_Start();
    SPI_SwapByte(data >> 8);
    SPI_SwapByte(data & 0xFF);
    SPI_Stop();
}

void LCD_SetAddressWindow(
    uint16_t x1,
    uint16_t y1,
    uint16_t x2,
    uint16_t y2
)
{
    /* 加上面板偏移，并设置后续像素数据要写入的矩形区域。 */
    x1 += LCD_X_OFFSET;
    x2 += LCD_X_OFFSET;
    y1 += LCD_Y_OFFSET;
    y2 += LCD_Y_OFFSET;
    /* 设置列地址范围。 */
    LCD_WriteCmd(0x2A);

    LCD_WriteData8((uint8_t)(x1 >> 8));
    LCD_WriteData8((uint8_t)x1);

    LCD_WriteData8((uint8_t)(x2 >> 8));
    LCD_WriteData8((uint8_t)x2);


    /* 设置行地址范围。 */
    LCD_WriteCmd(0x2B);

    LCD_WriteData8((uint8_t)(y1 >> 8));
    LCD_WriteData8((uint8_t)y1);

    LCD_WriteData8((uint8_t)(y2 >> 8));
    LCD_WriteData8((uint8_t)y2);

    /* 后续数据写入显存。 */
    LCD_WriteCmd(0x2C);
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    /* 坐标超出屏幕时直接忽略，避免产生无效的 LCD 地址。 */
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
    {
        return;
    }
    LCD_SetAddressWindow(x, y, x, y);
    LCD_WriteData16(color);
}

void LCD_Fill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    uint32_t count;
    /* x1、y1 是包含在内的终点坐标，先检查整个矩形是否合法。 */
    if (x0 > x1 || y0 > y1 || x1 >= LCD_WIDTH || y1 >= LCD_HEIGHT )
        {
            return;
        }
    LCD_SetAddressWindow(x0, y0, x1, y1);
    count = (uint32_t)(x1 - x0 + 1) * (y1 - y0 + 1);
    LCD_DC_high();
    LCD_CS_low();
    /* 保持片选有效，连续发送矩形区域内的全部 RGB565 像素。 */
    while (count--)
    {
        SPI_SwapByte((uint8_t)(color >> 8));
        SPI_SwapByte((uint8_t)color);
    }
    LCD_CS_high();

}

void LCD_Clear(uint16_t color)
{
    /* 用指定颜色填充整个屏幕。 */
    LCD_Fill(0 , 0 , LCD_WIDTH - 1 , LCD_HEIGHT - 1 , color);
}



void LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bgcolor, uint8_t size)
{
    uint8_t row, col;
    uint8_t repeat_x, repeat_y;
    uint8_t data;
    uint8_t index;

    /* ASCII 字库为 8x16，每个字形像素可按 size 倍整数放大。 */
    if (size == 0 || (uint32_t)x + 8U * size > LCD_WIDTH ||
        (uint32_t)y + 16U * size > LCD_HEIGHT)
    {
        return;
    }
    if (ch < 32 || ch > 126)
    {
        ch = '?';
    }

    index = ch - 32;
    LCD_SetAddressWindow(x,y, x + 8 * size - 1, y + 16 * size - 1);
    LCD_DC_high();
    LCD_CS_low();

    for (row = 0; row < 16; row++)
    {
        data = ASCII8x16[index][row];
        for (repeat_y = 0; repeat_y < size; repeat_y++)
        {
            for (col = 0; col < 8; col++)
            {
                uint16_t pixel = (data & (0x80 >> col)) ? color : bgcolor;
                for (repeat_x = 0; repeat_x < size; repeat_x++)
                {
                    SPI_SwapByte((uint8_t)(pixel >> 8));
                    SPI_SwapByte((uint8_t)pixel);
                }
            }
        }
    }

    LCD_CS_high();


}

void LCD_ShowChinese(uint16_t x, uint16_t y, uint16_t unicode, uint16_t color, uint16_t bgcolor, uint8_t size)
{
   const uint8_t *font;

   uint8_t row, col;
   uint8_t repeat_x, repeat_y;
   uint16_t data;

    /* 中文字库为 16x16，unicode 是该字符的 Unicode 编码。 */
    if (size == 0 || (uint32_t)x + 16U * size > LCD_WIDTH ||
       (uint32_t)y + 16U * size > LCD_HEIGHT)
   {
       return;
   }
   font = Font_GetChinese16(unicode);

    /* 字库中没有该字符时不绘制，避免访问空指针。 */
    if (font == 0)
   {
       return;
   }

    LCD_SetAddressWindow(x, y, x + 16 * size - 1, y + 16 * size - 1);
    LCD_DC_high();
    LCD_CS_low();

    for (row = 0; row < 16; row++)
    {
        data = (font[row * 2] << 8) | font[row * 2 + 1];
        for (repeat_y = 0; repeat_y < size; repeat_y++)
        {
            for (col = 0; col < 16; col++)
            {
                uint16_t pixel = (data & (0x8000 >> col)) ? color : bgcolor;
                for (repeat_x = 0; repeat_x < size; repeat_x++)
                {
                    SPI_SwapByte((uint8_t)(pixel >> 8));
                    SPI_SwapByte((uint8_t)pixel);
                }
            }
        }
    }
    LCD_CS_high();

}

static uint16_t UTF8_GetUnicode(
    const uint8_t *str,
    uint8_t *length
)
{
    uint16_t unicode;

    /* ASCII 字符使用单字节编码。 */
    if(str[0] < 0x80)
    {
        *length = 1;

        return str[0];
    }
    /* 常用中文基本属于 UTF-8 三字节编码，转换为 Unicode 编码点。 */
    if(
        (str[0] & 0xF0) ==
        0xE0 && str[1] != 0 && (str[1] & 0xC0) == 0x80 &&
        str[2] != 0 && (str[2] & 0xC0) == 0x80
    )
    {
        unicode =
            ((uint16_t)(str[0] & 0x0F) << 12) |
            ((uint16_t)(str[1] & 0x3F) << 6) |
            ((uint16_t)(str[2] & 0x3F));

        *length = 3;

        return unicode;
    }


    /* 编码格式无效时跳过当前字节，并用问号替代。 */
    *length = 1;

    return '?';
}

void LCD_ShowStringUTF8(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bgcolor, uint8_t size)
{
    const uint8_t *p;
    uint16_t unicode;

    uint8_t length;
    uint16_t width;

    /* 根据字符类型选择 8x16 ASCII 或 16x16 中文字形，并自动换行。 */
    if (str == 0 || size == 0 || 16U * size > LCD_HEIGHT ||
        16U * size > LCD_WIDTH || x >= LCD_WIDTH || y >= LCD_HEIGHT)
    {
        return;
    }

    p = (const uint8_t *) str;

    while (*p != '\0')
    {
        unicode = UTF8_GetUnicode(p, &length);
        /* 中文占 16 列，ASCII 占 8 列；不足一行时从下一行开始。 */
        width = (unicode <= 0x7F ? 8U : 16U) * size;
        if ((uint32_t)x + width > LCD_WIDTH)
        {
            x = 0;
            y += 16U * size;
        }
        if ((uint32_t)y + 16U * size > LCD_HEIGHT)
        {
            break;
        }
        if (unicode <= 0x7F)
        {
            LCD_ShowChar(x, y, (char)unicode, color, bgcolor, size);
        }
        else
        {
            LCD_ShowChinese(x, y, unicode, color, bgcolor, size);

        }
        p += length;
        x += width;
    }
}
