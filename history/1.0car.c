#include "stm32f10x.h"
#include "Car.h"
#include "Track.h"
#include "Bluetooth.h"
#include "Delay.h"
#include "PWM.h"     // 加 PWM 头文件
#include <string.h>

u8 TrackNow[4];
u8 TrackLast[4];
u8 StateStr[5];

void Send_Track_State(void);
void Send_Error(char *str);

int main(void)
{
    TIM2_PWM_Init();   // ←←← 电机必须开 PWM
    Car_Init();        // 小车初始化
    Bluetooth_Init();  // 蓝牙初始化
    Track_Init();      // 循迹初始化
    
    Delay_ms(500);
    Serial_SendString("Track System Ready\r\n");

    while(1)
    {
        // 读取4路循迹：左→右 PB9 PB8 PB7 PB6
        TrackNow[0] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9);
        TrackNow[1] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8);
        TrackNow[2] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);
        TrackNow[3] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);

        // 状态变化上报
        if(memcmp(TrackNow, TrackLast, 4) != 0)
        {
            memcpy(TrackLast, TrackNow, 4);
            Send_Track_State();
        }

        // ================= 循迹行走逻辑 =================
        if(TrackNow[1] == 0 && TrackNow[2] == 0)
        {
            Go_Ahead();       // 中间两个在黑线 → 直行
        }
        else if(TrackNow[0]==0 || TrackNow[1]==0)
        {
            Turn_Left();      // 左边碰到线 → 左转
        }
        else if(TrackNow[2]==0 || TrackNow[3]==0)
        {
            Turn_Right();     // 右边碰到线 → 右转
        }
        else
        {
            Car_Stop();       // 无线 → 停车
            Send_Error("Error: No Track\r\n");
        }

        Delay_ms(20);
    }
}

// 发送状态
void Send_Track_State(void)
{
    StateStr[0] = TrackNow[0] + '0';
    StateStr[1] = TrackNow[1] + '0';
    StateStr[2] = TrackNow[2] + '0';
    StateStr[3] = TrackNow[3] + '0';
    StateStr[4] = '\0';

    Serial_SendString("Track:");
    Serial_SendString((char*)StateStr);
    Serial_SendString("\r\n");
}

// 发送错误
void Send_Error(char *str)
{
    Serial_SendString(str);
}
