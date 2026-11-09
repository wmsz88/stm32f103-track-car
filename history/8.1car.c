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

// 分级转向计时
u8 left_grade  = 0;
u8 right_grade = 0;
u32 left_time  = 0;
u32 right_time = 0;

// 全黑标记：区分第一次/后续触发
u8 FirstAllBlack = 1;

// PD参数：小弯响应强、直行不晃
int error = 0;
int last_error = 0;
int turn_val = 0;
float Kp = 18.0f;
float Kd = 22.0f;
int base_speed = 35;

void Send_Track_State(void);
void Track_Control(void);
int Get_Error(void);
void PD_Speed_Ctrl(void);

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
    Serial_SendString("===== 4-Channel Track Car + PD =====\r\n");
    Serial_SendString("Logic: 1=Black, 0=White\r\n\r\n");

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

// 计算偏差
int Get_Error(void)
{
    if(TrackNow[1]==1 && TrackNow[2]==0)      return -1;
    else if(TrackNow[1]==0 && TrackNow[2]==1) return 1;
    else if(TrackNow[0]==1)                   return -2;
    else if(TrackNow[3]==1)                   return 2;
    else                                      return 0;
}

// PD 差速控制
void PD_Speed_Ctrl(void)
{
    error = Get_Error();
    turn_val = (int)(Kp * error + Kd * (error - last_error));
    last_error = error;

    int left = base_speed + turn_val;
    int right = base_speed - turn_val;

    if(left > 70)  left = 70;
    if(left < 15)  left = 15;
    if(right > 70) right = 70;
    if(right < 15) right = 15;

    Motor_SetLeftSpeed(left);
    Motor_SetRightSpeed(right);
}

