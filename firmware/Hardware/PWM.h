#ifndef __PWM_H
#define __PWM_H

#include "stm32f10x.h"

void TIM2_PWM_Init(void);
void TIM3_Servo_Init(void);
void PWM_SetCompare3(uint16_t compare);
void PWM_SetCompare4(uint16_t compare);
void Servo_SetAngle(uint8_t angle);

#endif
