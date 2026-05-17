#include "stm32f10x.h" // Device header
#include "Timer.h"

uint8_t hour = 0, minute = 0, second = 0; // 定时设置

void Timer_Init(void)
{
    // 先禁用定时器并清除标志位
    TIM_Cmd(TIM1, DISABLE);
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

    // 使能TIM1时钟（注意TIM1在APB2总线上）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 使用内部时钟
    TIM_InternalClockConfig(TIM1);
    TIM_DeInit(TIM1);

    // 初始化定时器时基
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     // 时钟分频
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数模式
    TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;               // 自动重装载值
    TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;             // 预分频值
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            // 重复计数(高级定时器特有)
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

    // 清除更新标志并启用更新中断
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);

    // 配置NVIC
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;        // TIM1更新中断通道
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 使能中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        // 子优先级
    NVIC_Init(&NVIC_InitStructure);
}

int Timer_Get(void)
{
    return second;
}
