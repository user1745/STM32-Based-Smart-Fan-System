#ifndef __ADC_H
#define __ADC_H

#include <stdint.h>

void AD_Init(void);

uint16_t AD_GetValue(uint8_t ADC_Channel);

float MQ2_GetData_PPM(void);

#endif
