#include "stm32f10x.h"
#include "Car.h"
#include "Track.h"
#include "Bluetooth.h"
#include "Delay.h"
#include "PWM.h"
#include <string.h>

// 5路循迹：TrackNow[0]=最左(PB9) [1]=中左(PB8) [2]=中间(PB12) [3]=中右(PB7) [4]=最右(PB6)
u8 TrackNow[5];
u8 TrackLast[5];
u8 StateStr[6];
u8 LastValidState[5];

void Send_Track_State(void);
void Track_Control(void);

int main(void)
{
    TIM2_PWM_Init();
    Car_Init();
    Bluetooth_Init();
    Track_Init();
    
    // 初始有效状态：仅中间黑 11011
    LastValidState[0] = 1;
    LastValidState[1] = 1;
    LastValidState[2] = 0;
    LastValidState[3] = 1;
    LastValidState[4] = 1;
    
    Delay_ms(500);
    Serial_SendString("===== 5-Channel Track Car Control =====\r\n");
    Serial_SendString("Logic: 0=Black, 1=White\r\n");
    Serial_SendString("01111=TurnL, 10111=Left, 11011=Go, 11101=Right, 11110=TurnR\r\n\r\n");

    while(1)
    {
        // 读取5路循迹传感器
        TrackNow[0] = Track_Get4();  // PB9 → 最左边
        TrackNow[1] = Track_Get3();  // PB8 → 中左
        TrackNow[2] = Track_Get5();  // PB12 → 中间
        TrackNow[3] = Track_Get2();  // PB7 → 中右
        TrackNow[4] = Track_Get1();  // PB6 → 最右边

        // 状态变化才发送
        if(memcmp(TrackNow, TrackLast, 5) != 0)
        {
            memcpy(TrackLast, TrackNow, 5);
            Send_Track_State();
        }

        Track_Control();
        Delay_ms(20);
    }
}

void Track_Control(void)
{
    // ==============================================
        // 仅处理这 7 种状态，其余全部沿用上次
    // ==============================================

    // 1. 00000 全黑 → 停车
    if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0 && TrackNow[4]==0)
    {
        Delay_ms(1000);
        Car_Stop();
        Serial_SendString("Action: 00000 All Black -> Stop\r\n");
    }

    // 2. 11111 全白 → 沿用上次
    else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1 && TrackNow[4]==1)
    {
        if(LastValidState[0]==0 && LastValidState[1]==1 && LastValidState[2]==1 && LastValidState[3]==1 && LastValidState[4]==1)
        {
            Turn_Left();
            Serial_SendString("Action: 11111 -> Last Turn_Left\r\n");
        }
        else if(LastValidState[0]==1 && LastValidState[1]==0 && LastValidState[2]==1 && LastValidState[3]==1 && LastValidState[4]==1)
        {
            Turn_Left();
            Serial_SendString("Action: 11111 -> Last Left\r\n");
        }
        else if(LastValidState[0]==1 && LastValidState[1]==1 && LastValidState[2]==0 && LastValidState[3]==1 && LastValidState[4]==1)
        {
            Go_Ahead();
            Serial_SendString("Action: 11111 -> Last Go\r\n");
        }
        else if(LastValidState[0]==1 && LastValidState[1]==1 && LastValidState[2]==1 && LastValidState[3]==0 && LastValidState[4]==1)
        {
            Turn_Right();
            Serial_SendString("Action: 11111 -> Last Right\r\n");
        }
        else if(LastValidState[0]==1 && LastValidState[1]==1 && LastValidState[2]==1 && LastValidState[3]==1 && LastValidState[4]==0)
        {
            Turn_Right();
            Serial_SendString("Action: 11111 -> Last Turn_Right\r\n");
        }
        else
        {
            Go_Ahead();
            Serial_SendString("Action: 11111 -> Last Go\r\n");
        }
    }

    // 3. 01111 仅最左黑 → 急左转
    else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1 && TrackNow[4]==1)
    {
        memcpy(LastValidState, TrackNow, 5);
        Turn_Left();
        Serial_SendString("Action: 01111 -> Turn_Left\r\n");
    }

    // 4. 10111 仅中左黑 → 左转
    else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==1 && TrackNow[3]==1 && TrackNow[4]==1)
    {
        memcpy(LastValidState, TrackNow, 5);
        Turn_Left();
        Serial_SendString("Action: 10111 -> Left\r\n");
    }

    // 5. 11011 仅中间黑 → 直行
    else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==0 && TrackNow[3]==1 && TrackNow[4]==1)
    {
        memcpy(LastValidState, TrackNow, 5);
        Go_Ahead();
        Serial_SendString("Action: 11011 -> Go_Ahead\r\n");
    }

    // 6. 11101 仅中右黑 → 右转
    else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==0 && TrackNow[4]==1)
    {
        memcpy(LastValidState, TrackNow, 5);
        Turn_Right();
        Serial_SendString("Action: 11101 -> Right\r\n");
    }

    // 7. 11110 仅最右黑 → 急右转
    else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1 && TrackNow[4]==0)
    {
        memcpy(LastValidState, TrackNow, 5);
        Turn_Right();
        Serial_SendString("Action: 11110 -> Turn_Right\r\n");
    }

    // ================================
    // 其余所有状态 → 直接沿用上次有效动作
    // ================================
    else
    {
        if(LastValidState[0]==0 && LastValidState[1]==1 && LastValidState[2]==1 && LastValidState[3]==1 && LastValidState[4]==1)
        {
            Turn_Left();
        }
        else if(LastValidState[0]==1 && LastValidState[1]==0 && LastValidState[2]==1 && LastValidState[3]==1 && LastValidState[4]==1)
        {
            Turn_Left();
        }
        else if(LastValidState[0]==1 && LastValidState[1]==1 && LastValidState[2]==0 && LastValidState[3]==1 && LastValidState[4]==1)
        {
            Go_Ahead();
        }
        else if(LastValidState[0]==1 && LastValidState[1]==1 && LastValidState[2]==1 && LastValidState[3]==0 && LastValidState[4]==1)
        {
            Turn_Right();
        }
        else if(LastValidState[0]==1 && LastValidState[1]==1 && LastValidState[2]==1 && LastValidState[3]==1 && LastValidState[4]==0)
        {
            Turn_Right();
        }
        else
        {
            Go_Ahead();
        }
        Serial_SendString("Action: Other State -> Last Action\r\n");
    }
}

void Send_Track_State(void)
{
    StateStr[0] = TrackNow[0] + '0';
    StateStr[1] = TrackNow[1] + '0';
    StateStr[2] = TrackNow[2] + '0';
    StateStr[3] = TrackNow[3] + '0';
    StateStr[4] = TrackNow[4] + '0';
    StateStr[5] = '\0';

    Serial_SendString("Track: ");
    Serial_SendString((char*)StateStr);
    Serial_SendString(" (0=Black,1=White)\r\n");
}
