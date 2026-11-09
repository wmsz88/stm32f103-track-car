#include "Track.h"

void Track_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // PB6, PB7, PB8, PB9 四个循迹传感器
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

// PB6 → 最左边
u8 Track_Get1(void) { return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6); }

// PB7 → 左中
u8 Track_Get2(void) { return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7); }

// PB8 → 右中
u8 Track_Get3(void) { return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8); }

// PB9 → 最右边
//u8 Track_Get4(void) { return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9); }
