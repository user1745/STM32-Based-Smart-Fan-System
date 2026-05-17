#ifndef __Usart_H
#define __Usart_H

#include <stdio.h>
#include <stdarg.h>

void Usart_Init(void);

void Usart_SendByte(uint8_t Byte);

void Usart_SendArray(uint8_t *Array, uint16_t Length);

void Usart_SendString(char *String);

void Usart_SendNumber(uint32_t Number, uint8_t Length);

void Usart_Printf(char *format, ...);

uint8_t Usart_GetRxFlag(void);

uint8_t Usart_GetRxData(void);

#endif
