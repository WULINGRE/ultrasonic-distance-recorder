/*
 * @Author       : thuanngo
 * @Date         : 2026-09-26 22:50:58
 * @LastEditors  : thuanngo
 * @LastEditTime : 2026-09-29 19:45:16
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\Hardware\ultrasonic.h
 */
# ifndef __ultrasonic_H__
# define __ultrasonic_H__ 

# include "stm32f10x.h"
# include <stdint.h>
# include "Timer.h"
# include "Delay.h"

/*========================================================
 * CS100A Hardware Definition
 *========================================================*/

#define CS100A_TRIG_GPIO        GPIOB
#define CS100A_TRIG_PIN         GPIO_Pin_0
#define CS100A_TRIG_RCC         RCC_APB2Periph_GPIOB

#define CS100A_ECHO_GPIO        GPIOA
#define CS100A_ECHO_PIN         GPIO_Pin_0
#define CS100A_ECHO_RCC         RCC_APB2Periph_GPIOA

#define CS100A_ECHO_EXTI_LINE        EXTI_Line0
#define CS100A_ECHO_EXTI_PORTSOURCE  GPIO_PortSourceGPIOA   
#define CS100A_ECHO_EXTI_PINSOURCE   GPIO_PinSource0



//#define CS100A_TIMEOUT_MS       50


typedef enum
{
    cs100a_idle = 0,
    cs100a_wait_rising,
    cs100a_measuring,
    cs100a_done,
    cs100a_state_timeout
}  cs100a_state;

extern volatile cs100a_state g_cs100a_state ;

extern volatile uint16_t g_cs100a_echo_time ;


void cs100a_init(void);

void cs100a_start(void);

uint8_t cs100a_isFinished(void);

uint8_t cs100a_isTimeout(void);

uint16_t cs100a_getEchoTimeUs(void);

uint16_t cs100a_getDistanceMm(void);

uint16_t cs100a_CalDistanceMm(void);

uint8_t cs100a_isBusy(void);


# endif
