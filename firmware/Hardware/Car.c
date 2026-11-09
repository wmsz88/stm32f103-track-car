#include "stm32f10x.h"
#include "Car.h"
#include "Motor.h"
#include "PWM.h"

void Car_Init(void)
{
    Motor_Init();  // 初始化电机驱动
    Car_Stop();    // 初始停止
}

// ===================== 标准正常运动逻辑 =====================
// 前进：左右电机正转 速度一致
void Go_Ahead(void)
{
    Motor_SetLeftSpeed(60);    // 左轮正转
    Motor_SetRightSpeed(60);   // 右轮正转
}

// 后退
void Go_Back(void)
{
    Motor_SetLeftSpeed(-40);   // 左轮反转
    Motor_SetRightSpeed(-40);  // 右轮反转
}

// 定点左转（原地左转/急转弯）：左轮停，右轮正转
void Turn_Left(void)
{
    Motor_SetLeftSpeed(-60);     // 左轮停止
    Motor_SetRightSpeed(60);   // 右轮正转
}

// 定点右转（原地右转/急转弯）：右轮停，左轮正转
void Turn_Right(void)
{
    Motor_SetLeftSpeed(60);    // 左轮正转
    Motor_SetRightSpeed(-60);    // 右轮停止
}

// 微调左转（循迹小幅度左转）：左轮慢，右轮快
void Self_Left(void)
{
    Motor_SetLeftSpeed(40);    // 左轮慢
    Motor_SetRightSpeed(60);   // 右轮快
}

// 微调右转（循迹小幅度右转）：左轮快，右轮慢
void Self_Right(void)
{
    Motor_SetLeftSpeed(40);    // 左轮快
    Motor_SetRightSpeed(60);   // 右轮慢
}

// 停止
void Car_Stop(void)
{
    Motor_SetLeftSpeed(0);
    Motor_SetRightSpeed(0);
}
