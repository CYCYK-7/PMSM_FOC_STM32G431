# STM32 学习笔记：PMSM 电流 PI 控制器、dq 解耦与带宽设计

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
```

以及：

```text
Vd / Vq
↓
Inverse Park
↓
Vα / Vβ
```

这一阶段开始进入真正的电流闭环控制：

```text
Id_ref - Id
↓
PI_d
↓
Vd

Iq_ref - Iq
↓
PI_q
↓
Vq
```

后续完整链路：

```text
Iu / Iv
↓
Clarke
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
SVPWM
↓
Duty
↓
TIM1 PWM
```

---

# 二、d/q 电流对象是什么

控制理论中：

> Plant = 被控对象。

对 d 轴来说：

```text
输入：Vd
输出：Id
```

所以：

```text
Vd → Id
```

这一动态关系就是：

> d 轴电流对象。

同理：

```text
Vq → Iq
```

就是：

> q 轴电流对象。

也就是说所谓：

```text
dq电流对象
```

本质是在问：

> PMSM 从 Vd/Vq 到 Id/Iq 的动态关系是什么。

---

# 三、PMSM dq 电压方程

PMSM 在 dq 坐标系中的电压方程：

$$
v_d
=
R_s i_d
+
L_d\frac{di_d}{dt}
-
\omega_eL_qi_q
$$

$$
v_q
=
R_s i_q
+
L_q\frac{di_q}{dt}
+
\omega_eL_di_d
+
\omega_e\psi_f
$$

其中：

- $R_s$：定子电阻
- $L_d$：d 轴电感
- $L_q$：q 轴电感
- $\omega_e$：电角速度
- $\psi_f$：永磁磁链
- $i_d,i_q$：dq 电流
- $v_d,v_q$：dq 电压

---

# 四、为什么 d/q 两个轴不是完全独立的

观察 d 轴：

$$
v_d
=
R_si_d
+
L_d\frac{di_d}{dt}
-
\omega_eL_qi_q
$$

如果它只是一个纯 RL 回路，应当只有：

$$
R_si_d
+
L_d\frac{di_d}{dt}
$$

但实际多出：

$$
-\omega_eL_qi_q
$$

说明：

> q 轴电流会影响 d 轴。

同理 q 轴：

$$
v_q
=
R_si_q
+
L_q\frac{di_q}{dt}
+
\omega_eL_di_d
+
\omega_e\psi_f
$$

其中：

$$
\omega_eL_di_d
$$

说明：

> d 轴电流会影响 q 轴。

因此：

```text
Id ↔ Iq
```

存在交叉影响。

这就是：

> dq 轴交叉耦合。

---

# 五、为什么速度越高耦合越明显

耦合项和反电动势都包含：

$$
\omega_e
$$

例如：

$$
\omega_eL_qi_q
$$

$$
\omega_eL_di_d
$$

$$
\omega_e\psi_f
$$

因此：

```text
速度低
↓
ωe小
↓
耦合项和反电动势较小
```

而：

```text
速度高
↓
ωe大
↓
dq耦合明显
↓
反电动势明显
```

所以高速下如果完全忽略这些项，电流环性能会明显变差。

---

# 六、为什么 dq 电流通道可以近似看成 RL 一阶对象

先看 d 轴：

$$
v_d
=
R_si_d
+
L_d\frac{di_d}{dt}
-
\omega_eL_qi_q
$$

定义补偿后的等效输入：

$$
v'_d
=
v_d+\omega_eL_qi_q
$$

则：

$$
v'_d
=
R_si_d
+
L_d\frac{di_d}{dt}
$$

q 轴类似：

$$
v_q
=
R_si_q
+
L_q\frac{di_q}{dt}
+
\omega_eL_di_d
+
\omega_e\psi_f
$$

定义：

$$
v'_q
=
v_q
-
\omega_eL_di_d
-
\omega_e\psi_f
$$

则：

$$
v'_q
=
R_si_q
+
L_q\frac{di_q}{dt}
$$

这样每个轴都变成：

$$
\boxed{
v=Ri+L\frac{di}{dt}
}
$$

也就是一个普通：

> RL 电路。

---

# 七、RL 电流对象的传递函数

从：

$$
v
=
Ri+L\frac{di}{dt}
$$

做拉普拉斯变换：

$$
V(s)
=
RI(s)+LsI(s)
$$

整理：

$$
V(s)
=
(R+Ls)I(s)
$$

所以：

$$
\boxed{
G(s)
=
\frac{I(s)}{V(s)}
=
\frac{1}{Ls+R}
}
$$

分母最高只有：

$$
s^1
$$

因此：

> RL 电流对象是一个一阶对象。

进一步写成：

$$
G(s)
=
\frac{1}{L\left(s+\frac RL\right)}
$$

所以对象极点：

$$
\boxed{
s=-\frac RL
}
$$

---

# 八、PI 控制器

PI：

$$
G_{PI}(s)
=
K_p+\frac{K_i}{s}
$$

整理：

$$
G_{PI}(s)
=
\frac{K_ps+K_i}{s}
$$

提取 $K_p$：

$$
G_{PI}(s)
=
K_p
\frac{s+\frac{K_i}{K_p}}
{s}
$$

因此 PI 有一个零点：

$$
\boxed{
s=-\frac{K_i}{K_p}
}
$$

---

# 九、为什么让 PI 零点和 RL 极点相等

RL 对象极点：

$$
-\frac RL
$$

PI 零点：

$$
-\frac{K_i}{K_p}
$$

令：

$$
\boxed{
\frac{K_i}{K_p}
=
\frac RL
}
$$

可以让：

```text
PI零点
```

与：

```text
RL对象极点
```

进行近似抵消。

开环：

$$
G_{open}(s)
=
G_{PI}(s)G(s)
$$

代入：

$$
G_{open}(s)
=
K_p
\frac{s+\frac{K_i}{K_p}}
{s}
\cdot
\frac{1}
{L\left(s+\frac RL\right)}
$$

当：

$$
\frac{K_i}{K_p}
=
\frac RL
$$

时：

$$
G_{open}(s)
=
\frac{K_p}{Ls}
$$

系统形式明显简化。

---

# 十、为什么极点抵消以后还要引入电流环带宽

条件：

$$
\frac{K_i}{K_p}
=
\frac RL
$$

只确定：

> $K_i$ 和 $K_p$ 的比例。

并没有确定它们的绝对大小。

例如：

```text
Kp = 1
Ki = 150
```

和：

```text
Kp = 10
Ki = 1500
```

可能都满足：

$$
\frac{K_i}{K_p}=150
$$

所以仅靠零极点抵消，还不能回答：

> 电流环到底应该多快。

因此还需要第二个设计条件：

> Current-loop Bandwidth，电流环带宽。

---

# 十一、电流环带宽的作用

极点抵消后：

$$
G_{open}(s)
=
\frac{K_p}{Ls}
$$

单位负反馈闭环：

$$
G_{closed}(s)
=
\frac{\frac{K_p}{Ls}}
{1+\frac{K_p}{Ls}}
$$

整理：

$$
G_{closed}(s)
=
\frac{K_p}
{Ls+K_p}
$$

再除以 $L$：

$$
G_{closed}(s)
=
\frac{\frac{K_p}{L}}
{s+\frac{K_p}{L}}
$$

标准一阶闭环形式：

$$
\boxed{
G_{closed}(s)
=
\frac{\omega_c}
{s+\omega_c}
}
$$

因此令：

$$
\boxed{
\frac{K_p}{L}
=
\omega_c
}
$$

得到：

$$
\boxed{
K_p=L\omega_c
}
$$

又因为：

$$
\frac{K_i}{K_p}
=
\frac RL
$$

所以：

$$
\boxed{
K_i=R\omega_c
}
$$

最终：

$$
\boxed{
K_p=L\omega_c
}
$$

$$
\boxed{
K_i=R\omega_c
}
$$

---

# 十二、电流环带宽的物理意义

电流环带宽：

$$
f_c
$$

代表：

> 电流闭环能够多快响应参考值和扰动变化。

角频率：

$$
\boxed{
\omega_c=2\pi f_c
}
$$

理想一阶闭环时间常数：

$$
\boxed{
\tau=\frac1{\omega_c}
}
$$

通常一阶系统大约经过：

$$
4\tau
$$

可以基本进入稳态。

因此：

```text
fc越高
↓
电流响应越快
```

但带宽不能无限提高。

---

# 十三、为什么电流环带宽不能接近 PWM 频率

当前：

$$
f_s=20kHz
$$

所以：

$$
T_s=50\mu s
$$

MCU 每：

$$
50\mu s
$$

才完成一次：

```text
ADC采样
↓
FOC计算
↓
PI
↓
PWM更新
```

真实系统还存在：

```text
ADC延迟
计算延迟
PWM更新延迟
```

因此控制系统存在总延迟。

---

# 十四、延迟为什么限制带宽

若总延迟近似：

$$
T_d
$$

在频率 $f$ 下产生的相位滞后约：

$$
\boxed{
\phi_d
=
-360^\circ fT_d
}
$$

假设：

$$
T_d=50\mu s
$$

即约一个控制周期。

---

## 带宽 500 Hz

$$
\phi_d
=
-360^\circ
\times500
\times50\times10^{-6}
$$

得到：

$$
\boxed{-9^\circ}
$$

---

## 带宽 1 kHz

$$
\boxed{-18^\circ}
$$

---

## 带宽 2 kHz

$$
\boxed{-36^\circ}
$$

带宽越高：

```text
延迟引起的相位滞后越大
↓
相位裕度变小
↓
系统更容易振荡
```

---

# 十五、PWM 频率与电流环带宽的关系

当前：

$$
f_s=20kHz
$$

如果：

$$
f_c=500Hz
$$

则：

$$
\frac{f_s}{f_c}=40
$$

即：

```text
采样频率 : 电流环带宽
≈ 40 : 1
```

如果：

$$
f_c=2kHz
$$

则：

$$
\frac{f_s}{f_c}=10
$$

控制环明显更接近数字系统的速度极限。

所以工程中常让：

> 电流环带宽明显低于 PWM / 采样频率。

不是绝对规则，而是为了：

```text
保留相位裕度
减小延迟影响
提高鲁棒性
```

---

# 十六、带宽越高的优缺点

## 优点

```text
Iq_ref变化
↓
Iq更快跟踪
```

以及：

```text
负载扰动
↓
电流环更快抑制
```

---

## 缺点

带宽高会导致：

```text
噪声敏感
参数误差敏感
延迟影响明显
PWM非线性影响明显
Dead Time影响明显
量化影响明显
```

所以：

> 电流环带宽并不是越高越好。

---

# 十七、500 Hz、1 kHz、2 kHz 对应的理想响应速度

## 500 Hz

$$
\omega_c
=
2\pi\times500
\approx3142rad/s
$$

$$
\tau
=
\frac1{3142}
\approx0.318ms
$$

约：

$$
4\tau
\approx1.27ms
$$

---

## 1 kHz

$$
\tau
\approx0.159ms
$$

$$
4\tau
\approx0.64ms
$$

---

## 2 kHz

$$
\tau
\approx0.080ms
$$

$$
4\tau
\approx0.32ms
$$

带宽提高时，理想电流响应越来越快。

---

# 十八、当前示例参数下 PI 参数计算

当前仅使用一组学习示例参数：

$$
R_s=1.2\Omega
$$

$$
L=8mH=0.008H
$$

注意：

> 这只是当前学习用参数，不代表实物电机真实参数。

---

## 500 Hz 带宽

$$
\omega_c
=
2\pi\times500
\approx3141.59
$$

$$
K_p
=
L\omega_c
$$

$$
=0.008\times3141.59
$$

得到：

$$
\boxed{
K_p\approx25.13
}
$$

$$
K_i
=
R\omega_c
$$

$$
=1.2\times3141.59
$$

得到：

$$
\boxed{
K_i\approx3769.9
}
$$

---

## 不同带宽对应 PI

| 电流环带宽 |  $K_p$ |   $K_i$ |
| ---------: | -----: | ------: |
|     500 Hz |  25.13 |  3769.9 |
|      1 kHz |  50.27 |  7539.8 |
|      2 kHz | 100.53 | 15079.6 |

规律：

> 带宽翻倍，Kp 和 Ki 都近似翻倍。

---

# 十九、为什么 Ki 看起来很大

例如：

$$
K_i=3769.9
$$

不能只看这个数字。

实际数字 PI 为：

$$
I_k
=
I_{k-1}
+
K_iT_se_k
$$

其中：

$$
T_s=50\mu s
$$

所以：

$$
K_iT_s
=
3769.9
\times0.00005
$$

得到：

$$
\approx0.1885
$$

因此真正每次控制中断起作用的是：

$$
K_iT_s
$$

而不是孤立的 $K_i$。

---

# 二十、Current PI 软件结构

定义两个 PI：

```text
PI_d
PI_q
```

d 轴：

$$
e_d
=
i_d^*-i_d
$$

$$
v_{d,PI}
=
PI_d(e_d)
$$

q 轴：

$$
e_q
=
i_q^*-i_q
$$

$$
v_{q,PI}
=
PI_q(e_q)
$$

---

# 二十一、SPMSM 中 Id_ref 和 Iq_ref

对于普通 SPMSM 基速以下 FOC：

常见：

$$
\boxed{
i_d^*=0
}
$$

而：

$$
i_q^*
$$

主要决定电磁转矩。

因此可以粗略理解：

```text
Id_ref
→ 磁链方向控制

