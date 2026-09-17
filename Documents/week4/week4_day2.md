# STM32 学习笔记：SVPWM、Min-Max 调制与三相 PWM 更新

## 一、当前学习位置

前面已经完成：

```text
Iu / Iv
↓
Clarke
↓
Iα / Iβ
↓
Park
↓
Id / Iq
↓
Current PI
↓
Vd / Vq
↓
Inverse Park
↓
Vα / Vβ
```

这一阶段需要解决：

> 如何把控制算法得到的 $V_\alpha,V_\beta$ 转换成 TIM1 可以真正执行的三相 PWM 占空比。

因此：

```text
Vα / Vβ
↓
SVPWM
↓
Duty_U / Duty_V / Duty_W
↓
CCR1 / CCR2 / CCR3
↓
TIM1
↓
三相互补 PWM
```

---

# 二、为什么 Vα/Vβ 不能直接写入 TIM1

FOC 得到：

$$
V_\alpha,\quad V_\beta
$$

它们描述的是：

> 希望三相逆变器在 $\alpha\beta$ 平面中合成的目标电压矢量。

但 TIM1 只能直接控制：

```text
CCR1
CCR2
CCR3
```

即三个桥臂的 PWM Duty。

所以必须经过：

$$
\boxed{
V_\alpha,V_\beta
\rightarrow
D_u,D_v,D_w
}
$$

再进行：

$$
\boxed{
D_u,D_v,D_w
\rightarrow
CCR1,CCR2,CCR3
}
$$

---

# 三、三相两电平逆变器的 8 个开关状态

三相逆变器有：

```text
U桥臂
V桥臂
W桥臂
```

每一个桥臂有两种状态：

```text
1 → 上管导通、下管关闭
0 → 上管关闭、下管导通
```

因此总状态数：

$$
2^3=8
$$

分别为：

```text
000
001
010
011
100
101
110
111
```

其中：

```text
000
111
```

不会产生三相线电压，因此称为：

$$
\boxed{\text{Zero Voltage Vector}}
$$

即零矢量。

剩余 6 个状态产生非零电压矢量：

$$
\boxed{
V_1,V_2,V_3,V_4,V_5,V_6
}
$$

---

# 四、为什么会有 6 个 Sector

6 个有效电压矢量在 $\alpha\beta$ 平面中相隔：

$$
60^\circ
$$

形成六边形。

因此空间被自然分成：

$$
\boxed{6\text{ 个 Sector}}
$$

通常：

```text
0°   ~ 60°  → Sector 1
60°  ~ 120° → Sector 2
120° ~ 180° → Sector 3
180° ~ 240° → Sector 4
240° ~ 300° → Sector 5
300° ~ 360° → Sector 6
```

目标电压矢量：

$$
\vec V_{ref}
=
V_\alpha+jV_\beta
$$

通常位于其中某一个 Sector 内。

---

# 五、经典 SVPWM 的核心思想

逆变器只能直接产生：

```text
6个固定方向的有效矢量
+
2个零矢量
```

却不能直接产生任意方向的：

$$
\vec V_{ref}
$$

因此 SVPWM 采用：

> 在一个 PWM 周期内，让两个相邻有效矢量和零矢量分别作用一段时间，使一个周期内的平均电压等于目标电压。

例如目标位于 Sector 1：

```text
V1方向：0°
V2方向：60°
```

则一个周期内使用：

```text
V1 → T1
V2 → T2
Zero Vector → T0
```

满足：

$$
\boxed{
T_s=T_1+T_2+T_0
}
$$

以及：

$$
\boxed{
\vec V_{ref}T_s
=
\vec V_1T_1
+
\vec V_2T_2
}
$$

这就是经典 SVPWM 最核心的伏秒平衡关系。

---

# 六、Sector 1 中 T1 和 T2 的来源

假设目标矢量：

$$
|\vec V_{ref}|=V_{ref}
$$

与 $V_1$ 的夹角为：

$$
\theta
$$

并且：

$$
0\leq\theta\leq60^\circ
$$

两电平逆变器有效电压矢量幅值为：

$$
|\vec V_1|
=
|\vec V_2|
=
\frac{2}{3}V_{dc}
$$

