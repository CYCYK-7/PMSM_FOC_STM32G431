# STM32 学习笔记03

# TIM4 普通 PWM

## 1. 本阶段完成的内容

本阶段完成了 TIM4 普通 PWM 的配置和实机验证：

- 配置 TIM4 Channel 1 为 PWM 输出
- PWM 输出引脚：PB6
- PWM 频率：20 kHz
- 初始占空比：50%
- 使用 Debugger 查看 `ARR`、`CCR1`、`CNT`
- 使用万用表粗略验证 PWM 平均电压
- 分别测试 25%、50%、75% 占空比
- 学会程序运行过程中修改 CCR 改变占空比

整体流程：

```text
170 MHz Timer Clock
↓
TIM4
↓
PSC / ARR
↓
确定 PWM 频率
↓
CCR1
↓
确定 PWM 占空比
↓
PB6 输出 PWM
```

---

## 2. PWM 是什么

PWM 全称：

**Pulse Width Modulation，脉宽调制**

PWM 本质上是一种周期性的高低电平信号。

例如：

```text
HIGH      ┌────────┐
          │        │
LOW ──────┘        └────────────
```

PWM 最重要的两个参数是：

1. **Frequency：频率**
2. **Duty Cycle：占空比**

---

## 3. PWM Frequency

PWM Frequency 表示：

> 每秒产生多少个 PWM 周期。

当前使用：

```text
PWM Frequency = 20 kHz
```

即：

$$
f_{PWM}=20000\text{ Hz}
$$

PWM 周期为：

$$
T_{PWM}=\frac{1}{f_{PWM}}
$$

因此：

$$
T_{PWM}=\frac{1}{20000}=50\mu s
$$

也就是说：

> 每 50 μs 完成一个 PWM 周期。

---

## 4. Duty Cycle

Duty Cycle 表示：

> 一个 PWM 周期中，高电平时间占整个周期的比例。

例如 50% Duty：

```text
HIGH      ┌────────┐
          │        │
LOW ──────┘        └────────
          ← 50% →
```

如果 PWM 周期为：

$$
50\mu s
$$

50% Duty 时：

$$
T_{HIGH}=25\mu s
$$

$$
T_{LOW}=25\mu s
$$

---

## 5. Timer 为什么可以产生 PWM

PWM 并不是 CPU 不断执行：

```c
GPIO = 1;
GPIO = 0;
```

来产生的。

真正的过程是：

```text
Timer Clock
↓
Timer 自动计数
↓
CNT 与 CCR 比较
↓
Timer 硬件自动改变输出状态
↓
产生 PWM
```

因此一旦 PWM 启动：

```c
HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
```

之后 PWM 主要由 Timer 硬件自动产生。

CPU 不需要每个 PWM 周期都手动控制 GPIO。

---

# 6. TIM4 当前配置

当前 TIM4 基本配置：

```text
Timer              = TIM4
Channel            = Channel 1
PWM Mode           = PWM Generation CH1
Output Pin         = PB6
Prescaler          = 0
Counter Period     = 8499
Pulse              = 4250
Counter Mode       = Up
Clock Division     = No Division
```

其中最重要的是：

```text
PSC = 0
ARR = 8499
CCR1 = 4250
```

---

# 7. PSC：Prescaler

PSC 全称：

**Prescaler，预分频器**

作用：

> 对 Timer 输入时钟进行分频。

实际分频系数为：

$$
PSC+1
$$

当前：

```text
PSC = 0
```

所以：

$$
PSC+1=1
$$

因此 Timer Clock 不分频。

当前 Timer Clock：

$$
170\text{ MHz}
$$

因此 Timer Counter 的计数时钟仍然为：

$$
170\text{ MHz}
$$

---

# 8. ARR：Auto Reload Register

ARR 全称：

**Auto Reload Register，自动重装载寄存器**

可以暂时理解为：

> 决定 Timer 一个周期计多少次。

当前：

```text
ARR = 8499
```

Timer 实际计数：

```text
0
1
2
3
...
8499
```

一共：

$$
8499+1=8500
$$

个计数。

计数完成后重新回到 0：

```text
0 → 1 → 2 → ... → 8499
                     ↓
                     0
                     ↓
             再重新开始
```

---

# 9. PWM 频率计算公式

对于当前向上计数模式：

$$
f_{PWM}
=
\frac{f_{TIM}}
{(PSC+1)(ARR+1)}
$$

当前：

$$
f_{TIM}=170\text{ MHz}
$$

$$
PSC=0
$$

$$
ARR=8499
$$

代入：

$$
f_{PWM}
=
\frac{170\times10^6}
{(0+1)(8499+1)}
$$

得到：

$$
f_{PWM}=20000\text{ Hz}
$$

即：

$$
\boxed{f_{PWM}=20\text{ kHz}}
$$

所以需要记住：

