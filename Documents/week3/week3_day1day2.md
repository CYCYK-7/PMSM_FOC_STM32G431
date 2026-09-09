# STM32 学习笔记：TIM1 三相互补 PWM 与 PWM 同步电流采样

## 1. 当前学习主线

这一阶段完成的整体链路：

```text
U/V 电流 ADC + DMA
↓
零电流 Offset 校准
↓
TIM1 三相中心对齐 PWM
↓
互补 PWM
↓
Dead Time
↓
TIM1 CH4 建立采样时刻
↓
ADC Injected
↓
PWM 同步采样
↓
ADC Interrupt
↓
同步得到 Iu / Iv
↓
Offset Compensation
↓
真实电流
```

---

# 2. TIM1 为什么用于电机控制

普通 PWM 实验使用过 TIM4。

真正三相电机控制开始使用 TIM1，因为 TIM1 属于高级定时器，可以支持：

- 三相 PWM
- 互补输出
- Dead Time
- Break 保护
- 中心对齐 PWM
- 定时触发 ADC

FOC 最终控制链：

```text
FOC
↓
Duty A/B/C
↓
CCR1/CCR2/CCR3
↓
TIM1
↓
三相 PWM
↓
Gate Driver
↓
MOSFET
↓
PMSM
```

---

# 3. TIM1 实际引脚

CubeMX 自动选择的引脚不一定符合 PCB。

当前开发板实际连接：

```text
PA8  → TIM1_CH1
PA9  → TIM1_CH2
PA10 → TIM1_CH3

PB13 → TIM1_CH1N
PB14 → TIM1_CH2N
PB15 → TIM1_CH3N
```

重要原则：

> Datasheet 告诉 MCU 某个引脚“能不能实现某功能”，原理图决定这块 PCB “实际应该使用哪个引脚”。

所以实际开发时：

```text
CubeMX
+
MCU Datasheet
+
开发板原理图
```

要结合起来看。

---

# 4. TIM1 中心对齐 PWM

当前设置：

```text
TIM1 Clock = 170 MHz
PSC = 0
ARR = 4249
Counter Mode = Center Aligned Mode 1
```

中心对齐计数：

```text
0
↑
↑
ARR
↓
↓
0
```

和普通向上计数不同，一个完整 PWM 周期包含：

```text
上数
+
下数
```

PWM 频率近似：

$$
f_{PWM}
=
\frac{f_{TIM}}
{2(PSC+1)(ARR+1)}
$$

当前：

$$
f_{PWM}
\approx20kHz
$$

---

# 5. CCR 与占空比

当前：

$$
ARR+1=4250
$$

所以：

```text
20% → CCR ≈ 850
50% → CCR ≈ 2125
80% → CCR ≈ 3400
```

基本关系：

$$
Duty\approx\frac{CCR}{ARR+1}
$$

因此：

> ARR 主要决定 PWM 周期，CCR 决定占空比。

---

# 6. 三相互补 PWM

TIM1 输出：

```text
CH1  / CH1N
CH2  / CH2N
CH3  / CH3N
```

例如：

```text
CH1  → 上桥臂
CH1N → 下桥臂
```

两路基本互补。

启动时主通道和互补通道需要分别 Start：

```c
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
```

所以再次记住：

> Init ≠ Start。

而且：

> 配置了 CH1N，也不代表 CH1N 会自动开始运行。

---

# 7. Dead Time

上下 MOSFET 不能在切换瞬间同时导通。

否则：

```text
上管导通
+
下管导通
↓
直通
↓
母线短路
```

因此加入 Dead Time：

```text
一个MOSFET关闭
↓
等待一小段时间
↓
另一个MOSFET打开
```

当前：

```text
Dead Time = 85
```

TIM1 时钟为 170 MHz 时，对应大约：

$$
500ns
$$

当前没有示波器，因此：

- PWM 频率
- 实际互补边沿
- 实际 Dead Time

暂时不能做完整物理波形验证。

以后有示波器后补测。

万用表只能粗略验证 PWM 平均电压。

---

# 8. `.ioc` 参数应该怎么学

不需要把所有 CubeMX 配置项都背下来。

可以分四类。

## 必须理解

```text
Pin Mapping
Clock
PSC
ARR
CCR
Counter Mode
Complementary PWM
Dead Time
Polarity
```