---

## V1 的 αβ 分量

因为：

$$
V_1
$$

位于：

$$
0^\circ
$$

所以：

$$
V_{1\alpha}
=
\frac23V_{dc}
$$

$$
V_{1\beta}=0
$$

---

## V2 的 αβ 分量

因为：

$$
V_2
$$

位于：

$$
60^\circ
$$

所以：

$$
V_{2\alpha}
=
\frac23V_{dc}\cos60^\circ
=
\frac13V_{dc}
$$

$$
V_{2\beta}
=
\frac23V_{dc}\sin60^\circ
=
\frac{V_{dc}}{\sqrt3}
$$

---

## 根据 β 轴求 T2

目标在 β 轴上的伏秒：

$$
V_{ref}\sin\theta T_s
$$

所以：

$$
V_{ref}\sin\theta T_s
=
\frac{V_{dc}}{\sqrt3}T_2
$$

得到：

$$
\boxed{
T_2
=
\frac{\sqrt3T_sV_{ref}}
{V_{dc}}
\sin\theta
}
$$

---

## 根据 α 轴求 T1

最终可得到：

$$
\boxed{
T_1
=
\frac{\sqrt3T_sV_{ref}}
{V_{dc}}
\sin(60^\circ-\theta)
}
$$

或者：

$$
\boxed{
T_1
=
\frac{\sqrt3T_sV_{ref}}
{V_{dc}}
\sin
\left(
\frac{\pi}{3}-\theta
\right)
}
$$

然后：

$$
\boxed{
T_0=T_s-T_1-T_2
}
$$

---

# 七、T1 和 T2 的直观意义

如果目标矢量越来越靠近 $V_1$：

$$
\theta\rightarrow0
$$

则：

$$
T_2\rightarrow0
$$

说明：

> 主要使用 $V_1$。

如果目标越来越靠近 $V_2$：

$$
\theta\rightarrow60^\circ
$$

则：

$$
T_1\rightarrow0
$$

说明：

> 主要使用 $V_2$。

所以 $T_1,T_2$ 本质表示：

> 一个 PWM 周期内两个相邻有效矢量分别应该作用多长时间。

---

# 八、七段式 SVPWM

以 Sector 1 为例，一种经典中心对称开关序列：

```text
000
↓
100
↓
110
↓
111
↓
110
↓
100
↓
000
```

对应：

```text
V0
↓
V1
↓
V2
↓
V7
↓
V2
↓
V1
↓
V0
```

更加详细的时间分配可以写成：

```text
000    T0/4
100    T1/2
110    T2/2
111    T0/2
110    T2/2
100    T1/2
000    T0/4
```

总时间：

$$
T_0+T_1+T_2=T_s
$$

这种排列左右对称，所以称为：

> Seven-segment SVPWM。

---

# 九、为什么三相 Duty 是下面的公式

Sector 1 中：

$$
\boxed{
D_u
=
\frac{
T_1+T_2+\frac{T_0}{2}
}
{T_s}
}
$$

$$
\boxed{
D_v
=
\frac{
T_2+\frac{T_0}{2}
}
{T_s}
}
$$

$$
\boxed{
D_w
=
\frac{
\frac{T_0}{2}
}
{T_s}
}
$$

其来源不是公式规定，而是：

> 统计 U、V、W 三个上桥臂在整个 PWM 周期中分别导通了多少时间。

---

## U 相

状态：

```text
100 → ON
110 → ON
111 → ON
```

所以：

$$
T_U
=
T_1+T_2+\frac{T_0}{2}
$$

因此：

$$
D_u=\frac{T_U}{T_s}
$$

---

## V 相

状态：

```text
100 → OFF
110 → ON
111 → ON
```

所以：

$$
T_V
=
T_2+\frac{T_0}{2}
$$

因此：

$$
D_v=\frac{T_V}{T_s}
$$

---

## W 相

状态：

```text
100 → OFF
110 → OFF
111 → ON
```

所以：

$$
T_W=\frac{T_0}{2}
$$

因此：

$$
D_w=\frac{T_W}{T_s}
$$

---

# 十、当前工程为什么不直接使用 Sector 法

经典实现流程：

