#include "stm32f10x.h"
#include "Car.h"
#include "Track.h"
#include "Bluetooth.h"
#include "Delay.h"
#include "PWM.h"
#include <string.h>

u8 TrackNow[4];
u8 TrackLast[4];
u8 StateStr[5];
u8 LastValidState[4];

void Send_Track_State(void);
void Track_Control(void);

int main(void)
{
    TIM2_PWM_Init();
    Car_Init();
    Bluetooth_Init();
    Track_Init();
    
    LastValidState[0] = 1;
    LastValidState[1] = 0;
    LastValidState[2] = 0;
    LastValidState[3] = 1;
    
    Delay_ms(500);
    Serial_SendString("===== 4-Channel Track Car Control =====\r\n");
    Serial_SendString("Logic: 1=Black, 0=White\r\n");
    Serial_SendString("x11x=Go, x10x=Left, x01x=Right, 1000=TurnL, 0001=TurnR\r\n\r\n");

    while(1)
    {
        TrackNow[0] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9);
        TrackNow[1] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8);
        TrackNow[2] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);
        TrackNow[3] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);

        if(memcmp(TrackNow, TrackLast, 4) != 0)
        {
            memcpy(TrackLast, TrackNow, 4);
            Send_Track_State();
        }

        Track_Control();
        Delay_ms(20);
    }
}

void Track_Control(void)
{
    // 1. 全黑停车
    if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1)
    {
		Delay_ms(2000);
        Car_Stop();
        Serial_SendString("1111 -> Stop\r\n");
        return;
    }

    // 2. 全白 保持上次
    if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        if(LastValidState[1]==1 && LastValidState[2]==0)      Self_Left();
        else if(LastValidState[1]==0 && LastValidState[2]==1) Self_Right();
        else if(LastValidState[0]==1)                         Turn_Left();
        else if(LastValidState[3]==1)                         Turn_Right();
        else                                                 Go_Ahead();
        Serial_SendString("0000 -> Keep\r\n");
        return;
    }

    // ===================== 全部16种状态判断 =====================

    // 0001 最右 → 右转
    if(TrackNow[0]==0&&TrackNow[1]==0&&TrackNow[2]==0&&TrackNow[3]==1)
    { Turn_Right(); Serial_SendString("0001 -> Turn_Right\r\n"); }

    // 0010 右中 → 右微调
    else if(TrackNow[0]==0&&TrackNow[1]==0&&TrackNow[2]==1&&TrackNow[3]==0)
    { Self_Right(); Serial_SendString("0010 -> Self_Right\r\n"); }

    // 0011 右中+最右 → 右转
    else if(TrackNow[0]==0&&TrackNow[1]==0&&TrackNow[2]==1&&TrackNow[3]==1)
    { Turn_Right(); Serial_SendString("0011 -> Turn_Right\r\n"); }

    // 0100 左中 → 左微调
    else if(TrackNow[0]==0&&TrackNow[1]==1&&TrackNow[2]==0&&TrackNow[3]==0)
    { Self_Left(); Serial_SendString("0100 -> Self_Left\r\n"); }

    // 0101 左中+最右 → 左微调
    else if(TrackNow[0]==0&&TrackNow[1]==1&&TrackNow[2]==0&&TrackNow[3]==1)
    { Self_Left(); Serial_SendString("0101 -> Self_Left\r\n"); }

    // 0110 双中 → 直行
    else if(TrackNow[0]==0&&TrackNow[1]==1&&TrackNow[2]==1&&TrackNow[3]==0)
    { Go_Ahead(); Serial_SendString("0110 -> Go_Ahead\r\n"); }

    // 0111 左中+右中+最右 → 右转
    else if(TrackNow[0]==0&&TrackNow[1]==1&&TrackNow[2]==1&&TrackNow[3]==1)
    { Turn_Right(); Serial_SendString("0111 -> Turn_Right\r\n"); }

    // 1000 最左 → 左转
    else if(TrackNow[0]==1&&TrackNow[1]==0&&TrackNow[2]==0&&TrackNow[3]==0)
    { Turn_Left(); Serial_SendString("1000 -> Turn_Left\r\n"); }

    // 1001 最左+最右 → 直行
    else if(TrackNow[0]==1&&TrackNow[1]==0&&TrackNow[2]==0&&TrackNow[3]==1)
    { Go_Ahead(); Serial_SendString("1001 -> Go_Ahead\r\n"); }

    // 1010 最左+右中 → 右微调
    else if(TrackNow[0]==1&&TrackNow[1]==0&&TrackNow[2]==1&&TrackNow[3]==0)
    { Self_Right(); Serial_SendString("1010 -> Self_Right\r\n"); }

    // 1011 最左+右中+最右 → 右微调
    else if(TrackNow[0]==1&&TrackNow[1]==0&&TrackNow[2]==1&&TrackNow[3]==1)
    { Self_Right(); Serial_SendString("1011 -> Self_Right\r\n"); }

    // 1100 最左+左中 → 左转
    else if(TrackNow[0]==1&&TrackNow[1]==1&&TrackNow[2]==0&&TrackNow[3]==0)
    { Turn_Left(); Serial_SendString("1100 -> Turn_Left\r\n"); }

    // 1101 最左+左中+最右 → 左微调
    else if(TrackNow[0]==1&&TrackNow[1]==1&&TrackNow[2]==0&&TrackNow[3]==1)
    { Self_Left(); Serial_SendString("1101 -> Self_Left\r\n"); }

    // 1110 最左+左中+右中 → 左转
    else if(TrackNow[0]==1&&TrackNow[1]==1&&TrackNow[2]==1&&TrackNow[3]==0)
    { Turn_Left(); Serial_SendString("1110 -> Turn_Left\r\n"); }

    // ===================== 兜底：保持上次动作（已修改） =====================
    else
    {
        if(LastValidState[1]==1 && LastValidState[2]==0)      Self_Left();
        else if(LastValidState[1]==0 && LastValidState[2]==1) Self_Right();
        else if(LastValidState[0]==1)                         Turn_Left();
        else if(LastValidState[3]==1)                         Turn_Right();
        else                                                  Go_Ahead();
        Serial_SendString("Other -> Keep Last Action\r\n");
        return;
    }

    // 只有有效状态才更新
    memcpy(LastValidState, TrackNow, 4);
}

void Send_Track_State(void)
{
    StateStr[0] = TrackNow[0] + '0';
    StateStr[1] = TrackNow[1] + '0';
    StateStr[2] = TrackNow[2] + '0';
    StateStr[3] = TrackNow[3] + '0';
    StateStr[4] = '\0';

    Serial_SendString("Track: ");
    Serial_SendString((char*)StateStr);
    Serial_SendString(" (1=Black,0=White)\r\n");
}