## 当前开始理解

```text
Preload
Update Event
ADC Trigger
Injected ADC
```

## 后面再重点学习

```text
TRGO / TRGO2
Break
Master / Slave
```

## 当前基本不用管

```text
Fast Mode
Clear Input
Asymmetrical Dead Time
```

目标不是背 CubeMX，而是：

> 知道我要实现什么功能，因此应该关注哪些参数。

---

# 9. PWM 为什么要同步 ADC

之前 ADC 是：

```text
ADC Continuous
↓
DMA
↓
一直采
```

这种方式不知道采样发生在 PWM 的什么位置。

真正电流控制需要：

```text
PWM运行
↓
到达指定位置
↓
ADC采样
↓
Control ISR
↓
FOC
↓
更新PWM
```

即：

$$
PWM
\rightarrow
ADC\ Trigger
\rightarrow
ADC\ Sampling
\rightarrow
Control\ ISR
\rightarrow
PWM\ Update
$$

这是实时 FOC 非常重要的一条时序链。

---

# 10. TIM1 CH4 的作用

TIM1：

```text
CH1 → A相PWM
CH2 → B相PWM
CH3 → C相PWM
```

CH4 不驱动功率器件，而专门作为：

> ADC 采样时刻发生器。

当前：

```text
Channel4 = PWM Generation No Output CH4
CCR4 = 4248
```

即：

$$
CCR4=ARR-1
$$

CH4 不需要 GPIO 输出，但仍然需要：

```c
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
```

因为：

```text
CH4内部PWM
↓
产生比较边沿
↓
触发ADC
```

所以：

> No Output ≠ 不需要启动。

---

# 11. Output Compare Preload

当前 CH4：

```text
Output Compare Preload = Disable
```

因为现在：

```text
CCR4 = 4248
```

是固定采样位置。

Disable：

```text
修改CCR
↓
立即生效
```

Enable：

```text
修改CCR
↓
先进入Preload
↓
等Update Event
↓
统一生效
```

以后如果 SVPWM 下需要动态修改采样位置，再深入学习 Preload。

---

# 12. Regular ADC 和 Injected ADC

现在开始区分两类 ADC。

## Regular ADC

适合：

```text
VDC
温度
普通模拟量
```

当前：

```text
ADC1 Regular
→ ADC1_IN1
→ VDC
→ DMA
```

---

## Injected ADC

适合：

```text
相电流
高速控制反馈
需要精确时刻采样的信号
```

当前：

```text
ADC1 Injected
→ Iu

ADC2 Injected
→ Iv
```

两者都由：

```text
TIM1_CC4
```

触发。

最终：

```text
TIM1 CH4
       ↓
   同步触发
   ↓       ↓
 ADC1     ADC2
   ↓       ↓
  Iu       Iv
```

---

# 13. 外设启动顺序

同步 ADC 时启动顺序很重要。

应该：

```text
OPAMP
↓
ADC Calibration
↓
ADC Ready
↓
Injected ADC等待触发
↓
最后启动TIM1
```

而不是先启动 TIM1。

原因：

> 如果 PWM 触发已经出现，而 ADC 还没准备好，这次触发可能直接丢失。

---

# 14. Injected ADC 中断

当前使用：

```c
HAL_ADCEx_InjectedStart_IT(&hadc1);
HAL_ADCEx_InjectedStart_IT(&hadc2);
```

转换完成后进入：

```c
void HAL_ADCEx_InjectedConvCpltCallback(
    ADC_HandleTypeDef *hadc)
```

回调中主要做：

```text
读取Iu Raw
读取Iv Raw
↓
Offset
↓
电流换算
```

以后这里会继续加入：

```text
Clarke
↓
Park
↓
PI
↓
SVPWM
↓
CCR Update
```

因此这个 Callback 会逐渐成为：

> FOC 高速 Control ISR。

---

# 15. 为什么 ADC 中断里不设置 uart_send_flag

ADC Injected 中断运行频率约：

$$
20kHz
$$

也就是：

$$
50\mu s
$$

左右执行一次。

UART 属于低速操作。

所以不能：

```text
20kHz ADC ISR
↓
UART发送
```

否则会严重拖慢控制程序。

当前结构：

