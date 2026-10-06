/*
 * @Author       : WXj
 * @Date         : 2026-09-27 10:10:09
 * @LastEditors  : Wangxiaojie
 * @LastEditTime : 2026-09-29 19:53:52
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\System\Timer.c
 */
# include "stm32f10x.h"
# include "Timer.h"
# include "ultrasonic.h"

void Timer_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_SetCounter(TIM2, 0);

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, DISABLE);
}


void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		
		//如何保证计数到65535的时候进行清零？
		/*而且有必要进行判断状态吗？
		*直接进行计数满就清零不可以吗？
		*另外变量g_cs100a_state 目前不可用，如何使其可用于状态判断
		*对于这个变量在多个文件中都出现过，如何能够保证不同文件中的参数指向相同的值？
		*/

/* ============================================================
 *
 * 【问题 1】如何保证计数到 65535 的时候进行清零？
 *
 * 答：通过自动重装载寄存器ARR的值进行定时清零。硬件本身会在计数器CNT达到ARR的值时自动将CNT清零，并触发更新中断（UIF）。因此，软件不需要手动清零CNT，只需设置ARR的值即可控制计数周期。
 *
 * 【问题 2】有必要进行判断状态吗？直接进行计数满就清零不可以吗？
 *
 * 答：需要正确判断超时。
 *
 * 【问题 3】变量 g_cs100a_state 目前不可用，如何使其可用于状态判断？
 *
 * 答：已进行extern声明，并在 ultrasonic.c 中定义，确保所有文件引用的是同一份内存。
 *
 * 【问题 4】该变量在多个文件中都出现过，如何保证不同文件中的参数指向相同的值？
 *
 * 答：extern 关键字用于在不同文件中声明同一个全局变量，确保它们指向同一块内存区域。只需在一个源文件中定义该变量（如 ultrasonic.c），其他文件通过 extern 声明即可访问同一变量。
 * ============================================================ */

        if (g_cs100a_state == cs100a_wait_rising || g_cs100a_state == cs100a_measuring)
        {
            TIM_Cmd(TIM2, DISABLE);
            g_cs100a_state = cs100a_state_timeout;
            g_cs100a_echo_time = 0;
            
        }
    }
}
