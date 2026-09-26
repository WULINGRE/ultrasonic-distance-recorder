/* Host regression test: compile the real driver against a recording SPI/GPIO
   adapter. No MCU, electrical timing, or physical LCD is simulated. */
#include "stm32f10x.h"
#include "st7735.h"
#include "font.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
#define RECORD_CHECK(expr) do { if (!(expr) && !fault) fault = __LINE__; } while (0)
GPIO_TypeDef test_gpio_a, test_gpio_b;
SPI_TypeDef test_spi;
static unsigned clocks, pb, configured_a, configured_b, pending, rx_checked;
static unsigned elapsed, reset_low_at, reset_high_at, saw_reset, enabled;
static unsigned count;
static int fault;
static struct { uint8_t data, dc; unsigned ms; } bytes[50000];

void RCC_APB2PeriphClockCmd(unsigned mask, int enable) { if (enable) clocks |= mask; }
void GPIO_PinRemapConfig(unsigned remap, int enable) { RECORD_CHECK(remap == GPIO_Remap_SPI1 && !enable); }
void GPIO_SetBits(GPIO_TypeDef *port, uint16_t pins)
{
    if (port != GPIOB) return;
    if ((pins & GPIO_Pin_14) && saw_reset) {
        RECORD_CHECK(elapsed - reset_low_at >= 10);
        reset_high_at = elapsed;
    }
    if (pins & GPIO_Pin_10) RECORD_CHECK(count > 40000);
    pb |= pins;
}
void GPIO_ResetBits(GPIO_TypeDef *port, uint16_t pins)
{
    if (port != GPIOB) return;
    pb &= ~pins;
    if (pins & GPIO_Pin_14) { saw_reset = 1; reset_low_at = elapsed; }
}
void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    if (port == GPIOA) {
        RECORD_CHECK(clocks & RCC_APB2Periph_GPIOA);
        if (init->GPIO_Mode == GPIO_Mode_AF_PP) configured_a |= init->GPIO_Pin;
    } else {
        RECORD_CHECK(clocks & RCC_APB2Periph_GPIOB);
        RECORD_CHECK(init->GPIO_Mode == GPIO_Mode_Out_PP);
        configured_b |= init->GPIO_Pin;
    }
}
void SPI_I2S_DeInit(SPI_TypeDef *spi) { (void)spi; pending = 0; }
void SPI_StructInit(SPI_InitTypeDef *init) { init->SPI_CRCPolynomial = 7; }
void SPI_Init(SPI_TypeDef *spi, SPI_InitTypeDef *init)
{
    (void)spi;
    RECORD_CHECK(init->SPI_CRCPolynomial == 7);
    RECORD_CHECK(init->SPI_DataSize == 8 && init->SPI_NSS == SPI_NSS_Soft);
    RECORD_CHECK(init->SPI_CPOL == 0 && init->SPI_CPHA == 0);
}
void SPI_Cmd(SPI_TypeDef *spi, int enable) { (void)spi; enabled = enable; }
int SPI_I2S_GetFlagStatus(SPI_TypeDef *spi, unsigned flag)
{
    (void)spi;
    if (flag == SPI_I2S_FLAG_RXNE) { rx_checked = 1; return pending != 0; }
    if (flag == SPI_I2S_FLAG_BSY) { RECORD_CHECK(rx_checked); return RESET; }
    return SET;
}
void SPI_I2S_SendData(SPI_TypeDef *spi, uint16_t data)
{
    (void)spi;
    RECORD_CHECK(enabled && !pending);
    RECORD_CHECK((configured_a & (GPIO_Pin_5 | GPIO_Pin_7)) == (GPIO_Pin_5 | GPIO_Pin_7));
    RECORD_CHECK((configured_b & (GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_10)) ==
                                (GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_10));
    RECORD_CHECK(!(pb & GPIO_Pin_12) && (pb & GPIO_Pin_14));
    RECORD_CHECK(saw_reset && elapsed - reset_high_at >= 120);
    RECORD_CHECK(count < 50000);
    if (count < 50000) {
        bytes[count].data = (uint8_t)data;
        bytes[count].dc = !!(pb & GPIO_Pin_13);
        bytes[count].ms = elapsed;
        count++;
    }
    pending = 1;
    rx_checked = 0;
}
uint16_t SPI_I2S_ReceiveData(SPI_TypeDef *spi)
{
    (void)spi;
    RECORD_CHECK(pending && rx_checked);
    pending = 0;
    return 0;
}
void Delay_ms(uint32_t ms) { elapsed += ms; }

