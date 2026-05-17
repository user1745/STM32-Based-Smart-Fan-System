#ifndef __TIMER_H
#define __TIMER_H

void Timer_Init(void);

int Timer_Get(void);

#define Echo_Port GPIOA
#define Echo_Pin GPIO_Pin_1

#endif