Iq_ref
→ 转矩方向控制
```

普通 SPMSM 常先采用：

```text
Id_ref = 0
```

---

# 二十二、数字 PI 实现

基本形式：

$$
e
=
ref-feedback
$$

比例：

$$
P
=
K_pe
$$

积分：

$$
I_k
=
I_{k-1}
+
K_iT_se
$$

输出：

$$
u=P+I
$$

代码核心：

```c
error = ref - feedback;

proportional =
    pi->kp * error;

integral_new =
    pi->integral
    + pi->ki * ts * error;

output_unsat =
    proportional
    + integral_new;
```

---

# 二十三、为什么 PI 要限幅

逆变器输出电压受母线电压限制。

所以：

```text
PI输出
```

不可能无限大。

因此需要：

```text
output_min
output_max
```

例如学习阶段暂时：

```text
-5V ~ +5V
```

这只是软件测试限幅，不是最终逆变器真实电压极限。

---

# 二十四、Integrator Windup

如果：

```text
Iq_ref很大
Iq一直跟不上
```

则误差长期存在。

如果积分器仍不断累加：

```text
integral
↓
越来越大
```

即使 PI 输出已经被限幅，内部积分量仍可能：

```text
5
10
20
50
100
...
```

这叫：

> Integrator Windup，积分饱和。

会导致：

```text
响应恢复慢
超调增加
控制器很久退不出饱和
```

因此需要：

> Anti-Windup，抗积分饱和。

当前使用：

```text
如果输出已经达到上限
且误差还想继续向上推
→ 暂停积分

