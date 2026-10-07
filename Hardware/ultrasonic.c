/*
 * @Author       : WXj
 * @Date         : 2026-09-26 22:50:58
 * @LastEditors  : Wangxiaojie
 * @LastEditTime : 2026-10-07 16:49:14
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\Hardware\ultrasonic.c
 */
# include <stdint.h>
# include "Timer.h"
# include "Delay.h"
#include "ultrasonic.h"

volatile cs100a_state  g_cs100a_state    = cs100a_idle;
volatile uint16_t      g_cs100a_echo_time = 0;



static void SortData(uint16_t *data, uint8_t size)
{
    for (int i = 0; i < size - 1; i++)
    {
        for (int j = 0; j < size - 1 - i; j++)
        {
            if (data[j] > data[j+1])
            {
                uint16_t temp = data[j];
                data[j] = data[j+1];
                data[j+1] = temp;
            }
        }
    }
}


static void cs100a_EXTI_init(void)
{
    RCC_APB2PeriphClockCmd(CS100A_TRIG_RCC, ENABLE);
    RCC_APB2PeriphClockCmd(CS100A_ECHO_RCC, ENABLE);// Enable GPIO clocks

    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);// Enable AFIO clock

    GPIO_EXTILineConfig(CS100A_ECHO_EXTI_PORTSOURCE, CS100A_ECHO_EXTI_PINSOURCE);// Connect EXTI Line to GPIO pin

    
    EXTI_InitStructure.EXTI_Line = CS100A_ECHO_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; // Trigger on both rising and falling edges
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;

    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn; // Assuming EXTI0 is used for the echo pin
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;// Set priority
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;// Set subpriority

    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;// Enable the interrupt

    NVIC_Init(&NVIC_InitStructure);





}

void cs100a_init(void)
{
    RCC_APB2PeriphClockCmd(CS100A_TRIG_RCC, ENABLE);
    RCC_APB2PeriphClockCmd(CS100A_ECHO_RCC, ENABLE);// Enable GPIO clocks

    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = CS100A_TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CS100A_TRIG_GPIO, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = CS100A_ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CS100A_ECHO_GPIO, &GPIO_InitStructure);

    cs100a_EXTI_init();
    Timer_Init();


}

void cs100a_start(void)
{
    if (g_cs100a_state == cs100a_wait_rising || g_cs100a_state == cs100a_measuring)
    {
        return; // Already measuring, ignore new start command
    }

    g_cs100a_echo_time = 0;

    g_cs100a_state = cs100a_wait_rising;

    TIM_Cmd(TIM2,DISABLE);
    TIM_SetCounter(TIM2, 0);// Reset the timer counter

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    TIM_Cmd(TIM2, ENABLE);

    GPIO_SetBits(CS100A_TRIG_GPIO, CS100A_TRIG_PIN);

    Delay_us(50); // 50us pulse

    GPIO_ResetBits(CS100A_TRIG_GPIO, CS100A_TRIG_PIN);



}

void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(CS100A_ECHO_EXTI_LINE) != RESET)
    {
        if (GPIO_ReadInputDataBit(CS100A_ECHO_GPIO, CS100A_ECHO_PIN) == Bit_SET)
        {
            if (g_cs100a_state == cs100a_wait_rising)
            {
                g_cs100a_state = cs100a_measuring;
                TIM_SetCounter(TIM2, 0); // Reset the timer counter
                TIM_ClearITPendingBit(TIM2, TIM_IT_Update);// Clear any pending update interrupt
            }

        }
        else
        {
            if (g_cs100a_state == cs100a_measuring)
            {
                g_cs100a_echo_time = TIM_GetCounter(TIM2);
                g_cs100a_state = cs100a_done;
                TIM_Cmd(TIM2, DISABLE);
            }
        }

        EXTI_ClearITPendingBit(CS100A_ECHO_EXTI_LINE);

    }
}

uint8_t cs100a_isFinished(void)
{
    return (
        g_cs100a_state == cs100a_done ||
        g_cs100a_state == cs100a_state_timeout 
    );
}

uint8_t cs100a_isTimeout(void)
{
    return (g_cs100a_state == cs100a_state_timeout);
}

uint16_t cs100a_getEchoTimeUs(void)
{
    if ( g_cs100a_state == cs100a_done )
    {
        return g_cs100a_echo_time;
    }
    else
    {
        return 0;
    }
}

uint16_t cs100a_getDistanceMm(void)
{
    uint32_t echo_us;
    uint32_t distance_mm;

    if (g_cs100a_state != cs100a_done)
    {
        return 0;
    }

    echo_us = g_cs100a_echo_time;

    /*
     * distance =
     * echo_us × 343 / 2000
     */
    distance_mm =
        echo_us * 343UL / 2000UL;

    return (uint16_t)distance_mm;
}

uint16_t cs100a_CalDistanceMm(void)
{
    uint16_t buffer[10];
    for (int i = 0; i < 10; i++)
    {
        cs100a_start();
        while (!cs100a_isFinished())
        {
            ;
        }
        buffer[i] = cs100a_getDistanceMm();
        Delay_ms(10);
    }
    SortData(buffer, 10);

    uint32_t sum = 0;

    for (int i = 2; i < 8; i++)
    {
        sum += buffer[i];
    }

    return (uint16_t)(sum / 6 ) ;
}

uint8_t cs100a_isBusy(void)
{
    return (
        g_cs100a_state == cs100a_wait_rising ||
        g_cs100a_state == cs100a_measuring
    );
}