> **PSC 和 ARR 共同决定 PWM Frequency。**

---

# 10. CCR：Capture/Compare Register

CCR 全称：

**Capture/Compare Register**

在当前 PWM 模式下，可以简单理解为：

> CCR 决定一个周期中高电平持续多久。

当前使用：

```text
CCR1 = 4250
```

而：

```text
ARR + 1 = 8500
```

所以占空比：

$$
Duty
=
\frac{CCR1}{ARR+1}\times100\%
$$

代入：

$$
Duty
=
\frac{4250}{8500}\times100\%
$$

得到：

$$
\boxed{Duty=50\%}
$$

---

# 11. ARR 和 CCR 的区别

这是 PWM 中非常重要的一组概念。

## ARR

ARR 主要决定：

> PWM 周期和频率。

```text
ARR变化
↓
PWM周期变化
↓
PWM频率变化
```

## CCR

CCR 主要决定：

> PWM 占空比。

```text
CCR变化
↓
高电平时间变化
↓
Duty变化
```

因此现阶段可以直接记：

> **ARR 主要决定 Frequency，CCR 主要决定 Duty。**

---

# 12. 25%、50%、75% Duty 实验

当前：

$$
ARR+1=8500
$$

占空比计算：

$$
CCR=(ARR+1)\times Duty
$$

---

## 25% Duty

$$
CCR=8500\times0.25
$$

得到：

$$
CCR=2125
$$

程序：

```c
__HAL_TIM_SET_COMPARE(&htim4,
                      TIM_CHANNEL_1,
                      2125);
```

此时：

```text
Duty ≈ 25%
Frequency ≈ 20 kHz
```

高电平时间：

$$
50\mu s\times25\%=12.5\mu s
$$

低电平时间：

$$
37.5\mu s
$$

---

## 50% Duty

$$
CCR=8500\times0.5=4250
$$

程序：

```c
__HAL_TIM_SET_COMPARE(&htim4,
                      TIM_CHANNEL_1,
                      4250);
```

此时：

```text
Duty ≈ 50%
Frequency ≈ 20 kHz
```

高低电平时间分别约：

$$
25\mu s
$$

---

## 75% Duty

$$
CCR=8500\times0.75
$$

得到：

$$
CCR=6375
$$

程序：

```c
__HAL_TIM_SET_COMPARE(&htim4,
                      TIM_CHANNEL_1,
                      6375);
```

此时：

```text
Duty ≈ 75%
Frequency ≈ 20 kHz
```

高电平时间：

$$
37.5\mu s
$$

低电平时间：

$$
12.5\mu s
$$

---

# 13. 三种 Duty 对应关系

| Duty | CCR1 | 高电平时间 | 低电平时间 |
| ---: | ---: | ---------: | ---------: |
|  25% | 2125 |    12.5 μs |    37.5 μs |
|  50% | 4250 |      25 μs |      25 μs |
|  75% | 6375 |    37.5 μs |    12.5 μs |

需要注意：

> 三种情况下 PWM Frequency 都保持 20 kHz。

因为：

```text
PSC 没变
ARR 没变
```

只有：

```text
CCR1
```

发生变化。

---

# 14. 程序运行过程中修改占空比

实际工程中不会每次改变占空比都进入 `.ioc` 修改 Pulse。

可以直接修改 CCR：

```c
__HAL_TIM_SET_COMPARE(&htim4,
                      TIM_CHANNEL_1,
                      compare);
```

例如：

```c
__HAL_TIM_SET_COMPARE(&htim4,
                      TIM_CHANNEL_1,
                      2125);
```

就是把 CCR1 改成 2125。

这说明：

> PWM Duty 可以在 MCU 运行过程中实时修改。

这一点以后做电机控制非常重要。

---

# 15. Init 和 Start 的区别

CubeMX 自动生成：

```c
MX_TIM4_Init();
```

作用：

> 配置 TIM4。

例如：

- PSC
- ARR
- Counter Mode
- PWM Mode
- Pulse

但：

```c
MX_TIM4_Init();
```

并不代表 PWM 已经开始输出。

还需要：

```c
HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
```

才真正启动 PWM。

因此需要记住：

```text
MX_TIM4_Init()
↓
配置 PWM

HAL_TIM_PWM_Start()
↓
启动 PWM
```

即：

> **Init ≠ Start**

---

# 16. CNT 是什么

CNT：

**Counter，计数器当前值**

运行时可以在 Debugger 中观察：

```text
TIM4->CNT
```

它会不断：

```text
0
↓
1
↓
2
↓
...
↓
8499
↓
0
↓
...
```

因为 PWM 频率很高，所以 Debugger 中每次暂停看到的 CNT 往往都不同。

当前还可以观察：

```text
TIM4->ARR
TIM4->CCR1
TIM4->CNT
```

例如：

```text
ARR  = 8499
CCR1 = 4250
CNT  = 当前实时计数值
```

