/* Author: thuanngo */
#ifndef LCD_TEST_STM32_H
#define LCD_TEST_STM32_H
#include <stdint.h>
typedef struct { unsigned id; } GPIO_TypeDef;
typedef struct { unsigned id; } SPI_TypeDef;
extern GPIO_TypeDef test_gpio_a, test_gpio_b;
extern SPI_TypeDef test_spi;
#define GPIOA (&test_gpio_a)
#define GPIOB (&test_gpio_b)
#define SPI1 (&test_spi)
#define RESET 0
#define SET 1
#define ENABLE 1
#define DISABLE 0
#define GPIO_Pin_5 (1U << 5)
#define GPIO_Pin_6 (1U << 6)
#define GPIO_Pin_7 (1U << 7)
#define GPIO_Pin_10 (1U << 10)
#define GPIO_Pin_12 (1U << 12)
#define GPIO_Pin_13 (1U << 13)
#define GPIO_Pin_14 (1U << 14)
#define GPIO_Mode_Out_PP 1
#define GPIO_Mode_AF_PP 2
#define GPIO_Mode_IPD 3
#define GPIO_Speed_50MHz 50
#define GPIO_Remap_SPI1 1
#define RCC_APB2Periph_GPIOA 1
#define RCC_APB2Periph_GPIOB 2
#define RCC_APB2Periph_AFIO 4
#define RCC_APB2Periph_SPI1 8
#define SPI_Direction_2Lines_FullDuplex 0
#define SPI_Mode_Master 1
#define SPI_DataSize_8b 8
#define SPI_FirstBit_MSB 0
#define SPI_CPOL_Low 0
#define SPI_CPHA_1Edge 0
#define SPI_NSS_Soft 1
#define SPI_BaudRatePrescaler_64 64
#define SPI_I2S_FLAG_TXE 1
#define SPI_I2S_FLAG_RXNE 2
#define SPI_I2S_FLAG_BSY 4
typedef struct {
    uint16_t GPIO_Pin;
    unsigned GPIO_Mode, GPIO_Speed;
} GPIO_InitTypeDef;
typedef struct {
    unsigned SPI_Direction, SPI_Mode, SPI_DataSize, SPI_FirstBit;
    unsigned SPI_CPOL, SPI_CPHA, SPI_NSS, SPI_BaudRatePrescaler, SPI_CRCPolynomial;
} SPI_InitTypeDef;
void RCC_APB2PeriphClockCmd(unsigned mask, int enable);
void GPIO_PinRemapConfig(unsigned remap, int enable);
void GPIO_SetBits(GPIO_TypeDef *port, uint16_t pins);
void GPIO_ResetBits(GPIO_TypeDef *port, uint16_t pins);
void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
void SPI_I2S_DeInit(SPI_TypeDef *spi);
void SPI_StructInit(SPI_InitTypeDef *init);
void SPI_Init(SPI_TypeDef *spi, SPI_InitTypeDef *init);
void SPI_Cmd(SPI_TypeDef *spi, int enable);
int SPI_I2S_GetFlagStatus(SPI_TypeDef *spi, unsigned flag);
void SPI_I2S_SendData(SPI_TypeDef *spi, uint16_t data);
uint16_t SPI_I2S_ReceiveData(SPI_TypeDef *spi);
#endif
