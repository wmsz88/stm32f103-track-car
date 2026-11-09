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

u8 FirstBlackFlag = 1; 

// 中间两路分级控制变量
u8 left_grade = 0;
u8 right_grade = 0;
u32 left_time = 0;
u32 right_time = 0;

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
    Serial_SendString("First Black: Go, After 1.5s Stop\r\n");
    Serial_SendString("Middle: 1s+1grade, 3s→Turn\r\n\r\n");

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
    // 全黑 1111
    if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1)
    {
        if(FirstBlackFlag == 1)
        {
            FirstBlackFlag = 0;
            Go_Ahead();
            Serial_SendString("Action: 1111 First Black -> Go_Ahead\r\n");
        }
        else
        {
            Delay_ms(1500);
            Car_Stop();
            Serial_SendString("Action: 1111 All Black -> Stop After 1.5s\r\n");
        }
        left_grade = 0; right_grade = 0;
        left_time = 0; right_time = 0;
        return;
    }

    // 全白 0000
    else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        if(LastValidState[1]==1 && LastValidState[2]==0) Self_Left();
        else if(LastValidState[1]==0 && LastValidState[2]==1) Self_Right();
        else if(LastValidState[0]==1) Turn_Left();
        else if(LastValidState[3]==1) Turn_Right();
        else Go_Ahead();
        Serial_SendString("Action: 0000 -> Last Action\r\n");
        left_grade = 0; right_grade = 0;
        left_time = 0; right_time = 0;
        return;
    }

    // 1000 急转弯左
    else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        memcpy(LastValidState, TrackNow, 4);
        Turn_Left();
        Serial_SendString("Action: 1000 -> Turn_Left\r\n");
        left_grade = 0; right_grade = 0;
        return;
    }

    // 0001 急转弯右
    else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==1)
    {
        memcpy(LastValidState, TrackNow, 4);
        Turn_Right();
        Serial_SendString("Action: 0001 -> Turn_Right\r\n");
        left_grade = 0; right_grade = 0;
        return;
    }

    // ===================== 分级左转 x10x =====================
    else if(TrackNow[1] == 1 && TrackNow[2] == 0)
    {
        memcpy(LastValidState, TrackNow, 4);
        right_grade = 0; right_time = 0;
        left_time += 20;

        if(left_time >= 1000 && left_grade < 2)
        {
            left_grade++;
            left_time = 0;
            
        }

        if(left_grade >= 2)
        {
            Turn_Left();
            Serial_SendString("Action: Left 3Grade - Turn_Left\r\n");
        }
        else
        {
            Self_Left();
            Serial_SendString("Action: x10x - Self_Left\r\n");
        }
    }

    // ===================== 分级右转 x01x =====================
    else if(TrackNow[1] == 0 && TrackNow[2] == 1)
    {
        memcpy(LastValidState, TrackNow, 4);
        left_grade = 0; left_time = 0;
        right_time += 20;

        if(right_time >= 1000 && right_grade < 2)
        {
            right_grade++;
            right_time = 0;
            
        }

        if(right_grade >= 2)
        {
            Turn_Right();
            Serial_SendString("Action: Right 3Grade - Turn_Right\r\n");
        }
        else
        {
            Self_Right();
            Serial_SendString("Action: x01x - Self_Right\r\n");
        }
    }

    // x11x 直行
    else if(TrackNow[1] == 1 && TrackNow[2] == 1)
    {
        memcpy(LastValidState, TrackNow, 4);
        Go_Ahead();
        Serial_SendString("Action: x11x -> Go_Ahead\r\n");
        left_grade = 0; right_grade = 0;
        left_time = 0; right_time = 0;
    }
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
    Serial_SendString("\r\n");
}
