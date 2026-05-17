#include "stm32f10x.h" // Device header
#include "Delay.h"
#include "Encoder.h"
#include "Key.h"
#include "OLED.h"
#include "Timer.h"
#include "HC05.h"
#include "oledmenu.h"

void Servo_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 注意APB2要对应APB2

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	TIM_InternalClockConfig(TIM3);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1; // ARR
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1; // PSC
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure = {0};
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0; // CCR

	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);

	TIM_Cmd(TIM3, ENABLE);
}

void PWM_SetCompare1(uint16_t Compare)
{
	TIM_SetCompare1(TIM3, Compare);
}

void Servo_SetAngle(float Angle)
{
	PWM_SetCompare1(Angle / 180 * 2000 + 500); // 设置占空比											//将角度线性变换，对应到舵机要求的占空比范围上
}

// 全局变量
static int shake_angle = 50;
static int shake_dir = 1; // 1表示增加角度，-1表示减少角度

int Servo_Shake(void)
{
	int8_t NUM = Encoder_Get();
	HC05_GetData(RxData);
	if (RxSTA == 0)
	{
		RxSTA = 1;

		if (RxData[0] == 'D') // 上一项
		{
			g_FanState.shake = 0;
			return 0;
		}
	}
	if (NUM == -1)
	{
		g_FanState.shake = 0;
		return 0;
	}

	// 更新舵机角度
	Servo_SetAngle(shake_angle);

	// 更新下一个角度
	shake_angle += shake_dir;

	// 反转方向
	if (shake_angle >= 130)
	{
		shake_dir = -1;
	}
	else if (shake_angle <= 50)
	{
		shake_dir = 1;
	}

	return 1; // 表示摇头进行中
}