```text
Vα / Vβ
↓
判断 Sector
↓
计算 T1
↓
计算 T2
↓
计算 T0
↓
根据 Sector 分配 Duty
↓
Duty U/V/W
```

需要：

```text
Sector判断
+
多个公式
+
不同Sector的Duty映射
```

实现和调试相对复杂。

当前 STM32 工程使用：

> Min-Max Common-Mode Injection

它在线性调制区能够得到与标准对称 SVPWM 等价的 Duty。

---

# 十一、Min-Max SVPWM 的计算过程

首先把：

$$
V_\alpha,V_\beta
$$

通过逆 Clarke 得到三相参考电压：

$$
\boxed{
V_u=V_\alpha
}
$$

$$
\boxed{
V_v
=
-\frac12V_\alpha
+
\frac{\sqrt3}{2}V_\beta
}
$$

$$
\boxed{
V_w
=
-\frac12V_\alpha
-
\frac{\sqrt3}{2}V_\beta
}
$$

其中：

$$
\frac{\sqrt3}{2}
\approx0.8660254
$$

代码：

```c
vu = v_alpha;

vv =
    -0.5f * v_alpha
    + 0.8660254f * v_beta;

vw =
    -0.5f * v_alpha
    - 0.8660254f * v_beta;
```

---

# 十二、公共模电压注入

找三相中的：

$$
V_{max}
=
\max(V_u,V_v,V_w)
$$

以及：

$$
V_{min}
=
\min(V_u,V_v,V_w)
$$

计算：

$$
\boxed{
V_{offset}
=
-\frac{V_{max}+V_{min}}2
}
$$

然后：

$$
V_u'=V_u+V_{offset}
$$

$$
V_v'=V_v+V_{offset}
$$

$$
V_w'=V_w+V_{offset}
$$

最后转换为 Duty：