如果误差有利于退出饱和
→ 允许积分
```

这种简单 Anti-Windup。

---

# 二十五、dq 前馈解耦

前面 PI 参数设计时，把电流对象近似成：

$$
\frac1{Ls+R}
$$

这个近似建立在：

> dq 耦合项和反电动势被忽略或补偿

的前提下。

---

## d 轴解耦

原式：

$$
v_d
=
R_si_d
+
L_d\frac{di_d}{dt}
-
\omega_eL_qi_q
$$

希望 PI 只控制：

$$
R_si_d
+
L_d\frac{di_d}{dt}
$$

因此真正电压指令：

$$
\boxed{
v_d^*
=
v_{d,PI}
-
\omega_eL_qi_q
}
$$

---

## q 轴解耦

原式：

$$
v_q
=
R_si_q
+
L_q\frac{di_q}{dt}
+
\omega_eL_di_d
+
\omega_e\psi_f
$$

真正电压指令：

$$
\boxed{
v_q^*
=
v_{q,PI}
+
\omega_eL_di_d
+
\omega_e\psi_f
}
$$

---

# 二十六、为什么叫前馈解耦

PI 属于反馈：

```text
误差产生
↓
PI检测到误差
↓
再修正
```

而 dq 耦合项：

$$
\omega_eL_qi_q
$$

$$
\omega_eL_di_d
$$

以及反电动势：

$$
\omega_e\psi_f
$$

可以根据当前：

```text
ωe
Id
Iq
L
ψf
```

直接计算。

所以可以提前补偿：

```text
知道干扰会出现
↓
提前加对应电压
↓
减小它对电流环的影响
```

这就是：

> Feedforward Decoupling，前馈解耦。

---

# 二十七、SPMSM 下的解耦形式

SPMSM 通常：

$$
L_d\approx L_q=L_s
$$

因此：

$$
\boxed{
v_d^*
=
v_{d,PI}
-
\omega_eL_si_q
}
$$

$$
\boxed{
v_q^*
=
v_{q,PI}
+
\omega_eL_si_d
+
\omega_e\psi_f
}
$$

---

# 二十八、速度越高为什么解耦越重要

例如：

$$
L_s=0.008H
$$

$$
i_q=2A
$$

$$
\omega_e=500rad/s
$$

d 轴耦合量：

$$
\omega_eL_si_q
$$

$$
=
500
\times0.008
\times2
$$

得到：

$$
8V
$$

说明 q 轴会对 d 轴产生明显影响。

---

反电动势示例：

$$
\psi_f=0.1Wb
$$

$$
\omega_e=500rad/s
$$

则：

$$
\omega_e\psi_f
=
50V
$$

所以：

```text
速度越高
↓
反电动势越高
↓
需要的电压越大
```

最终受到：

```text
DC Bus Voltage
```

限制。

再继续提高速度时会涉及弱磁控制。

当前暂不展开。

---

# 二十九、当前为什么还不正式加入前馈解耦

当前系统使用：

```text
theta_e_test
```

而不是真实电机转子角度。

所以对应的：

```text
omega_e
```

也不是真实电机电角速度。

如果现在直接计算：

$$
\omega_e\psi_f
$$

就相当于加入一个：

> 假的反电动势补偿。

因此当前阶段：

- 理解 dq 解耦公式
- 理解物理意义
- 预留软件结构

但暂时不把真实解耦补偿加入最终控制输出。

等后面：

```text
Encoder
↓
真实 θe
↓
真实 ωe
```

以后再正式接入。

---

# 三十、当前 Current PI 设计完整逻辑

完整逻辑不是：

```text
随便找 Kp Ki
↓
测试
```

而是：

```text
PMSM dq模型
↓
发现dq交叉耦合和反电动势
↓
前馈解耦
↓
每个轴近似为RL对象
↓
G(s)=1/(Ls+R)
↓
PI零点匹配RL极点
↓
Ki/Kp = R/L
↓
这里只确定了比例
↓
再指定电流环带宽
↓
Kp = Lωc
Ki = Rωc
↓
考虑ADC/PWM/计算延迟
↓
带宽不能过高
↓
第一次实机使用保守带宽
↓
后续逐步提高并观察响应
```

---

# 三十一、当前学习项目暂定设计

当前：

$$
f_{PWM}=20kHz
$$

第一次电流环调试暂定：

$$
\boxed{
f_c=500Hz
}
$$

原因：

$$
\frac{20kHz}{500Hz}
=
40
$$

即：

```text
Sampling / PWM
:
Current-loop Bandwidth