---

# 17. GPIO Alternate Function

TIM4_CH1 并不是普通 GPIO Output。

它属于：

**Alternate Function，复用功能**

STM32 一个 GPIO 引脚往往可以具有多种功能，例如：

```text
GPIO
UART
Timer
SPI
I2C
ADC
...
```

TIM4_CH1 也可能映射到不同 GPIO。

例如当前最终使用：

```text
PB6 → TIM4_CH1
```

因此配置 STM32 Pin 时不能简单认为：

> 某个外设永远对应唯一一个 GPIO。

正确思路：

```text
查看 MCU 支持的 Alternate Function
↓
查看开发板原理图
↓
确定实际硬件 Pin
↓
CubeMX 配置
```

---

# 18. 使用万用表粗略验证 PWM

万用表不能真正看到 PWM 方波，但是可以测量 PWM 的平均电压。

测试方法：

```text
万用表 → DC Voltage
黑表笔 → GND
红表笔 → PWM 输出引脚
```

当前 PWM：

```text
LOW  ≈ 0 V
HIGH ≈ 3.3 V
```

平均电压约：

$$
V_{avg}=3.3\times Duty
$$

---

## 25%

$$
V_{avg}=3.3\times0.25
$$

$$
V_{avg}\approx0.825V
$$

---

## 50%

$$
V_{avg}=3.3\times0.5
$$

$$
V_{avg}\approx1.65V
$$

---

## 75%

$$
V_{avg}=3.3\times0.75
$$

$$
V_{avg}\approx2.475V
$$

因此万用表理论上可以看到：

| Duty | 平均电压 |
| ---: | -------: |
|  25% | ≈ 0.83 V |
|  50% | ≈ 1.65 V |
|  75% | ≈ 2.48 V |

实际测量存在一些误差属于正常现象。

---

# 19. 万用表不能验证 PWM Frequency

需要注意：

如果万用表显示：

```text
1.65 V
```

只能大致说明：

> 当前信号平均电压约为 1.65 V。

不能证明：

```text
PWM Frequency = 20 kHz
Duty = 50%
```

因为一个稳定的 1.65 V DC 电压，万用表也会显示 1.65 V。

真正观察 PWM 波形应该使用：

- 示波器
- 逻辑分析仪

可以直接观察：

```text
Frequency
Period
High Time
Low Time
Duty Cycle
```

---

# 20. PWM 与以后 SVPWM 的关系

当前只有一路 PWM：

```text
Duty
↓
CCR1
↓
TIM4_CH1
↓
PWM
```

以后 PMSM FOC 中会变成三相：

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
三相逆变器
↓
PMSM
```

因此：

> SVPWM 算法最终必须落实到 Timer 的 CCR 上。

可以记住：

```text
SVPWM
↓
Duty_A / Duty_B / Duty_C
↓
CCR1 / CCR2 / CCR3
↓
PWM
```

这也是学习普通 PWM 的重要意义。

---

# 21. 本阶段最需要记住的知识

1. PWM 最重要的两个参数：

   - Frequency
   - Duty Cycle

2. PWM Frequency：

   $$
   f_{PWM}
   =
   \frac{f_{TIM}}
   {(PSC+1)(ARR+1)}
   $$

3. PWM Duty：

   $$
   Duty
   =
   \frac{CCR}{ARR+1}\times100\%
   $$

4. 当前配置：

   ```text
   Timer Clock = 170 MHz
   PSC = 0
   ARR = 8499
   ```

   得到：

   $$
   f_{PWM}=20\text{ kHz}
   $$

5. 当前：

   ```text
   CCR1 = 4250
   ```

   对应：

   $$
   Duty=50\%
   $$

6. 可以简单记：

   > **PSC + ARR 决定频率，CCR 决定占空比。**

7. 修改 CCR 可以在程序运行过程中实时改变 Duty。

8. `MX_TIM4_Init()`：

   > 配置 Timer。

9. `HAL_TIM_PWM_Start()`：

   > 真正启动 PWM。

10. PWM 是 Timer 硬件自动产生的，不需要 CPU 不断手动翻转 GPIO。

11. PWM 输出 Pin 属于 GPIO Alternate Function。

12. 万用表只能粗略观察平均电压，真正观察 PWM 应使用示波器或逻辑分析仪。

---

# 22. 与后续学习的关系

普通 PWM 是后续学习以下内容的基础：

```text
普通 PWM
↓
三相 PWM
↓
TIM1 Advanced Timer
↓
Complementary PWM
↓
Dead Time
↓
SVPWM
↓
PMSM FOC
```

因此当前最核心的是彻底理解：

$$
\boxed{
Clock
\rightarrow PSC
\rightarrow ARR
\rightarrow PWM Frequency
}
$$

以及：

$$
\boxed{
CCR
\rightarrow Duty Cycle
}
$$