void Track_Control(void)
{
    // 1. 全黑逻辑：第一次直行，第二次及以后1.5s后停车
    if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1)
    {
        if(FirstAllBlack == 1)
        {
            // 第一次全黑：直行，并标记置0
            Go_Ahead();
            FirstAllBlack = 0;
            Serial_SendString("1111 -> First Time Go Ahead\r\n");
        }
        else
        {
            // 后续全黑：延时1.5秒停车
            Delay_ms(1500);
            Car_Stop();
            Serial_SendString("1111 -> Stop After 1.5s\r\n");
        }
        
        // 重置分级计时与档位
        left_grade = 0;
        right_grade = 0;
        left_time = 0;
        right_time = 0;
        return;
    }

    // 2. 全白 保持上次动作
    if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        if(LastValidState[1]==1 && LastValidState[2]==0)      Self_Left();
        else if(LastValidState[1]==0 && LastValidState[2]==1) Self_Right();
        else if(LastValidState[0]==1)                         Turn_Left();
        else if(LastValidState[3]==1)                         Turn_Right();
        else                                                  PD_Speed_Ctrl();
        Serial_SendString("0000 -> Keep\r\n");
        
        left_grade = 0;
        right_grade = 0;
        return;
    }

    // 0100 左中 → 左微调(分级+PD)
    if(TrackNow[0]==0&&TrackNow[1]==1&&TrackNow[2]==0&&TrackNow[3]==0)
    {
        right_grade = 0;
        right_time = 0;
        left_time += 20;
        
        if(left_time >= 2000 && left_grade < 3)
        {
            left_grade++;
            left_time = 0;
        }
        
        if(left_grade >= 3)
        {
            Turn_Left();
            Serial_SendString("0100 -> Grade3 Turn_Left\r\n");
        }
        else
        {
            PD_Speed_Ctrl();
            Serial_SendString("0100 -> PD Left\r\n");
        }
    }

    // 0010 右中 → 右微调(分级+PD)
    else if(TrackNow[0]==0&&TrackNow[1]==0&&TrackNow[2]==1&&TrackNow[3]==0)
    {
        left_grade = 0;
        left_time = 0;
        right_time += 20;
        
        if(right_time >= 2000 && right_grade < 3)
        {
            right_grade++;
            right_time = 0;
        }
        
        if(right_grade >= 3)
        {
            Turn_Right();
            Serial_SendString("0010 -> Grade3 Turn_Right\r\n");
        }
        else
        {
            PD_Speed_Ctrl();
            Serial_SendString("0010 -> PD Right\r\n");
        }
    }

    // 0001 最右 → 急右转
    else if(TrackNow[0]==0&&TrackNow[1]==0&&TrackNow[2]==0&&TrackNow[3]==1)
    { Turn_Right(); Serial_SendString("0001 -> Turn_Right\r\n"); }

    // 0011 右中+最右 → 急右转
    else if(TrackNow[0]==0&&TrackNow[1]==0&&TrackNow[2]==1&&TrackNow[3]==1)
    { Turn_Right(); Serial_SendString("0011 -> Turn_Right\r\n"); }

    // 0101 左中+最右 → 左微调(PD)
    else if(TrackNow[0]==0&&TrackNow[1]==1&&TrackNow[2]==0&&TrackNow[3]==1)
    { PD_Speed_Ctrl(); Serial_SendString("0101 -> PD Left\r\n"); }

    // 0110 双中 → 直行(PD)
    else if(TrackNow[0]==0&&TrackNow[1]==1&&TrackNow[2]==1&&TrackNow[3]==0)
    { PD_Speed_Ctrl(); Serial_SendString("0110 -> PD Go\r\n"); }

    // 0111 左中+右中+最右 → 急右转
    else if(TrackNow[0]==0&&TrackNow[1]==1&&TrackNow[2]==1&&TrackNow[3]==1)
    { Turn_Right(); Serial_SendString("0111 -> Turn_Right\r\n"); }

    // 1000 最左 → 急左转
    else if(TrackNow[0]==1&&TrackNow[1]==0&&TrackNow[2]==0&&TrackNow[3]==0)
    { Turn_Left(); Serial_SendString("1000 -> Turn_Left\r\n"); }

    // 1001 最左+最右 → 直行(PD)
    else if(TrackNow[0]==1&&TrackNow[1]==0&&TrackNow[2]==0&&TrackNow[3]==1)
    { PD_Speed_Ctrl(); Serial_SendString("1001 -> PD Go\r\n"); }

    // 1010 最左+右中 → 右微调(PD)
    else if(TrackNow[0]==1&&TrackNow[1]==0&&TrackNow[2]==1&&TrackNow[3]==0)
    { PD_Speed_Ctrl(); Serial_SendString("1010 -> PD Right\r\n"); }

    // 1011 最左+右中+最右 → 右微调(PD)
    else if(TrackNow[0]==1&&TrackNow[1]==0&&TrackNow[2]==1&&TrackNow[3]==1)
    { PD_Speed_Ctrl(); Serial_SendString("1011 -> PD Right\r\n"); }

    // 1100 最左+左中 → 急左转
    else if(TrackNow[0]==1&&TrackNow[1]==1&&TrackNow[2]==0&&TrackNow[3]==0)
    { Turn_Left(); Serial_SendString("1100 -> Turn_Left\r\n"); }

    // 1101 最左+左中+最右 → 左微调(PD)
    else if(TrackNow[0]==1&&TrackNow[1]==1&&TrackNow[2]==0&&TrackNow[3]==1)
    { PD_Speed_Ctrl(); Serial_SendString("1101 -> PD Left\r\n"); }

    // 1110 最左+左中+右中 → 急左转
    else if(TrackNow[0]==1&&TrackNow[1]==1&&TrackNow[2]==1&&TrackNow[3]==0)
    { Turn_Left(); Serial_SendString("1110 -> Turn_Left\r\n"); }

    // 兜底：保持上次动作
    else
    {
        if(LastValidState[1]==1 && LastValidState[2]==0)      Self_Left();
        else if(LastValidState[1]==0 && LastValidState[2]==1) Self_Right();
        else if(LastValidState[0]==1)                         Turn_Left();
        else if(LastValidState[3]==1)                         Turn_Right();
        else                                                  PD_Speed_Ctrl();
        Serial_SendString("Other -> Keep Last Action\r\n");
        
        left_grade = 0;
        right_grade = 0;
        return;
    }

    // 更新有效状态
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
    Serial_SendString("\r\n");
}