$$
\boxed{
D_u
=
0.5+\frac{V_u'}{V_{dc}}
}
$$

$$
\boxed{
D_v
=
0.5+\frac{V_v'}{V_{dc}}
}
$$

$$
\boxed{
D_w
=
0.5+\frac{V_w'}{V_{dc}}
}
$$

---

# 十三、为什么可以给三相同时加入 Voffset

因为电机真正主要受到线电压控制。

例如：

$$
V_{uv}=V_u-V_v
$$

三相同时加一个公共量 $C$：

$$
V_u'=V_u+C
$$

$$
V_v'=V_v+C
$$

则：

$$
V_{uv}'
=
(V_u+C)-(V_v+C)
$$

所以：

$$
\boxed{
V_{uv}'=V_u-V_v
}
$$

线电压没有变化。

因此可以利用公共模电压：

> 对三个调制波整体进行上下移动，而不改变最终需要的线电压。

---

# 十四、Min-Max 为什么和经典 SVPWM 等价

经典 SVPWM：

```text
Vα/Vβ
↓
Sector
↓
T1 / T2
↓
T0
↓
零矢量分配
↓
Duty
```

Min-Max：

```text
Vα/Vβ
↓
Inverse Clarke
↓
Vu / Vv / Vw
↓
Vmax / Vmin
↓
Common-Mode Injection
↓
Duty
```

两者在线性调制区、采用对应的中心对称调制时，可以得到相同三相 Duty。

核心区别：

> 经典方法显式计算零矢量时间 $T_0$。

而：

> Min-Max 通过公共模电压隐式完成零矢量分配。

---

# 十五、两种方法的数值验证

例如：

$$
V_\alpha=4V
$$

$$
V_\beta=2V
$$

$$
V_{dc}=24V
$$

目标幅值：

$$
V_{ref}
=
\sqrt{4^2+2^2}
\approx4.472V
$$

角度：

$$
\theta
=
\tan^{-1}\left(\frac24\right)
\approx26.565^\circ
$$

所以位于：

```text
Sector 1
```

经典方法计算得到约：

$$
T_1=8.89\mu s
$$

$$
T_2=7.22\mu s
$$

$$
T_0=33.89\mu s
$$

最终：

$$
D_u\approx0.6611
$$

$$
D_v\approx0.4833
$$

$$
D_w\approx0.3389
$$

Min-Max 方法计算后同样得到：

$$
\boxed{
D_u\approx0.6611
}
$$

$$
\boxed{
D_v\approx0.4833
}
$$

$$
\boxed{
D_w\approx0.3389
}
$$

说明两种方法最终结果一致。

---

# 十六、Min-Max 和经典 Sector-SVPWM 对比

| 对比项           | Sector-SVPWM       | Min-Max SVPWM      |
| ---------------- | ------------------ | ------------------ |
| 输入             | $V_\alpha,V_\beta$ | $V_\alpha,V_\beta$ |
| Sector 判断      | 需要               | 不需要             |
| $T_1/T_2/T_0$    | 显式计算           | 不显式计算         |
| Sector Duty 映射 | 需要               | 不需要             |
| 实现复杂度       | 较高               | 较低               |
| Debug 难度       | 较高               | 较低               |
| MCU 计算量       | 较多               | 较少               |
| 线性区 Duty      | 等价               | 等价               |
| 母线利用率       | 相同               | 相同               |
| 开关状态信息     | 很清楚             | 不直接体现         |
| Sector 信息      | 天然获得           | 需要额外计算       |
| 高级 PWM 扩展    | 更直接             | 需要额外处理       |

因此当前 STM32 工程：

> 实际程序继续使用 Min-Max SVPWM。

但理论上仍然需要理解：

```text
Sector
T1
T2
T0
开关状态
```

因为这些信息以后可能用于：

```text
ADC采样窗口优化
动态采样时刻
DPWM
过调制
单/双电阻电流重构
开关损耗优化
```

---

# 十七、SVPWM 电压矢量限制

逆变器能够输出的电压不是无限的。

标准 SVPWM 在线性调制区允许的最大旋转电压矢量：

$$
\boxed{
V_{max}
=
\frac{V_{dc}}{\sqrt3}
}
$$

即：

$$
V_{max}
\approx0.57735V_{dc}
$$

因此需要保证：

$$
\boxed{
\sqrt{
V_\alpha^2+V_\beta^2
}
\leq
\frac{V_{dc}}{\sqrt3}
}
$$

---

# 十八、为什么最大值是 Vdc / √3

6 个有效矢量形成六边形。

有效矢量长度：

$$
\frac23V_{dc}
$$

为了让电压矢量能够在任意方向连续旋转而不离开线性区，需要限制在六边形内切圆内。

内切圆半径：

$$
\frac23V_{dc}\cos30^\circ
$$

由于：

$$
\cos30^\circ
=
\frac{\sqrt3}{2}
$$

得到：

$$
\boxed{
V_{max}
=
\frac{V_{dc}}{\sqrt3}
}
$$

---

# 十九、电压矢量超限时为什么不能分别 Clamp

错误方法：

```c
if (v_alpha > vmax)
    v_alpha = vmax;

if (v_beta > vmax)
    v_beta = vmax;
```

这样会改变：

> 电压矢量方向。

正确方法是保持方向不变，只缩短长度。

如果：

$$
V=
\sqrt{V_\alpha^2+V_\beta^2}
$$

且：

$$
V>V_{max}
$$

计算：

$$
k=
\frac{V_{max}}{V}
$$

然后：

$$
V_\alpha'=kV_\alpha
$$

$$
V_\beta'=kV_\beta
$$

因此：

```text
方向不变
幅值缩小
```

---

# 二十、当前电压限幅函数

```c
void SVPWM_LimitVoltage(float *v_alpha,
                        float *v_beta,
                        float vdc)
{
    float vmax;
    float magnitude_sq;
    float vmax_sq;
    float magnitude;
    float scale;

    if (vdc <= 0.0f)
    {
        *v_alpha = 0.0f;
        *v_beta  = 0.0f;
        return;
    }

    vmax =
        vdc * 0.577350269f;

    magnitude_sq =
        (*v_alpha) * (*v_alpha)
        +
        (*v_beta) * (*v_beta);

    vmax_sq =
        vmax * vmax;

    if (magnitude_sq > vmax_sq)
    {
        magnitude =
            sqrtf(magnitude_sq);

        scale =
            vmax / magnitude;

        *v_alpha *= scale;
        *v_beta  *= scale;
    }
}
```

只有超限时才执行：

```c
sqrtf()
```

可以减少不必要计算。

---

# 二十一、SVPWM 软件模块

建立：

```text
Core/Inc/svpwm.h
Core/Src/svpwm.c
```

输出结构：

```c
typedef struct
{
    float duty_u;
    float duty_v;
    float duty_w;

} SVPWM_Output_t;
```

主函数：

```c
void SVPWM_Run(float v_alpha,
               float v_beta,
               float vdc,
               SVPWM_Output_t *out);
```

当前采用：

> Min-Max Common-Mode Injection。

---

# 二十二、Duty 转换为 TIM1 CCR

当前 TIM1：

```text
ARR = 4249
```

因此 PWM 周期计数：

```text
ARR + 1 = 4250
```

Duty 与 CCR 近似关系：

$$
CCR
\approx
Duty\times(ARR+1)
$$

例如：

$$
Duty=0.5
$$

则：

$$
CCR
\approx2125
$$

代码：

```c
static uint32_t DutyToCCR(float duty)
{
    uint32_t arr;
    uint32_t period;
    uint32_t ccr;

    arr =
        __HAL_TIM_GET_AUTORELOAD(&htim1);

    period =
        arr + 1U;

    if (duty < 0.0f)
    {
        duty = 0.0f;
    }

    if (duty > 1.0f)
    {
        duty = 1.0f;
    }

    ccr =
        (uint32_t)(duty * (float)period);

    if (ccr > arr)
    {
        ccr = arr;
    }

    return ccr;
}
```

---

# 二十三、CCR 实时更新

SVPWM 得到：

```text
duty_u
duty_v
duty_w
```

转换：

```c
pwm_ccr_u =
    DutyToCCR(svpwm_out.duty_u);

pwm_ccr_v =
    DutyToCCR(svpwm_out.duty_v);

pwm_ccr_w =
    DutyToCCR(svpwm_out.duty_w);
```

最终：

```c
__HAL_TIM_SET_COMPARE(&htim1,
                      TIM_CHANNEL_1,
                      pwm_ccr_u);

__HAL_TIM_SET_COMPARE(&htim1,
                      TIM_CHANNEL_2,
                      pwm_ccr_v);

__HAL_TIM_SET_COMPARE(&htim1,
                      TIM_CHANNEL_3,
                      pwm_ccr_w);
```

---

# 二十四、为什么不用单独更新 CH1N / CH2N / CH3N

当前 TIM1 已经配置：

```text
CH1  / CH1N
CH2  / CH2N
CH3  / CH3N
```

所以软件只需要修改：

```text
CCR1
CCR2
CCR3
```

TIM1 硬件自动生成：

```text
互补输出
+
Dead Time
```

因此：

```text
CCR1
↓
CH1 Duty
↓
TIM1硬件产生CH1N
```

不需要单独计算 N 通道 Duty。

---

# 二十五、为什么 CH1/CH2/CH3 要开启 Preload

现在 SVPWM 每一个控制周期都会修改：

```text
CCR1
CCR2
CCR3
```

如果修改后立即生效，就可能发生：

```text
先写CCR1
↓
CCR1立即改变

再写CCR2
↓
CCR2再改变

最后写CCR3
```

导致三个通道不是严格同时更新。

因此：

```text
CH1 Output Compare Preload → Enable
CH2 Output Compare Preload → Enable
CH3 Output Compare Preload → Enable
```

新值先进入 Preload：

```text
CCR新值
↓
Preload
↓
等待Update Event
↓
统一生效
```

从而提高三相 PWM 更新一致性。

当前 CH4：

```text
Output Compare Preload → Disable
```

可以继续保留。

因为：

```text
CH4
→ 当前仅用于固定ADC采样时刻
→ CCR4暂时不动态变化
```

---

# 二十六、当前完整高速控制链

目前高速实时链已经形成：

```text
TIM1 PWM
↓
CH4触发ADC
↓
ADC1 / ADC2 Injected
↓
Iu / Iv
↓
Offset Compensation
↓
Current
↓
Clarke
↓
Iα / Iβ
↓
Park
↓
Id / Iq
↓
Current PI
↓
Vd / Vq
↓
InvPark
↓
Vα / Vβ
↓
Voltage Vector Limit
↓
Min-Max SVPWM
↓
Duty U/V/W
↓
CCR1/CCR2/CCR3
↓
TIM1
```

这已经是 FOC 高速控制 ISR 的主体结构。

---

# 二十七、SVPWM 调试中发现的 PI 积分问题

曾出现：

```text
Id_ref = 0
Iq_ref = 0
```

但是：

```text
pi_id.integral ≈ 4V
pi_iq.integral ≈ 3V

Vd ≈ 5V
Vq ≈ 3V
```

这并不是 SVPWM 错误。

原因是：

> PI_Reset() 只是在某一个时刻把积分器清零。

随后每个 20 kHz 控制周期都会再次执行：

```c
PI_Run(...)
```

只要：

$$
Id\neq0
$$

或者：

$$
Iq\neq0
$$

积分器就会重新积累。

---

# 二十八、为什么当前测试尤其容易积分起来

当前仍使用：

```text
theta_e_test
```

作为人工电角度。

它以约：

```text
1 Hz
```

旋转。

如果 ADC 中存在一个很小的静态偏置：

```text
Iα ≈ 常数
Iβ ≈ 常数
```

经过不断旋转的 Park Transform 后，会变成：

```text
Id / Iq低频周期变化
```

于是 PI 会认为：

```text
Current Error ≠ 0
```

并持续积分。

而当前又没有真正工作的电机闭环：

```text
PI输出改变
↓
电流实际上不会按照命令响应
↓
误差不会真正被消除
↓
积分器继续积累
```

因此 PI 最终到达限幅属于正常现象。

---

# 二十九、PI Reset 的正确理解

错误理解：

```text
PI_Reset()
↓
以后Integral一直等于0
```

正确理解：

```text
PI_Reset()
↓
此刻Integral = 0

下一次PI_Run()
↓
重新开始积分
```

因此真正控制系统需要：

> Controller Enable / Disable 状态管理。

---

# 三十、加入 foc_enable

增加：

```c
volatile uint8_t foc_enable = 0;
```

控制器未启用时：

```c
if (foc_enable)
{
    vd_cmd =
        PI_Run(&pi_id,
               id_ref,
               i_dq.d,
               CURRENT_LOOP_TS);

    vq_cmd =
        PI_Run(&pi_iq,
               iq_ref,
               i_dq.q,
               CURRENT_LOOP_TS);
}
else
{
    PI_Reset(&pi_id);
    PI_Reset(&pi_iq);

    vd_cmd = 0.0f;
    vq_cmd = 0.0f;
}
```

因此：

```text
FOC Disable
↓
PI积分清零
↓
Vd = 0
Vq = 0
↓
Valpha = 0
Vbeta = 0
↓
SVPWM
↓
Duty ≈ 50% / 50% / 50%
```

注意：

> 50% Duty 本身不等于完整的功率级安全关断。

真正实机还需要正式的 PWM Enable / Disable、Gate Driver Enable、Break/Fault 等机制。

---

# 三十一、初始化顺序问题

之前代码存在：

```text
先启动TIM1 / ADC Trigger
↓
控制ISR已经开始运行
↓
之后才PI_Init
↓
之后才设置theta_e_step
```

这种顺序不规范。

正确原则：

```text
软件控制变量初始化
↓
PI Init / Reset
↓
角度变量初始化
↓
OPAMP
↓
ADC Calibration
↓
ADC准备好
↓
Injected ADC等待Trigger
↓
最后启动TIM1
```

即：

> 所有软件状态必须准备完成以后，最后才启动会产生高速实时中断的定时器。

---

# 三十二、删除旧 PWM 测试代码

之前 PWM 学习阶段留下：

```c
CCR1 = 20%
CCR2 = 50%
CCR3 = 80%
```

进入 SVPWM 阶段以后必须删除。

否则：

```text
TIM1启动
↓
在SVPWM真正接管前
↓
先输出20/50/80%
```

实际带功率时并不合适。

初始化状态应该由正式控制状态统一管理。

---

# 三十三、未来更合理的软件状态

电机控制程序不能简单：

```text
上电
↓
全部控制器立即运行
```

更合理的是：

```text
INIT
↓
CURRENT OFFSET CALIBRATION
↓
READY
↓
FOC ENABLE
↓
RUN
↓
FAULT / DISABLE
```

其中：

```text
INIT
→ 软件和硬件初始化

OFFSET CALIBRATION
→ 电流零点标定

READY
→ 系统已准备但不输出控制

FOC ENABLE
→ 清PI并允许控制

RUN
→ 正常FOC运行

FAULT
→ PWM关闭、控制器Reset
```

这是后续真正实机运行前必须逐渐建立的结构。

---

# 三十四、Min-Max 与 Sector-SVPWM 应该怎么选

当前工程实际实现建议：

$$
\boxed{\text{Min-Max SVPWM}}
$$

原因：

- 代码简单
- 分支少
- 不需要 Sector 映射
- Debug 更方便
- MCU 运算量较小
- 在线性区与标准对称 SVPWM 等价

理论学习仍需要掌握：

$$
\boxed{
Sector,\ T_1,\ T_2,\ T_0
}
$$

因为后面这些信息可能用于：

```text
ADC采样位置优化
动态CCR4
电流重构
DPWM
过调制
PWM开关序列优化
```

所以二者关系可以记成：

```text
经典 Sector-SVPWM
→ 用于理解空间矢量和高级控制

Min-Max SVPWM
→ 当前工程用于高效计算 Duty
```

不是：

```text
Min-Max 和 SVPWM 二选一
```

而是：

> Min-Max 本身就是一种 SVPWM 实现方式。

---

# 三十五、本阶段最重要的认识

- 三相两电平逆变器只有 8 种基本开关状态。
- 其中有 6 个有效矢量和 2 个零矢量。
- 6 个有效矢量形成六边形，并把平面分成 6 个 Sector。
- 经典 SVPWM 利用两个相邻有效矢量和零矢量合成目标平均电压。
- $T_1,T_2$ 来自矢量伏秒平衡。
- $T_0=T_s-T_1-T_2$。
- 七段式 SVPWM 使用中心对称的开关序列。
- Duty 公式来源于统计每个上桥臂在一个周期中的 ON 时间。
- Min-Max 使用公共模电压注入隐式完成零矢量分配。
- Min-Max 在线性区可以得到与经典对称 SVPWM 等价的 Duty。
- SVPWM 最大线性电压矢量约为 $V_{dc}/\sqrt3$。
- 电压超限应该按比例缩放整个矢量，不能分别 Clamp α、β。
- Duty 最终转换为 CCR1/CCR2/CCR3。
- 互补 PWM 和 Dead Time 由 TIM1 硬件完成。
- CH1/2/3 动态修改 CCR 时需要 Preload。
- PI Reset 只是清除当前积分状态，不会阻止之后重新积分。
- 当前人工角度 + 非闭环电机环境下，不应该让 Current PI 无条件长期运行。
- 控制程序需要逐步加入 FOC Enable / Disable 和状态管理。

---

# 三十六、当前学习进度

已完成：

- [x] SVPWM 基本概念
- [x] 两电平逆变器 8 个开关状态
- [x] 6 个有效电压矢量
- [x] 6 个 Sector
- [x] $T_1/T_2/T_0$ 基本原理
- [x] Sector 1 的 $T_1/T_2$ 推导
- [x] 七段式 SVPWM
- [x] Duty 公式来源
- [x] Min-Max SVPWM
- [x] Common-Mode Injection
- [x] Min-Max 与 Sector-SVPWM 数值对比
- [x] SVPWM 电压矢量限幅
- [x] Duty → CCR
- [x] TIM1 CCR1/2/3 实时更新
- [x] PWM Preload 概念应用
- [x] PI 积分异常定位
- [x] `foc_enable` 基本思想
- [x] 实时控制初始化顺序检查

当前实际代码建议继续采用：

```text
Min-Max SVPWM
```

而经典：

```text
Sector + T1 + T2 + T0
```

作为理论基础保留。

下一阶段将继续完善：

```text
真实 Vdc
控制状态管理
真实转子角度 / 电角度
最终真实 FOC 闭环
```
