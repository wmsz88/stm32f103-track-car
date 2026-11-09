# STM32F103 循迹小车 · 4 路传感 + PD 差速

> STM32F103C8T6 + 标准外设库（SPL）+ 4 路光电循迹 + PD 差速控制
> 从 19 个迭代版本收敛而来，最终版为 `firmware/`（对应本地 8.1car）

---

## 项目做了什么

小车沿黑线自主行驶：4 路光电传感器读线位 → 计算偏差 → PD 控制器输出左右轮差速 → 电机 PWM 调速。

**三个版本阶段的算法演进**（每版 `main.c` 都留在 `history/`）：

| 阶段 | 做法 | 问题 |
|---|---|---|
| v1.0 | 4 路循迹 + `if/else` 硬编码转向 | 直道抖动，弯道丢线 |
| v5.0 | 5 路循迹 + 状态码（`01111` / `10111` / `11011` / `11101` / `11110`） | 更细，但仍是"档位式"开关控制 |
| **v8.1** | **4 路 + PD 差速**（Kp=18、Kd=22、base=35、限幅 15–70） | 连续量输出，直道稳、小弯响应快 |

> 这条演进线本身就是项目最有价值的部分：**从规则式控制到反馈式控制**。

---

## 硬件

| 部件 | 说明 |
|---|---|
| 主控 | STM32F103C8T6（72MHz） |
| 电机驱动 | 双路直流电机 + PWM 调速（TIM2） |
| 循迹 | 4 路光电，PB9/PB8/PB7/PB6（左 → 右） |
| 蓝牙/串口 | USART1，PA9(TX) / PA10(RX)，9600 8N1，中断接收 |
| 其他模块 | 超声波（`Hardware/Ultrasonic.c`）、按键（`Hardware/Key.c`） |

> ⚠️ **本版没有做蓝牙遥控指令解析**：USART1 主要用来**回传调试信息**（循迹状态、转向档位、错误码）。
> 超声波模块保留在 `Hardware/` 下，但未接入 v8.1 的主循环。
> 这两点是真的没做，不写在简历里。

---

## 核心算法

### 偏差计算（4 路）

```c
if (TrackNow[1]==1 && TrackNow[2]==0) return -1;   // 偏左
if (TrackNow[1]==0 && TrackNow[2]==1) return  1;   // 偏右
if (TrackNow[0]==1)                   return -2;   // 大偏左
if (TrackNow[3]==1)                   return  2;   // 大偏右
return 0;                                          // 居中
```

### PD 差速

```c
turn_val = Kp * error + Kd * (error - last_error);
left  = base_speed + turn_val;
right = base_speed - turn_val;
// 限幅 15 ~ 70，防止一侧反转/堵转
```

**为什么用 PD 不用 PID**：循迹是**无静差跟踪**任务（偏差为 0 就直行），
积分项在直道上只会累积导致超调，所以只用 P 保证响应、D 抑制震荡。

### 全黑处理（比赛用）

4 路全黑 = 到达停止线。逻辑是**第一次直行通过，第二次（及以后）延时 1.5s 后停车**——
用 `FirstAllBlack` 标志区分首末次通过，避免"一压线就停"。

---

## 目录

```
firmware/            Keil MDK 工程（可直接打开 Project.uvprojx）
├── Start/           CMSIS 启动文件
├── Library/         标准外设库
├── System/          Delay 等系统层
├── Hardware/        PWM / Motor / Car / Track / Bluetooth / Ultrasonic / Key
├── User/            main.c、stm32f10x_it.c
└── Project.uvprojx
history/             19 个版本的 main.c（v1.0 → v8.1），用于对比算法演进
```

---

## 编译

1. Keil MDK 打开 `firmware/Project.uvprojx`
2. 编译目标：`STM32F103C8`
3. 烧录：ST-Link / 串口 ISP

> 仓库内**不含** `Objects/`、`Listings/`、`.uvguix.*`、`.uvoptx` 等本地构建产物与个人配置。

---

## 已知局限（下一版要补的）

| 问题 | 影响 | 改进方向 |
|---|---|---|
| 无编码器反馈 | 只能"按占空比开环跑"，坡道/电池压降会跑偏 | 加编码器 + 速度闭环（见 `f407-rtos-motor-control`） |
| 主循环 `Delay_ms(20)` 阻塞 | 采样与控制耦合，抖动大 | 定时器中断或 RTOS 任务 |
| 蓝牙只发不收 | 无法遥控 | 加指令解析（`Serial_GetRxData` 已备好但未接） |
| 超声波未接入 | 无避障 | 接入主循环做优先级仲裁 |