```text
ADC Injected ISR（20kHz）
→ 采样
→ 电流计算
→ 以后FOC

TIM6（1ms）
→ 计时
→ 每1秒 uart_send_flag = 1

main while
→ UART打印
```

重要原则：

> 高速控制中断负责采样和控制；低速后台任务负责通信和监控。

---

# 16. 同步 Offset 校准

之前已经做过 Offset。

现在改成 PWM 同步 Injected ADC 后，需要重新用最终采样路径校准。

前 1000 次同步采样：

```text
Iu Raw
Iv Raw
↓
累加
↓
平均
↓
Iu Offset
Iv Offset
```

如果采样频率约：

$$
20kHz
$$

那么：

$$
\frac{1000}{20000}=0.05s
$$

约：

```text
50 ms
```

即可完成零点校准。

---

# 17. 电流换算

基本关系仍然不变：

$$
I\propto ADC_{raw}-ADC_{offset}
$$

代码：

```c
iu_adc_delta =
    (int32_t)iu_raw_sync -
    (int32_t)iu_offset_adc;

iv_adc_delta =
    (int32_t)iv_raw_sync -
    (int32_t)iv_offset_adc;
```

当前硬件：

```text
Shunt = 5 mΩ
OPAMP Gain ≈ 7.33
```

因此约：

$$
1\ ADC\ Count\approx21.98mA
$$

换算：

```c
iu_ma = iu_adc_delta * 2198L / 100L;
iv_ma = iv_adc_delta * 2198L / 100L;
```

零电流状态：

```text
Iu ≈ 0
Iv ≈ 0
```

说明同步采样 + Offset + 电流换算链路正常。

---

# 18. 当前实时控制架构

目前已经逐渐形成：

```text
                    TIM1 20kHz
                         ↓
                   PWM输出
                         ↓
                 CH4采样触发
                         ↓
              ADC1 / ADC2 Injected
                    ↓       ↓
                   Iu       Iv
                     \     /
                      ↓   ↓
                    Offset
                      ↓
                   Current
                      ↓
             [下一步 Clarke]
```

同时后台：

```text
TIM6
↓
低速Flag
↓
main while
↓
UART / Debug
```

这就是以后嵌入式 FOC 的基本软件架构雏形。

---

# 19. 当前没有示波器时的验证方式

目前可以使用：

- Debugger Expressions
- UART
- 万用表
- CubeMonitor（以后加入）

可以验证：

```text
ARR
CCR
ADC Raw
Offset
Current
ISR Counter
Duty
```

但 CubeMonitor 和万用表不能真正验证：

```text
PWM真实边沿
PWM频率精度
互补边沿
500ns Dead Time
```

这些以后有示波器后统一补测。

---

# 20. 这一阶段最需要记住

- TIM1 是电机控制高级定时器
- CH1/2/3 输出三相 PWM
- CH1N/2N/3N 输出互补 PWM
- Dead Time 防止上下 MOSFET 直通
- Center-Aligned PWM 一个周期包含上数和下数
- CCR 决定 Duty
- CH4 可以不输出 GPIO，只用于产生 ADC 采样时刻
- VDC 适合 Regular ADC
- 电流适合 Injected ADC
- PWM 可以硬件触发 ADC
- ADC Injected ISR 将成为以后 FOC Control ISR
- 高速 ISR 里不要做 UART
- Offset 应该使用最终实际采样链重新标定

---

# 当前学习进度

- [x] GPIO / LED
- [x] UART
- [x] Timer / Interrupt
- [x] 普通 PWM
- [x] ADC Polling
- [x] ADC + DMA
- [x] OPAMP 电流采样
- [x] U/V 两相电流 Offset
- [x] ADC → 电流换算
- [x] TIM1 中心对齐 PWM
- [x] 三相 PWM
- [x] 互补 PWM
- [x] Dead Time
- [x] TIM1 CH4 ADC Trigger
- [x] Regular / Injected ADC
- [x] PWM 同步电流采样
- [x] Injected ADC Interrupt
- [x] 同步 Offset 校准
- [ ] Clarke
- [ ] Park
- [ ] InvPark
- [ ] CubeMonitor
- [ ] Current PI
- [ ] SVPWM
- [ ] Encoder
- [ ] Current Loop FOC

下一阶段：

> **Clarke Transform：正式把同步采集到的 Iu、Iv 从三相坐标系转换到 αβ 坐标系。**