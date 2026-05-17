#include "stm32f10x.h" // Device header
#include <stdio.h>
#include <stdarg.h>

uint8_t Usart_RxData;
uint8_t Usart_RxFlag;

void Usart_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure = {0};
    USART_InitStructure.USART_BaudRate = 460800;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

void Usart_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        ;
}

void Usart_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        Usart_SendByte(Array[i]);
    }
}

void Usart_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        Usart_SendByte(String[i]);
    }
}

uint32_t Usart_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

void Usart_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        Usart_SendByte(Number / Usart_Pow(10, Length - i - 1) % 10 + '0');
    }
}

int fputc(int ch, FILE *f)
{
    Usart_SendByte(ch);
    return ch;
}

void Usart_Printf(char *format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Usart_SendString(String);
}

uint8_t Usart_GetRxFlag(void)
{
    if (Usart_RxFlag == 1)
    {
        Usart_RxFlag = 0;
        return 1;
    }
    return 0;
}

uint8_t Usart_GetRxData(void)
{
    return Usart_RxData;
}

// 标准库示例：连续判断 5 个 0xAA
void USART1_IRQHandler(void)
{
    static uint8_t aa_count = 0; // 静态变量，记录连续收到 0xAA 的次数

    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t res = USART_ReceiveData(USART1);

        if (res == 0xAA)
        {
            aa_count++;
            if (aa_count >= 5) // 连续收到 5 次 0xAA 才重启
            {
                __disable_irq();
                NVIC_SystemReset();
            }
        }
        else
        {
            aa_count = 0;       // 一旦断开连续，计数清零
            Usart_RxData = res; // 直接使用已读取的数据，避免重复读取
            Usart_SendByte(Usart_RxData);
            Usart_RxFlag = 1;
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
