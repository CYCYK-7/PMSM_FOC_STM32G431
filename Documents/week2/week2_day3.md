# STM32 学习笔记：TIM4 普通 PWM

## 1. 今天完成的内容

今天完成了：

```text
TIM4_CH1 配置
↓
20 kHz PWM
↓
启动 PWM 输出
↓
Debugger 检查 ARR / CCR / CNT
↓
万用表实机验证
↓
25% / 50% / 75% 占空比测试
```

当前已经验证：

```text
PWM Frequency ≈ 20 kHz
Duty = 25%   ✅
Duty = 50%   ✅
Duty = 75%   ✅
```

同时已经开始进入：

```text
ADC1 基础采样
```

但 ADC 还没有完成实机采样，所以留到下一次继续。

------

# 2. Timer 产生 PWM 的基本原理

PWM 不是 CPU 不断执行：

```c
GPIO = 1;
GPIO = 0;
```

产生的。

而是：

```text
Timer Clock
↓
定时器自动计数
↓
CNT 与 CCR 比较
↓
硬件自动控制 GPIO
↓
产生 PWM
```

所以一旦：

```c
HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
```

启动成功，Timer 就可以自己持续产生 PWM。

CPU 不需要每个 PWM 周期都手动翻转 GPIO。

这个概念以后做电机控制非常重要。

------

# 3. PWM 最重要的三个参数

目前最需要熟记：

```text
PSC
ARR
CCR
```

### PSC：Prescaler

预分频器。

决定 Timer 计数之前先把输入时钟除多少。

实际分频：

PSC+1PSC+1

当前：

```text
PSC = 0
```

所以：

0+1=10+1=1

不分频。

------

### ARR：Auto Reload Register

可以简单理解成：

> 一个 PWM 周期需要计多少次。

当前：

```text
ARR = 8499
```

实际一个周期：

ARR+1=8500ARR+1=8500

个计数。

------

### CCR：Capture/Compare Register

在 PWM 模式下，可以简单理解成：

> 决定高电平维持多久，也就是控制占空比。

例如：

```text
CCR1 = 4250
```

配合：

```text
ARR + 1 = 8500
```

得到：

Duty=42508500=50%Duty=\frac{4250}{8500}=50\%

------

# 4. PWM 频率公式

当前使用：

```text
Timer Clock = 170 MHz
PSC = 0
ARR = 8499
```

PWM 频率：

fPWM=fTIM(PSC+1)(ARR+1)f_{PWM} = \frac{f_{TIM}} {(PSC+1)(ARR+1)}

代入：

fPWM=170×106(0+1)(8499+1)f_{PWM} = \frac{170\times10^6} {(0+1)(8499+1)}

得到：

20 kHz\boxed{20\,kHz}

对应周期：

T=120000=50μsT=\frac1{20000}=50\mu s

所以必须记住：

> **PSC 和 ARR 决定 PWM 频率。**

------

# 5. PWM 占空比公式

向上计数 PWM 当前可以近似记成：

Duty=CCRARR+1×100%Duty = \frac{CCR}{ARR+1}\times100\%

因为：

```text
ARR + 1 = 8500
```

所以：

| Duty | CCR1 |
| ---- | ---- |
| 25%  | 2125 |
| 50%  | 4250 |
| 75%  | 6375 |

今天已经实际验证这三个值。

最重要的结论：

ARR主要决定频率，CCR主要决定占空比\boxed{ARR主要决定频率，CCR主要决定占空比}

这个以后必须记住。

------

# 6. 改变 CCR 不会改变 PWM 频率

例如：

```text
CCR = 2125
↓
25%

CCR = 4250
↓
50%

CCR = 6375
↓
75%
```

但：

```text
PSC = 0
ARR = 8499
```

一直没有变化。

所以 PWM 始终：

```text
20 kHz
```

只是高电平持续时间发生变化。

例如：

### 25%

```text
周期       50 μs
HIGH约     12.5 μs
LOW约      37.5 μs
```

### 50%

```text
HIGH约     25 μs
LOW约      25 μs
```

### 75%

```text
HIGH约     37.5 μs
LOW约      12.5 μs
```

------

# 7. 程序运行时修改占空比

不需要每次为了改 Duty 都进入 `.ioc`。

可以直接：

```c
__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 2125);
```

或者：

```c
__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 4250);
```

或者：

```c
__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 6375);
```

本质就是：

> 修改 TIM4 Channel 1 的 CCR1。

以后真实电机运行过程中 PWM 占空比本来就是实时变化的，所以这种方式比反复修改 `.ioc` 更接近真实工程。

------

# 8. `MX_TIM4_Init()` 和 `HAL_TIM_PWM_Start()` 的区别

