#include "Ultrasonic.h"
#include "Delay.h"

void Ultrasonic_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // PB12 TRIG
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // PB13 ECHO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void TIM4_US_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    // 72M/(71+1)=1MHz → 1us计数1次
    TIM_TimeBaseStructure.TIM_Period = 65535;
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM4, DISABLE);
}

float Get_Distance(void)
{
    u32 time = 0;
    u32 timeout = 0;

    GPIO_SetBits(GPIOB, GPIO_Pin_12);
    Delay_us(10);
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);

    // 等待Echo变高，超时防止卡死
    while((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)==0) && timeout<30000) timeout++;
    TIM_SetCounter(TIM4, 0);
    TIM_Cmd(TIM4, ENABLE);

    timeout = 0;
    // 等待Echo变低
    while((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)==1) && timeout<30000) timeout++;
    TIM_Cmd(TIM4, DISABLE);

    time = TIM_GetCounter(TIM4);
    return time / 58.0f; // 距离(cm) = 时间(us)/58
}
