/*
 * @Author       : WXj
 * @Date         : 2026-09-25 23:00:11
 * @LastEditors  : Wangxiaojie
 * @LastEditTime : 2026-10-07 10:55:11
 * @Description  : 
 * @FilePath     : \STM32超声波测距方案\main.c
 */
#include "stm32f10x.h"                  // Device header
#include "st7735.h"
#include "Delay.h"
#include "ultrasonic.h"


uint16_t distance_mm = 0;
uint16_t echo_time_us = 0;

static void LCD_ShowUnit(uint16_t x, uint16_t y, uint32_t value, uint16_t color, uint16_t bgcolor , uint8_t size)
{
	char   buf[10];
	uint8_t n = 0;
	if (value == 0)
	{
		LCD_ShowChar(x,y,'0',color,bgcolor,size);
	}
	while (value > 0 && n < 10)
	{
		buf[n] = (char)('0' + (value % 10));
		value /= 10;
		n++;
	}
	while (n > 0)
	{
		n--;
		LCD_ShowChar(x,y,buf[n],color,bgcolor,size);
		x+=8*size;
	}
	
}

int main(void)
{
	
	LCD_Init();
	cs100a_init();// Initialize the ultrasonic sensor

	/* Power-on color test; each solid frame remains visible for 300 ms. */
	LCD_Clear(LCD_RED);
	Delay_ms(300);
	LCD_Clear(LCD_WHITE);
	Delay_ms(300);
	LCD_ShowStringUTF8(0, 0, "Hello, 超声波!", LCD_BLACK, LCD_WHITE, 1);
	
	LCD_ShowStringUTF8(0, 16, "POWERED BY", LCD_BLACK, LCD_WHITE, 1);

	LCD_ShowStringUTF8(0, 32, "STM32F103C8T6", LCD_GREEN, LCD_WHITE, 1);

	

	LCD_ShowStringUTF8(0, 48, "Distance:", LCD_BLACK, LCD_WHITE, 1);
	//LCD_ShowUnit(0,64,distance_mm,LCD_GREEN,LCD_WHITE,1);
	LCD_ShowStringUTF8(64, 64, "mm", LCD_BLACK, LCD_WHITE, 1);
	
	while(1)
	{
		cs100a_start();
		while (cs100a_isFinished() == 0)
		{
			//空等测量结束
		}
		if (cs100a_isTimeout() == 1)
		{
			LCD_ShowStringUTF8(0,64,"---   ",LCD_RED,LCD_WHITE,1);
			
		}
		else
		{
			distance_mm = cs100a_CalDistanceMm();
			LCD_Fill(0,64,63,80,LCD_WHITE);
			LCD_ShowUnit(0,64,distance_mm,LCD_GREEN,LCD_WHITE,1);
			
		}
		Delay_ms(300);
	}
	
	
	
	/*============================================================
	* 设置的中断函数在主函数中如何调用才可以实现连续测量与显示？
	* 
	*
	*
	*
	*
	*/

/* ============================================================
 * 【问题】设置的中断函数在主函数中如何调用才可以实现连续测量与显示？
 * ============================================================
 *
 * 答：先说结论。这两个中断函数（EXTI0_IRQHandler、TIM2_IRQHandler）
 *     不需要在主函数里"调用"，也不应该去调用——它们是由硬件通过中断向量
 *     表自动触发的。主函数要做的事情不是"调用中断"，而是在 while(1) 里写一个
 *     "轮询状态"的循环：发起测量 → 等本次结束 → 读结果 → 显示 → 再发起。
 *
 * 一、为什么不需要、也不能手工调用
 *   1) 中断函数的名字是和启动文件里的向量表绑定的。
 *      Start/startup_stm32f10x_md.s 里，EXTI0_IRQHandler 出现在第 85 行
 *      （向量表）和第 235 行（WEAK 弱定义），TIM2_IRQHandler 出现在
 *      第 107 行和第 257 行。链接器一旦在你的程序里找到同名函数，
 *      就把它的地址填进向量表；之后只要 ECHO 引脚有边沿（EXTI0）、
 *      或者 TIM2 计数溢出（更新事件），Cortex-M3 内核会自动跳过去执行。
 *      整个过程不需要 main() 参与，也不需要你写任何调用语句。
 *   2) 如果在 main() 里写 EXTI0_IRQHandler();，那只是把它当成一个普通函数
 *      又执行了一遍：会重复读 TIM_GetCounter()、重复清中断标志、
 *      在错误的时刻改写 g_cs100a_state，把状态机彻底搅乱。
 *   3) 所以正确的分工是：中断函数只负责"在后台记录数据、推进状态"，
 *      main() 只负责"读状态、取数据、显示"，两者通过全局变量
 *      （g_cs100a_state、g_cs100a_echo_time）通信。
 *      这也正是这两个变量必须声明为全局、并且加 volatile 的原因——
 *      volatile 保证主循环每次都真的去读内存，而不会把
 *      "状态 == done" 的判定结果缓存进寄存器，导致永远等不到。
 *
 * 二、当前 main.c 为什么测不到：读得太早，而且只测了一次
 *       第 58 行  cs100a_start();                          // 只是"发起"
 *       第 60 行  echo_time_us = cs100a_getEchoTimeUs();   // 回波还没回来
 *       第 62 行  distance_mm  = cs100a_getDistanceMm();   // 回波还没回来
 *   执行到第 62 行时，g_cs100a_state 还停在 cs100a_wait_rising
 *   （正在等回波的上升沿），而 cs100a_getDistanceMm() 的开头是
 *        if (g_cs100a_state != cs100a_done) { return 0; }
 *   所以它直接返回 0，屏幕上永远显示 0。而且这一段在 while(1) 之外，
 *   只执行一次，即使等到了也不会再测第二次。
 *   要做到"连续测量与显示"，必须把"发起 → 等待 → 读取 → 显示"这个过程
 *   放进 while(1) 里反复执行。
 *
 * 三、推荐的主循环结构（以下是示意，本次不修改代码）
 *
 *    int main(void)
 *    {
 *        LCD_Init();
 *        cs100a_init();
 *
 *        // 静态文字开机只画一次，不进循环（原因见第四点第 3 条）
 *        LCD_Clear(LCD_WHITE);
 *        LCD_ShowStringUTF8(0,  0, "Hello, 超声波!", LCD_BLACK, LCD_WHITE, 1);
 *        LCD_ShowStringUTF8(0, 16, "POWERED BY",     LCD_BLACK, LCD_WHITE, 1);
 *        LCD_ShowStringUTF8(0, 32, "STM32F103C8T6",  LCD_GREEN, LCD_WHITE, 1);
 *        LCD_ShowStringUTF8(0, 48, "Distance:",      LCD_BLACK, LCD_WHITE, 1);
 *        LCD_ShowStringUTF8(64, 64, "mm",            LCD_BLACK, LCD_WHITE, 1);
 *
 *        while (1)
 *        {
 *            cs100a_start();                     // 1. 发起一次测量
 *
 *            while (cs100a_isFinished() == 0)    // 2. 等本次测量结束
 *            {
 *                // 空等即可。g_cs100a_state 是 volatile，
 *                // 所以这里每次都真的去内存读，能等到中断改状态。
 *                // 最坏情况是等一次超时，即 65.5ms。
 *            }
 *
 *            if (cs100a_isTimeout())             // 3. 区分"超时"和"测到"
 *            {
 *                LCD_ShowStringUTF8(0, 64, "---   ", LCD_BLACK, LCD_WHITE, 1);
 *            }
 *            else
 *            {
 *                echo_time_us = cs100a_getEchoTimeUs();
 *                distance_mm  = cs100a_getDistanceMm();
 *                LCD_ShowUnit(0, 64, distance_mm, LCD_GREEN, LCD_WHITE, 1);
 *            }
 *
 *            Delay_ms(100);                      // 4. 控制刷新率
 *        }
 *    }
 *
 * 四、四个必须注意的地方
 *
 *   1) 等待条件用 cs100a_isFinished()，不要自己直接比较状态值。
 *      它把 cs100a_done（测到回波）和 cs100a_state_timeout（超时）
 *      都算作"本次结束"，所以无论哪种结果循环都能往下走；
 *      再用 cs100a_isTimeout() 区分这两种情况。
 *      而读取类函数 cs100a_getEchoTimeUs() / cs100a_getDistanceMm()
 *      只在状态是 cs100a_done 时返回有效值，其余情况一律返回 0。
 *      所以"先判断状态、再读数据"是必须的，否则会把 0 当成真实距离显示出去。
 *      这几个函数在 ultrasonic.h 里已经导出，直接用它们比自己写
 *      if (g_cs100a_state == ...) 更清楚，也能避免以后再出现枚举名写错的问题。
 *
 *   2) cs100a_start() 本身是可以重复调用的，所以"反复调用"就等于"连续测量"。
 *      它开头有一句保护：
 *          if (状态 == cs100a_wait_rising || 状态 == cs100a_measuring) return;
 *      意思是"上一次还没结束就忽略这次调用，不打断它"。
 *      而每次成功调用都会完整地做一遍：
 *          清 g_cs100a_echo_time → 置状态为 cs100a_wait_rising
 *          → 复位并启动 TIM2 → 拉高 TRIG → 延时 10us → 拉低 TRIG
 *      注意顺序：TIM2 是先启动、后发触发脉冲的；而回波上升沿到来时，
 *      EXTI0 中断里会执行 TIM_SetCounter(TIM2, 0) 把计数器清零。
 *      所以最终读到的是纯粹的"回波高电平宽度"，这一点设计是对的。
 *
 *   3) 静态文字不要放进循环，只刷新会变的数值。
 *      LCD 刷新很慢：SPI1 的分频是 64（Hardware/st7735.c 第 85 行
 *      SPI_BaudRatePrescaler_64），而 SPI1 挂在 APB2 上、PCLK2 = 72MHz，
 *      所以 SPI 时钟 ≈ 72MHz / 64 = 1.125MHz。按这个速率算：
 *          一个 8x16 的 ASCII 字符 = 8x16 = 128 像素 x 2 字节
 *                                   = 256 字节 ≈ 1.8ms
 *          一个 16x16 的汉字      = 16x16 = 256 像素 x 2 字节
 *                                   = 512 字节 ≈ 3.6ms
 *          所以 "Hello, 超声波!" 这一行（8 个 ASCII + 3 个汉字）
 *                                   = 3584 字节 ≈ 25ms
 *          而整屏 LCD_Clear()     = 160x128 = 20480 像素 x 2 字节
 *                                   = 40960 字节 ≈ 291ms
 *      （这也是开机时"红屏清一次 + 白屏清一次"加上两次 300ms 延时，
 *        一共要一秒多的原因。）
 *      如果每圈循环都把所有文字重画一遍，光显示就要一百多毫秒，
 *      测量周期反而被显示拖慢了。所以：静态文字开机画一次，
 *      循环里只重画 distance_mm 那几个数字。
 *
 *   4) 数值位数变化会留下残影，这是连续显示最常见的坑。
 *      LCD_ShowUnit() 是按实际位数逐位画字符的（每画一位 x += 8*size）。
 *      如果上一次显示 1234（占了 4 个字格），这一次是 87（只占 2 个字格），
 *      后面两格没有被覆盖，屏幕上就会残留成 "8734" 这样的假数字。
 *      两种解决办法，任选一种：
 *        a) 补足固定宽度。例如固定占 5 格，先画 5 个空格把这一小块擦掉，
 *           再画数值：
 *               LCD_ShowStringUTF8(0, 64, "     ", LCD_BLACK, LCD_WHITE, 1);
 *               LCD_ShowUnit(0, 64, distance_mm, LCD_GREEN, LCD_WHITE, 1);
 *        b) 画之前用背景色把这一小块区域填掉：
 *               LCD_Fill(0, 64, 63, 79, LCD_WHITE);
 *           （LCD_Fill 的四个坐标都是包含端点的，只要 x1 < LCD_WIDTH、
 *             y1 < LCD_HEIGHT 就合法，这里 63 < 160、79 < 128，没问题。）
 *      现在的 "mm" 画在 x=64；uint16_t 最大 65535 是 5 位数、占 40 像素，
 *      所以 x=0~39 这个范围不会和 "mm" 重叠，用哪种做法都安全。
 *
 * 五、两点可选改进（不影响本次功能，供以后扩展参考）
 *   1) 等待方式。上面用的是"忙等"，最坏要等一次超时，也就是 65.5ms
 *      （TIM2 的 ARR = 0xFFFF、计数频率 1MHz，见 System/Timer.c）。
 *      这段时间 CPU 什么都不做。对"只做测距 + 显示"的当前程序完全够用；
 *      但以后如果还要同时处理按键、串口，就应该改成非阻塞：
 *      主循环不等待，只判断 cs100a_isFinished()，没结束就先去做别的事，
 *      下一圈再回来检查。这样既不占死 CPU，也不会漏掉按键。
 *   2) 想缩短最坏等待时间，可以把 System/Timer.c 里的
 *      TIM_TimeBaseStructure.TIM_Period 从 0xFFFF 改小，
 *      例如 50000 → 50ms 超时（代价是量程上限相应变小，50ms 对应约 8.5m，
 *      而本模块实际能测的距离远小于此，所以通常没有损失）。
 *      顺带一个好处：ARR 设得比"溢出"更早触发超时，
 *      能避开"回波下降沿恰好落在计数器回卷之后"那种读到偏小值的窗口。
 *
 * 【一句话总结】
 *   中断函数不用、也不能在主函数里调用——它们由硬件向量表自动触发。
 *   main() 要做的是在 while(1) 里循环执行：
 *       cs100a_start()  →  等 cs100a_isFinished()  →
 *       按 cs100a_isTimeout() 分支读取并显示  →  Delay_ms(100)
 *   并且只刷新数值、注意位数变化留下的残影。
 * ============================================================ */
	
}

