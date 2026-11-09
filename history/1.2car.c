#include "stm32f10x.h"
#include "Car.h"
#include "Track.h"
#include "Bluetooth.h"
#include "Delay.h"
#include "PWM.h"
#include "Motor.h"
#include <string.h>

u8 TrackNow[3];   // [0]左 [1]中 [2]右
u8 TrackLast[3];
u8 StateStr[4];

void Send_Track_State(void);
void Send_Error(char *str);

int main(void)
{
    TIM2_PWM_Init();   // PWM初始化（必须最先）
    Car_Init();        // 小车/电机初始化
    Bluetooth_Init();  // 蓝牙初始化
    Track_Init();      // 循迹传感器初始化
    
    Delay_ms(500);
    Serial_SendString("===== Track Car Ready (3-Channel, Black=0) =====\r\n");

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
        
        // 左右都检测到黑线（左=0，右=0）-> 高速90直行（黑线在中间）
        if(TrackNow[0] == 0 && TrackNow[2] == 0)
        {
            Motor_SetLeftSpeed(-90);   // 高速90
            Motor_SetRightSpeed(-90);
        }
        // 全白(111) 无轨迹（黑线在传感器空隙） -> 低速直行，继续寻线
        else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==1)
        {
            Motor_SetLeftSpeed(-20);   // 低速寻线
            Motor_SetRightSpeed(-20);
        }
        // 仅右黑(110) 车右偏 -> 左转
        else if(TrackNow[0]==1 && TrackNow[1]==1 && TrackNow[2]==0)
        {
            Turn_Left();
        }
        // 仅中黑(101) 居中 -> 中速50直行
        else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==1)
        {
            Motor_SetLeftSpeed(-50);   // 中速50
            Motor_SetRightSpeed(-50);
        }
        // 中+右黑(100) 偏右 -> 左转
        else if(TrackNow[0]==1 && TrackNow[1]==0 && TrackNow[2]==0)
        {
            Turn_Left();
        }
        // 左+中黑(011) 偏左 -> 右转
        else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==1)
        {
            Turn_Right();
        }
        // 仅左黑(010) 偏左 -> 右转
        else if(TrackNow[0]==0 && TrackNow[1]==1 && TrackNow[2]==0)
        {
            Turn_Right();
        }
        // 仅左黑另一种情况(001) 车左偏 -> 右转
        else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==1)
        {
            Turn_Right();
        }
        // 全黑(000) 十字路口 -> 低速20直行
        else if(TrackNow[0]==0 && TrackNow[1]==0 && TrackNow[2]==0)
        {
            Motor_SetLeftSpeed(-20);
            Motor_SetRightSpeed(-20);
        }

        Delay_ms(20); // 控制循迹刷新速度
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
    Serial_SendString(" (1=White,0=Black)\r\n");
}

// 蓝牙发送错误信息
void Send_Error(char *str)
{
    Serial_SendString(str);
}
