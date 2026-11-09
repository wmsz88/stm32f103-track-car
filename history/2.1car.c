#include "stm32f10x.h"
#include "Car.h"
#include "Track.h"
#include "Bluetooth.h"
#include "Delay.h"
#include "PWM.h"
#include <string.h>

u8 TrackNow[4];   // [0]左(PB9) [1]中左(PB8) [2]中右(PB7) [3]右(PB6)
u8 TrackLast[4];
u8 StateStr[5];
u8 LastValidState[4];

void Send_Track_State(void);
void Send_Error(char *str);
void Track_Control(void);

int main(void)
{
    TIM2_PWM_Init();
    Car_Init();
    Bluetooth_Init();
    Track_Init();
    
    // 初始化为直行状态（1001）
    LastValidState[0] = 1;
    LastValidState[1] = 0;
    LastValidState[2] = 0;
    LastValidState[3] = 1;
    
    Delay_ms(500);
    Serial_SendString("===== 4-Channel Track Car Control =====\r\n");
    Serial_SendString("Logic: Sensor Black -> Turn to that side\r\n");
    Serial_SendString("Left sensors black -> Turn Left, Right sensors black -> Turn Right\r\n\r\n");

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
    // ========== 优先级1：全黑(0000) ==========
    if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        Go_Ahead();
        Serial_SendString("Action: 0000 All Black -> Go Ahead\r\n");
    }
    
    // ========== 优先级2：全白(1111) ==========
    else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1)
    {
        if(LastValidState[0]==0 && LastValidState[1]==0 && LastValidState[2]==0 && LastValidState[3]==0)
        {
            Go_Ahead();
            Serial_SendString("Action: 1111 No Track -> LastState(0000) Go Ahead\r\n");
        }
        else if(LastValidState[0]==1 && LastValidState[1]==0 && LastValidState[2]==0 && LastValidState[3]==1)
        {
            Go_Ahead();
            Serial_SendString("Action: 1111 No Track -> LastState(1001) Go Ahead\r\n");
        }
        else if(LastValidState[1]==0 || LastValidState[0]==0)  // 左侧黑线
        {
            Self_Left();
            Serial_SendString("Action: 1111 No Track -> LastState(Left Black) Self_Left\r\n");
        }
        else if(LastValidState[2]==0 || LastValidState[3]==0)  // 右侧黑线
        {
            Self_Right();
            Serial_SendString("Action: 1111 No Track -> LastState(Right Black) Self_Right\r\n");
        }
        else
        {
            Go_Ahead();
            Serial_SendString("Action: 1111 No Track -> Default Go Ahead\r\n");
        }
    }
    
    // ========== 优先级3：严格按自定义循迹逻辑执行 ==========
    else
    {
        memcpy(LastValidState, TrackNow, 4);
        
        // ===================== 直行 =====================
        // 理想居中：1001
        if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==1)
        {
            Go_Ahead();
            Serial_SendString("Action: 1001 Perfect Center -> Go Ahead\r\n");
        }
        // 十字路口：0110
        else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==0)
        {
            Go_Ahead();
            Serial_SendString("Action: 0110 Intersection -> Go Ahead\r\n");
        }
        
        // ===================== 左转（车身偏右） =====================
        // 0011 左两路黑 → 左转
        else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==1 && TrackNow[3]==1)
        {
            Turn_Left();
            Serial_SendString("Action: 0011 Left Two Black -> Turn_Left\r\n");
        }
        // 0001 左+中黑 → 左转
        else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==1)
        {
            Turn_Left();
            Serial_SendString("Action: 0001 Left+Mid Black -> Turn_Left\r\n");
        }
        // 0010 中左黑 → 左转
        else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==1 && TrackNow[3]==0)
        {
            Turn_Left();
            Serial_SendString("Action: 0010 MidLeft Black -> Turn_Left\r\n");
        }
        // 0111 左黑 → 左转
        else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1)
        {
            Self_Left();
            Serial_SendString("Action: 0111 Left Black -> Self_Left\r\n");
        }
        // 1010 中左黑 → 左转
        else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==1 && TrackNow[3]==0)
        {
            Self_Left();
            Serial_SendString("Action: 1010 MidLeft Black -> Self_Left\r\n");
        }
        
        // ===================== 右转（车身偏左） =====================
        // 1100 右两路黑 → 右转
        else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==0 && TrackNow[3]==0)
        {
            Turn_Right();
            Serial_SendString("Action: 1100 Right Two Black -> Turn_Right\r\n");
        }
        // 1000 右+中黑 → 右转
        else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
        {
            Turn_Right();
            Serial_SendString("Action: 1000 Right+Mid Black -> Turn_Right\r\n");
        }
        // 0100 中右黑 → 右转
        else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==0 && TrackNow[3]==0)
        {
            Self_Right();
            Serial_SendString("Action: 0100 MidRight Black -> Self_Right\r\n");
        }
        // 1110 右黑 → 右转
        else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==0)
        {
            Self_Right();
            Serial_SendString("Action: 1110 Right Black -> Self_Right\r\n");
        }
        // 0101 中右黑 → 右转
        else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==0 && TrackNow[3]==1)
        {
            Self_Right();
            Serial_SendString("Action: 0101 MidRight Black -> Self_Right\r\n");
        }
        
        // ===================== 默认直行 =====================
        else
        {
            Go_Ahead();
            Serial_SendString("Action: Default State -> Go Ahead\r\n");
        }
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
    Serial_SendString(" (1=White,0=Black)\r\n");
}

void Send_Error(char *str)
{
    Serial_SendString(str);
}
