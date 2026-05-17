#include "stm32f10x.h" // Device header

void Beep_Init(void)
{
    /*开启时钟*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // 开启GPIOB的时钟
    // 使用各个外设前必须开启时钟，否则对外设的操作无效
    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure = {0}; // 定义结构体变量

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // GPIO模式，赋值为推挽输出模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;        // GPIO引脚，赋值为第12号引脚
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // GPIO速度，赋值为50MHz

    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void Beep_Start(void)
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
}

void Beep_Stop(void)
{
    GPIO_SetBits(GPIOB, GPIO_Pin_12);
}
