#include "stm32f10x.h"
#include "Car.h"
#include "Track.h"
#include "Bluetooth.h"
#include "Delay.h"   // 使用你的延时头文件
#include "PWM.h"
#include "Motor.h"
#include <string.h>

u8 TrackNow[4];
u8 TrackLast[4];
u8 StateStr[5];
u8 LastValidState[4];

// ====================== 正确定义全局变量 ======================
u8 Speed = 20;        // 初始速度 20
u16 SpeedTimer = 0;   // 计时变量

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
    
    Delay_ms(500);  // 使用你的延时函数
    Serial_SendString("===== 4-Channel Track Car Control =====\r\n");
    Serial_SendString("Logic: 1=Black, 0=White\r\n");
    Serial_SendString("Straight: +10 per sec, Min 20, Max 70\r\n\r\n");

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
        Delay_ms(20);  // 使用你的延时函数
    }
}

void Track_Control(void)
{
    // 全黑停车
    if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1)
    {
        Speed = 20;
        SpeedTimer = 0;
        Delay_ms(1000);  // 使用你的延时
        Car_Stop();
        Serial_SendString("Action: 1111 All Black -> Car Stop\r\n");
    }

    // 全白 → 沿用上次动作
    else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        Speed = 20;
        SpeedTimer = 0;
        
        if(LastValidState[1]==1 && LastValidState[2]==0)
        {
            Self_Left();
            Serial_SendString("Action: 0000 -> Last Self_Left\r\n");
        }
        else if(LastValidState[1]==0 && LastValidState[2]==1)
        {
            Self_Right();
            Serial_SendString("Action: 0000 -> Last Self_Right\r\n");
        }
        else if(LastValidState[0]==1 && LastValidState[1]==0 && LastValidState[2]==0 && LastValidState[3]==0)
        {
            Turn_Left();
            Serial_SendString("Action: 0000 -> Last Turn_Left\r\n");
        }
        else if(LastValidState[0]==0 && LastValidState[1]==0 && LastValidState[2]==0 && LastValidState[3]==1)
        {
            Turn_Right();
            Serial_SendString("Action: 0000 -> Last Turn_Right\r\n");
        }
        else
        {
            Go_Ahead();
            Serial_SendString("Action: 0000 -> Last Go_Ahead\r\n");
        }
    }

    // 1000 → 急转弯左
    else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        memcpy(LastValidState, TrackNow, 4);
        Speed = 20;
        SpeedTimer = 0;
        Turn_Left();
        Serial_SendString("Action: 1000 -> Turn_Left\r\n");
    }

    // 0001 → 急转弯右
    else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==1)
    {
        memcpy(LastValidState, TrackNow, 4);
        Speed = 20;
        SpeedTimer = 0;
        Turn_Right();
        Serial_SendString("Action: 0001 -> Turn_Right\r\n");
    }

    // x10x → 左转
    else if(TrackNow[1] == 1 && TrackNow[2] == 0)
    {
        memcpy(LastValidState, TrackNow, 4);
        Speed = 20;
        SpeedTimer = 0;
        Self_Left();
        Serial_SendString("Action: x10x -> Self_Left\r\n");
    }

    // x01x → 右转
    else if(TrackNow[1] == 0 && TrackNow[2] == 1)
    {
        memcpy(LastValidState, TrackNow, 4);
        Speed = 20;
        SpeedTimer = 0;
        Self_Right();
        Serial_SendString("Action: x01x -> Self_Right\r\n");
    }

    // ====================== 直行加速 ======================
    // x11x → 直行，每秒+10，最高70
    else if(TrackNow[1] == 1 && TrackNow[2] == 1)
    {
        memcpy(LastValidState, TrackNow, 4);
        
        // 计时 1 秒加速一次
        SpeedTimer++;
		if(SpeedTimer >= 50)
		{
			SpeedTimer = 0;
			if(Speed < 70)
				Speed += 10;
		}
		Motor_SetLeftSpeed(Speed);
		Motor_SetRightSpeed(Speed);
		
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
    Serial_SendString(" (1=Black,0=White)\r\n");
}
