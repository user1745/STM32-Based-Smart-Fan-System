#include "stm32f10x.h" // Device header
#include "Usart.h"

uint8_t RxSTA = 1;
char RxData[5] = {0};

void HC05_Init(void)
{
	Usart_Init();
}

void HC05_EnterAT(void) // 使能蓝牙，允许连接
{
	GPIO_SetBits(GPIOA, GPIO_Pin_0);
}

void HC05_ExitAT(void) // 失能蓝牙，无法连接
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_0);
}

void HC05_SendString(char *Buf)
{
	Usart_Printf(Buf);
}

void HC05_GetData(char *Buf)
{
	uint32_t count = 0, a = 0;
	while (count < 10000)
	{
		if (Usart_GetRxFlag() == 1)
		{
			Buf[a] = Usart_GetRxData();
			a++;
			count = 0;
			RxSTA = 0;
		}
		count++;
	}
}
