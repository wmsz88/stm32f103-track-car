#include "stm32f10x.h"
#include "Car.h"
#include "Track.h"
#include "Bluetooth.h"
#include "Delay.h"
#include "PWM.h"
#include "Motor.h"
#include <string.h>

u8 TrackNow[4];
u8 TrackLast[4];
u8 StateStr[5];
u8 LastValidState[4];

u8 FirstBlackFlag = 1;

u8 left_grade = 0;
u8 right_grade = 0;
u32 left_time = 0;
u32 right_time = 0;

// ==================== 【已调稳】柔顺 PD 参数 ====================
int error = 0;
int last_error = 0;
int turn = 0;
float Kp = 12.0f;    // 大幅降低转向力度（解决摆动）
float Kd = 25.0f;   // 加大阻尼（稳稳吸线）
int base_speed = 40;
// ==============================================================

void Send_Track_State(void);
void Track_Control(void);
int Get_Track_Error(void);
void PD_Motor_Control(void);

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
    Serial_SendString("===== 4CH TRACKING CAR PD + GRADE =====\r\n");
    Serial_SendString("2s Grade +1, 3Grade -> Turn\r\n\r\n");

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

int Get_Track_Error(void)
{
    if(TrackNow[1]==1 && TrackNow[2]==0)      return -1;
    else if(TrackNow[1]==0 && TrackNow[2]==1) return +1;
    else if(TrackNow[0]==1)                   return -2;
    else if(TrackNow[3]==1)                   return +2;
    else                                      return 0;
}

void PD_Motor_Control(void)
{
    error = Get_Track_Error();
    turn = (int)(Kp * error + Kd * (error - last_error));
    last_error = error;

    int left = base_speed + turn;
    int right = base_speed - turn;

    // 更宽松的安全限幅，防止抖动
    if(left > 75)  left = 75;
    if(left < 20)  left = 20;   // 电机最低速度，防止停转
    if(right > 75) right = 75;
    if(right < 20) right = 20;

    Motor_SetLeftSpeed(left);
    Motor_SetRightSpeed(right);
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
            Serial_SendString("First Black -> Go Ahead\r\n");
        }
        else
        {
            Delay_ms(1000);
            Car_Stop();
            Serial_SendString("All Black -> Stop After 1.5s\r\n");
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
        Serial_SendString("All White -> Keep Last Action\r\n");
        left_grade = 0; right_grade = 0;
        left_time = 0; right_time = 0;
        return;
    }

    // 1000 急左转
    else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        memcpy(LastValidState, TrackNow, 4);
        Turn_Left();
        Serial_SendString("1000 -> Turn Left\r\n");
        left_grade = 0; right_grade = 0;
        return;
    }

    // 0001 急右转
    else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==1)
    {
        memcpy(LastValidState, TrackNow, 4);
        Turn_Right();
        Serial_SendString("0001 -> Turn Right\r\n");
        left_grade = 0; right_grade = 0;
        return;
    }

    // x10x 分级左转（2秒一级）
    else if(TrackNow[1]==1 && TrackNow[2]==0)
    {
        memcpy(LastValidState, TrackNow, 4);
        right_grade = 0; right_time = 0;
        left_time += 20;

        if(left_time >= 2000 && left_grade < 3)
        {
            left_grade++;
            left_time = 0;
        }

        if(left_grade >= 3)
        {
            Turn_Left();
            Serial_SendString("Left Grade 3 -> Turn Left\r\n");
        }
        else
        {
            PD_Motor_Control();
            Serial_SendString("x10x -> PD Left\r\n");
        }
    }

    // x01x 分级右转（2秒一级）
    else if(TrackNow[1]==0 && TrackNow[2]==1)
    {
        memcpy(LastValidState, TrackNow, 4);
        left_grade = 0; left_time = 0;
        right_time += 20;

        if(right_time >= 2000 && right_grade < 3)
        {
            right_grade++;
            right_time = 0;
        }

        if(right_grade >= 3)
        {
            Turn_Right();
            Serial_SendString("Right Grade 3 -> Turn Right\r\n");
        }
        else
        {
            PD_Motor_Control();
            Serial_SendString("x01x -> PD Right\r\n");
        }
    }

    // x11x 直行
    else if(TrackNow[1]==1 && TrackNow[2]==1)
    {
        memcpy(LastValidState, TrackNow, 4);
        PD_Motor_Control();
        Serial_SendString("x11x -> PD Straight\r\n");
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
