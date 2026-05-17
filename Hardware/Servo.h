#ifndef __SERVO_H
#define __SERVO_H

void Servo_Init(void);

void Servo_SetAngle(float Angle);

int Servo_Shake(void);

void PWM_SetCompare1(uint16_t Compare);

#endif
