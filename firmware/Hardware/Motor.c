#include "stm32f10x.h"
#include "Motor.h"
#include "PWM.h"   // 必须加，修复所有警告

void Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    // 方向引脚初始化
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5| GPIO_Pin_6| GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// 左电机速度
void Motor_SetLeftSpeed(int8_t Speed)
{
    if(Speed > 0)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_5);
        GPIO_ResetBits(GPIOA, GPIO_Pin_4);
        PWM_SetCompare4(Speed);            // 改用通道4 (PA3)
    }
    else if(Speed < 0)
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_5);
        GPIO_SetBits(GPIOA, GPIO_Pin_4);
        PWM_SetCompare4(-Speed);
    }
    else
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_4);
        GPIO_ResetBits(GPIOA, GPIO_Pin_5);
        PWM_SetCompare4(0);
    }
}

// 右电机速度
void Motor_SetRightSpeed(int8_t Speed)
{
    if(Speed > 0)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_7);   // IN1
        GPIO_ResetBits(GPIOA, GPIO_Pin_6); // IN2
        PWM_SetCompare3(Speed);            // 改用通道3 (PA2)
    }
    else if(Speed < 0)
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_7);
        GPIO_SetBits(GPIOA, GPIO_Pin_6);
        PWM_SetCompare3(-Speed);           // 改用通道3
    }
    else
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_6);
        GPIO_ResetBits(GPIOA, GPIO_Pin_7);
        PWM_SetCompare3(0);
    }
}
