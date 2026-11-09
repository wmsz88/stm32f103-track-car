#include "PWM.h"

void TIM2_PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 时基提速：预分频 0，自动重装载 99 → 频率 720kHz
    TIM_TimeBaseInitStructure.TIM_Period = 99;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 0;  // 这里改成 0，速度最快
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;

    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC4Init(TIM2, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);

    TIM_Cmd(TIM2, ENABLE);
}

void TIM3_Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBase3_Struct;
    TIM_OCInitTypeDef TIM_OC3_Struct;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    TIM_TimeBase3_Struct.TIM_Prescaler = 71;
    TIM_TimeBase3_Struct.TIM_Period = 1999;
    TIM_TimeBase3_Struct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBase3_Struct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBase3_Struct);

    TIM_OCStructInit(&TIM_OC3_Struct);
    TIM_OC3_Struct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OC3_Struct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC3_Struct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OC3Init(TIM3, &TIM_OC3_Struct);
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_Cmd(TIM3, ENABLE);
}

void PWM_SetCompare3(uint16_t compare)
{
    TIM_SetCompare3(TIM2, compare);
}

void PWM_SetCompare4(uint16_t compare)
{
    TIM_SetCompare4(TIM2, compare);
}

void Servo_SetAngle(uint8_t angle)
{
    TIM_SetCompare3(TIM3, angle*11+50);
}
