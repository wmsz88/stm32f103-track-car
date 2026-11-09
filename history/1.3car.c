#include "stm32f10x.h"
#include "Car.h"
#include "Track.h"
#include "Bluetooth.h"
#include "Delay.h"
#include "PWM.h"
#include "Motor.h"
#include <string.h>
#include <stdio.h>

u8 TrackNow[3];   // [0]左 [1]中 [2]右
u8 TrackLast[3];
u8 StateStr[4];

int CurrentSpeed = 20;   // 当前速度，初始20
int TargetSpeed = 20;     // 目标速度

void Send_Track_State(void);
void Send_Error(char *str);
void SetCarSpeed(int speed);  // 设置小车速度（带限幅）
void UpdateSpeed(void);       // 速度渐变更新

// 设置小车速度，自动限制在0-60之间（负号表示前进）
void SetCarSpeed(int speed)
{
    if(speed > 60) speed = 60;
    if(speed < 0) speed = 0;
    Motor_SetLeftSpeed(-speed);
    Motor_SetRightSpeed(-speed);
}

// 速度渐变：每次±10，直到达到目标速度
void UpdateSpeed(void)
{
    if(CurrentSpeed < TargetSpeed)
    {
        CurrentSpeed += 10;
        if(CurrentSpeed > TargetSpeed) CurrentSpeed = TargetSpeed;
        SetCarSpeed(CurrentSpeed);
    }
    else if(CurrentSpeed > TargetSpeed)
    {
        CurrentSpeed -= 10;
        if(CurrentSpeed < TargetSpeed) CurrentSpeed = TargetSpeed;
        SetCarSpeed(CurrentSpeed);
    }
}

int main(void)
{
    TIM2_PWM_Init();   // PWM初始化（必须最先）
    Car_Init();        // 小车/电机初始化
    Bluetooth_Init();  // 蓝牙初始化
    Track_Init();      // 循迹传感器初始化
    
    Delay_ms(500);
    Serial_SendString("===== Track Car Ready (3-Channel, Black=0) =====\r\n");
    Serial_SendString("Speed: 20 (initial)\r\n");

    while(1)
    {
        // 读取3路循迹模块：左=PB6，中=PB7，右=PB8（黑线=0，白线=1）
        TrackNow[0] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);  // 左
        TrackNow[1] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);  // 中
        TrackNow[2] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8);  // 右

        // 状态变化时通过蓝牙上报
        if(memcmp(TrackNow, TrackLast, 3) != 0)
        {
            memcpy(TrackLast, TrackNow, 3);
            Send_Track_State();
        }

        // ===================== 三路循迹行驶逻辑（黑线=0） =====================
        
        // 左右都检测到黑线（左=0，右=0）-> 目标速度60（最快）
        if(TrackNow[0] == 0 && TrackNow[2] == 0)
        {
            TargetSpeed = 60;
            UpdateSpeed();
        }
        // 仅中黑(101) 居中 -> 目标速度50
        else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==1)
        {
            TargetSpeed = 50;
            UpdateSpeed();
        }
        // 全黑(000) 十字路口 -> 目标速度30（减速通过）
        else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0)
        {
            TargetSpeed = 30;
            UpdateSpeed();
        }
        // 全白(111) 无轨迹（黑线在传感器空隙） -> 目标速度20（低速寻线）
        else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1)
        {
            TargetSpeed = 20;
            UpdateSpeed();
        }
        // 以下转向状态：不加速，甚至减速
        // 仅右黑(110) 车右偏 -> 左转
        else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==0)
        {
            TargetSpeed = 20;     // 转向时降低目标速度
            UpdateSpeed();
            Turn_Left();
        }
        // 中+右黑(100) 偏右 -> 左转
        else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==0)
        {
            TargetSpeed = 20;
            UpdateSpeed();
            Turn_Left();
        }
        // 左+中黑(011) 偏左 -> 右转
        else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==1)
        {
            TargetSpeed = 20;
            UpdateSpeed();
            Turn_Right();
        }
        // 仅左黑(010) 偏左 -> 右转
        else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==0)
        {
            TargetSpeed = 20;
            UpdateSpeed();
            Turn_Right();
        }
        // 仅左黑另一种情况(001) 车左偏 -> 右转
        else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==1)
        {
            TargetSpeed = 20;
            UpdateSpeed();
            Turn_Right();
        }

        Delay_ms(50); // 速度渐变需要稍长延时，50ms更新一次速度
    }
}

// 蓝牙发送循迹状态
void Send_Track_State(void)
{
    StateStr[0] = TrackNow[0] + '0';
    StateStr[1] = TrackNow[1] + '0';
    StateStr[2] = TrackNow[2] + '0';
    StateStr[3] = '\0';

    Serial_SendString("Track: ");
    Serial_SendString((char*)StateStr);
    Serial_SendString(" Speed: ");
    
    // 发送当前速度
    char speedStr[10];
    sprintf(speedStr, "%d", CurrentSpeed);
    Serial_SendString(speedStr);
    Serial_SendString("\r\n");
}

// 蓝牙发送错误信息
void Send_Error(char *str)
{
    Serial_SendString(str);
}