__declspec(dllexport) int LCD_RunTests(void)
{
    unsigned i, found_color = 0, found_sleep = 0, found_display = 0;
    unsigned x, y, source, pixel_index;
    uint16_t expected;
    LCD_Init();
    CHECK(!fault && !pending && (pb & GPIO_Pin_10) && (pb & GPIO_Pin_12));
    CHECK(bytes[0].dc == 0 && bytes[0].data == 0x01);
    for (i = 0; i < count; i++) {
        if (bytes[i].dc) continue;
        if (bytes[i].data == 0x3A) {
            CHECK(i + 1 < count && bytes[i + 1].dc && bytes[i + 1].data == 0x05);
            found_color++;
        }
        if (bytes[i].data == 0x11) {
            CHECK(bytes[i].ms - bytes[0].ms >= 150);
            CHECK(bytes[i + 1].ms - bytes[i].ms >= 120);
            found_sleep++;
        }
        if (bytes[i].data == 0x29) found_display++;
        CHECK(bytes[i].data != 0x05);
    }
    CHECK(found_color == 1 && found_sleep == 1 && found_display == 1);

    count = 0;
    LCD_Clear(LCD_BLUE);
    CHECK(count == 11 + 128 * 160 * 2);
    CHECK(bytes[0].data == 0x2A && !bytes[0].dc);
    CHECK(bytes[4].data == 127 && bytes[9].data == 159);
    CHECK(bytes[10].data == 0x2C && !bytes[10].dc);
    for (i = 11; i < count; i += 2) {
        CHECK(bytes[i].dc && bytes[i + 1].dc);
        CHECK(bytes[i].data == 0 && bytes[i + 1].data == 0x1F);
    }
    count = 0;
    LCD_Fill(2,3,4,6,LCD_RED);
    CHECK(count == 11 + 3 * 4 * 2);
    CHECK(bytes[2].data == 2 && bytes[4].data == 4);
    CHECK(bytes[7].data == 3 && bytes[9].data == 6);
    count = 0;
    LCD_Fill(4,3,2,6,LCD_RED);
    LCD_DrawPixel(128,0,LCD_RED);
    LCD_ShowChar(0,0,'A',LCD_RED,LCD_BLUE,0);
    LCD_ShowChinese(127,159,Chinese16_Table[0].unicode,LCD_RED,LCD_BLUE,1);
    CHECK(count == 0);

    LCD_ShowChar(0,0,'A',LCD_RED,LCD_BLUE,2);
    CHECK(count == 11 + 16 * 32 * 2);
    for (y = 0; y < 32; y++) for (x = 0; x < 16; x++) {
        source = ASCII8x16['A' - 32][y / 2];
        expected = (source & (0x80 >> (x / 2))) ? LCD_RED : LCD_BLUE;
        pixel_index = 11 + (y * 16 + x) * 2;
        CHECK(bytes[pixel_index].data == (expected >> 8));
        CHECK(bytes[pixel_index + 1].data == (expected & 255));
    }
    count = 0;
    LCD_ShowChinese(0,0,Chinese16_Table[0].unicode,LCD_RED,LCD_BLUE,2);
    CHECK(count == 11 + 32 * 32 * 2);
    count = 0;
    LCD_ShowStringUTF8(120,144,"AB",LCD_RED,LCD_BLUE,1);
    CHECK(count == 11 + 8 * 16 * 2); /* Last cell fits; next row does not. */
    count = 0;
    LCD_ShowStringUTF8(120,0,"AB",LCD_RED,LCD_BLUE,1);
    CHECK(count == 2 * (11 + 8 * 16 * 2));
    CHECK(bytes[267 + 2].data == 0 && bytes[267 + 7].data == 16);
    count = 0;
    LCD_ShowStringUTF8(0,0,"\xE4",LCD_RED,LCD_BLUE,1);
    CHECK(count == 11 + 8 * 16 * 2);
    CHECK(!fault && !pending && (pb & GPIO_Pin_12));
    return 0;
}

__declspec(dllexport) int LCD_GetMockFault(void) { return fault; }