≈ 40 : 1
```

属于比较保守的起点。

以后电流环跑通后可以尝试：

```text
500 Hz
↓
750 Hz
↓
1 kHz
```

逐渐提高，同时观察：

- 电流阶跃响应
- 超调
- 震荡
- 电流噪声
- Vd/Vq波动
- 稳定性

---

# 三十二、本阶段核心公式

## PMSM dq 电压方程

$$
v_d
=
R_si_d
+
L_d\frac{di_d}{dt}
-
\omega_eL_qi_q
$$

$$
v_q
=
R_si_q
+
L_q\frac{di_q}{dt}
+
\omega_eL_di_d
+
\omega_e\psi_f
$$

---

## RL 电流对象

$$
\boxed{
G(s)
=
\frac{1}{Ls+R}
}
$$

---

## RL 对象极点

$$
\boxed{
s=-\frac RL
}
$$

---

## PI 零点

$$
\boxed{
s=-\frac{K_i}{K_p}
}
$$

---

## 零极点匹配

$$
\boxed{
\frac{K_i}{K_p}
=
\frac RL
}
$$

---

## 电流环带宽

$$
\boxed{
\omega_c=2\pi f_c
}
$$

---

## PI 参数

$$
\boxed{
K_p=L\omega_c
}
$$

$$
\boxed{
K_i=R\omega_c
}
$$

---

## 理想闭环

$$
\boxed{
G_{closed}(s)
=
\frac{\omega_c}
{s+\omega_c}
}
$$

---

## 时间常数

$$
\boxed{
\tau=\frac1{\omega_c}
}
$$

---

## 延迟引起的相位滞后

$$
\boxed{
\phi_d
=
-360^\circ fT_d
}
$$

---

## dq 前馈解耦

$$
\boxed{
v_d^*
=
v_{d,PI}
-
\omega_eL_qi_q
}
$$

$$
\boxed{
v_q^*
=
v_{q,PI}
+
\omega_eL_di_d
+
\omega_e\psi_f
}
$$

---

# 三十三、本阶段真正需要理解的内容

- “dq 电流对象”就是从 Vd/Vq 到 Id/Iq 的电机动态。
- PMSM dq 两轴本身存在交叉耦合。
- q 轴还存在永磁体反电动势。
- dq 解耦后，每个轴可近似为 RL 一阶对象。
- RL 对象传递函数为 $1/(Ls+R)$。
- PI 零点可以与 RL 极点进行匹配。
- 零极点匹配只确定 Ki/Kp 比例。
- 电流环带宽决定 PI 的绝对大小和响应速度。
- 电流环带宽不能接近 PWM / 采样频率。
- 带宽越高，响应越快，但噪声、延迟和参数误差影响越明显。
- 数字 PI 必须考虑采样周期 Ts。
- PI 输出是 Vd/Vq 电压指令，不是电流，也不是 Duty。
- PI 输出必须有限幅。
- 积分器需要 Anti-Windup。
- dq 前馈解耦用于减小轴间耦合和反电动势影响。
- 当前没有真实 θe/ωe，因此暂时不正式加入前馈解耦。
- 第一次实机电流环应从较保守带宽开始。

---

# 三十四、当前阶段进度

已完成：

- [x] Current PI 基本结构
- [x] d/q 两轴 PI
- [x] 数字积分
- [x] PI 输出限幅
- [x] Anti-Windup
- [x] dq 电流对象理解
- [x] PMSM dq 耦合项分析
- [x] RL 一阶对象推导
- [x] PI 零极点匹配
- [x] 电流环带宽概念
- [x] Kp / Ki 理论计算
- [x] 数字延迟对带宽的限制
- [x] dq 前馈解耦原理
- [x] 反电动势补偿原理

当前暂定：

```text
PWM Frequency = 20 kHz
Current-loop Bandwidth = 500 Hz
```

下一阶段：

> **SVPWM：把 Vα/Vβ 转换为三相 PWM Duty，并最终更新 TIM1 CCR1/CCR2/CCR3。**

