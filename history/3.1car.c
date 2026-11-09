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

void Send_Track_State(void);
void Track_Control(void);

int main(void)
{
    TIM2_PWM_Init();      // PWM初始化
    Car_Init();           // 小车初始化
    Bluetooth_Init();     // 蓝牙初始化
    Track_Init();         // 循迹传感器初始化
    
    Delay_ms(500);
    Serial_SendString("===== 4-Channel Track Car Started =====\r\n");
    
    while(1)
    {
        // 读取4路传感器状态 (1=黑线, 0=白)
        TrackNow[0] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9);   // 左
        TrackNow[1] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8);   // 中左
        TrackNow[2] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);   // 中右
        TrackNow[3] = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);   // 右

        // 状态变化时打印
        if(memcmp(TrackNow, TrackLast, 4) != 0)
        {
            memcpy(TrackLast, TrackNow, 4);
            Send_Track_State();
        }

        Track_Control();
        Delay_ms(20);  // 20ms控制周期
    }
}

void Track_Control(void)
{
    // ========== 按循迹逻辑表执行 ==========
    // 根据传感器布局：[左, 中左, 中右, 右]
    // 规则：1 = 黑色, 0 = 白色
    
    // 1. 理想居中 (0110) → 直行
    if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==0)
    {
        Go_Ahead();
        Serial_SendString("0110 -> Go_Ahead\r\n");
    }
    
    // 2. 左两路黑 (1100) → 急右转
    else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        Turn_Right();
        Serial_SendString("1100 -> Turn_Right\r\n");
    }
    
    // 3. 右两路黑 (0011) → 急左转
    else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==1 && TrackNow[3]==1)
    {
        Turn_Left();
        Serial_SendString("0011 -> Turn_Left\r\n");
    }
    
    // 4. 左+中黑 (1110) → 急左转
    else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==0)
    {
        Self_Left();
        Serial_SendString("1110 -> Turn_Left\r\n");
    }
    
    // 5. 右+中黑 (0111) → 急右转
    else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1)
    {
        Self_Right();
        Serial_SendString("0111 -> Turn_Right\r\n");
    }
    
    // 6. 中左黑 (0100) → 急右转
    else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        Self_Right();
        Serial_SendString("0100 -> Turn_Right\r\n");
    }
    
    // 7. 中右黑 (0010) → 微调左转
    else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==1 && TrackNow[3]==0)
    {
        Self_Left();
        Serial_SendString("0010 -> Self_Left\r\n");
    }
    
    // 8. 左黑 (1000) → 微调右转
    else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
        Turn_Right();
        Serial_SendString("1000 -> Self_Right\r\n");
    }
    
    // 9. 右黑 (0001) → 微调左转
    else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==1)
    {
        Turn_Left();
        Serial_SendString("0001 -> Turn_Left\r\n");
    }
    
    // 10. 中左黑 (1010) → 微调左转
    else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==1 && TrackNow[3]==0)
    {
        Self_Left();
        Serial_SendString("1010 -> Self_Left\r\n");
    }
    
    // 11. 中右黑 (0101) → 微调右转
    else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==0 && TrackNow[3]==1)
    {
        Self_Right();
        Serial_SendString("0101 -> Self_Right\r\n");
    }
    
    // 12. 全黑 (1111) → 停止（到达终点）
    else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1 && TrackNow[3]==1)
    {
		Delay_ms(1000);
        Car_Stop();
        Serial_SendString("1111 -> Car_Stop (Finish)\r\n");
    }
    
    // 13. 全白 (0000) → 直行（掉线保护）
    else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==0)
    {
		Delay_ms(4000);
        Go_Ahead();
        Serial_SendString("0000 -> Go_Ahead (No line)\r\n");
    }
    
    // 14. 十字路口 (1001) → 直行
    else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==0 && TrackNow[3]==1)
    {
        Go_Ahead();
        Serial_SendString("1001 -> Go_Ahead (Intersection)\r\n");
    }
    
    // 默认直行
    else
    {
        Go_Ahead();
        Serial_SendString("Default -> Go_Ahead\r\n");
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
