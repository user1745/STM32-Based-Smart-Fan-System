#ifndef __DELAY_H
#define __DELAY_H

void Delay_us(uint32_t us);

void Delay_ms(uint32_t ms);

void Delay_s(uint32_t s);

/**
 * @brief  微秒级延时
 * @param  xus 延时时长，范围：0~233015
 * @retval 无
 */
void delay_us(uint32_t xus);

void delay_ms(uint32_t xms);

void delay_s(uint32_t xs);

#endif