这个必须记住。

```c
MX_TIM4_Init();
```

表示：

> 把 TIM4 的 PSC、ARR、PWM Mode、CCR 等参数配置好。

但：

> **Init 不等于 Timer 已经开始输出 PWM。**

还需要：

```c
HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
```

才真正：

> 启动 TIM4 CH1 PWM。

所以：

```text
MX_TIM4_Init()
↓
配置

HAL_TIM_PWM_Start()
↓
运行
```

可以简单记：

Init≠Start\boxed{Init\neq Start}

------

# 9. 引脚复用 AF

今天 TIM4_CH1 一开始 CubeMX 自动选择了：

```text
PA11
```

后来改用了：

```text
PB6
```

这里学到一个很重要的 STM32 知识：

> 一个外设功能可能能够映射到多个 GPIO。

例如：

```text
TIM4_CH1
├── PA11
└── PB6
```

这属于：

> **Alternate Function，复用功能。**

因此实际工程不能单纯看 CubeMX 自动选择哪个 Pin，而要结合：

```text
MCU支持的复用功能
+
开发板原理图
```

确定最终 Pin。

------

# 10. 万用表怎么粗略验证 PWM

万用表使用：

```text
DC Voltage
直流电压档
```

连接：

```text
黑表笔 → GND
红表笔 → PWM输出引脚
```

PWM 为：

```text
LOW = 0 V
HIGH ≈ 3.3 V
```

那么万用表测到的平均电压大约：

Vavg≈3.3×DutyV_{avg}\approx3.3\times Duty

因此理论上：

| Duty | 平均电压 |
| ---- | -------- |
| 25%  | ≈0.825 V |
| 50%  | ≈1.65 V  |
| 75%  | ≈2.475 V |

今天通过这种方式可以粗略验证 Duty 是否发生变化。

但是必须知道：

> **万用表不能真正验证 PWM 频率。**

例如万用表显示：

```text
1.65 V
```

无法证明它一定是：

```text
20 kHz + 50% PWM
```

因为稳定的 1.65 V 直流也可能显示一样。

以后真正验证 PWM：

> **示波器 / 逻辑分析仪更专业。**

------

# 11. PWM 与以后 SVPWM 的关系

今天这个实验和以后 PMSM 控制直接相关。

现在：

```text
一个 Duty
↓
CCR1
↓
一路 PWM
```

以后 SVPWM：

```text
SVPWM
↓
Duty_A
Duty_B
Duty_C
↓
CCR1
CCR2
CCR3
↓
三相 PWM
↓
逆变器
↓
PMSM
```

所以 SVPWM 最终算出的结果并不是“直接控制电机”。

它最终还是要转换成：

```text
PWM Duty
```

然后写入 Timer 的 CCR。

这个关系以后一定要能够直接想到：

SVPWM→DutyA,B,C→CCR1,2,3→PWM\boxed{ SVPWM \rightarrow Duty_{A,B,C} \rightarrow CCR_{1,2,3} \rightarrow PWM }

------

# 12. 今天开始接触 ADC

PWM 完成以后，下一阶段正式进入：

```text
ADC
↓
ADC + DMA
↓
电流采样
↓
CubeMonitor
↓
PI / Clarke / Park / FOC
```

ADC 基础阶段准备使用：

```text
ADC1
ADC1_IN1
PA0
```

先完成：

```text
模拟电压
↓
ADC
↓
0~4095 数字量
↓
UART打印
```

12 bit ADC 最重要的基础关系：

212=40962^{12}=4096

所以数字范围：

0∼4095\boxed{0\sim4095}

如果参考电压约 3.3 V：

VADC=ADCraw4095×3.3V_{ADC} = \frac{ADC_{raw}}{4095}\times3.3

例如：

```text
ADC_raw ≈ 2048
```

则：

VADC≈1.65VV_{ADC}\approx1.65V

这部分今天先有概念，**下一次从 ADC1 配置和第一次实际读取 `adc_raw` 正式继续。**

------

## 当前学习进度

```text
GPIO / LED                 ✅
USART3                      ✅
USB-TTL 串口                ✅
Timer 1 ms Interrupt        ✅
Timer + UART Flag           ✅
普通 PWM                    ✅
20 kHz PWM                  ✅
25 / 50 / 75% Duty          ✅
PWM 实机粗测                 ✅

ADC基础                      ▶ 下一步
ADC + DMA                   ⬜
电流采样                     ⬜
CubeMonitor                  ⬜
```

今天到这里正好是一个很合适的收口点：**基础数字外设已经从 GPIO → UART → Timer → PWM 连起来了，下一次正式跨入模拟量采集 ADC。**