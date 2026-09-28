# STM32 学习笔记：Hall 低速处理、速度有效性与连续相对电角度

## 一、本阶段所在位置

上一阶段已经完成 Hall 基础位置与速度反馈：

```text
Hall A / B / C
↓
Hall State
↓
Sector
↓
Direction
↓
Capture Period
↓
RPM
↓
ωm
↓
ωe
```

并已经确认：

```text
Hall Mapping：

001 → Sector 0
011 → Sector 1
010 → Sector 2
110 → Sector 3
100 → Sector 4
101 → Sector 5
```

电机极对数：

$$
\boxed{p=2}
$$

旋转方向约定：

```text
CW  → 正方向
CCW → 负方向
```

因此：

```text
CW：
RPM > 0
ωe  > 0

CCW：
RPM < 0
ωe  < 0
```

本阶段的主要目标是：

```text
Hall离散位置
↓
Hall速度完善
↓
Sector内插值
↓
连续相对电角度 hall_theta_rel
```

---

# 二、原 Hall 速度估计存在的问题

原 TIM4 配置：

```text
Timer Clock = 170 MHz
PSC = 169
ARR = 65535
```

因此：

$$
f_{TIM4}
=
\frac{170MHz}{169+1}
=
1MHz
$$

即：

$$
1\ Count=1\mu s
$$

最大计时范围：

$$
65536\mu s
=
65.536ms
$$

对于当前电机：

$$
p=2
$$

机械转速公式：

$$
n=\frac{5}{\Delta t}
$$

当：

$$
\Delta t=65.536ms
$$

最低直接测速范围约为：

$$
n\approx76.3rpm
$$

因此原配置存在三个问题：

```text
1. 低速测量范围过窄

2. 电机停止后没有新的Hall边沿，
   RPM会保持最后一次数值

3. 程序不知道当前速度是否仍然可信
```

---

# 三、TIM4 改为 100 kHz

将：

```text
PSC = 169
```

修改为：

```text
PSC = 1699
```

于是：

$$
f_{TIM4}
=
\frac{170MHz}{1699+1}
=
100kHz
$$

因此：

$$
\boxed{
1\ Count=10\mu s
}
$$

ARR 继续：

```text
65535
```

最大计时时间变为：

$$
65536\times10\mu s
=
655.36ms
$$

最低直接测速范围降低到：

$$
n
=
\frac5{0.65536}
\approx7.63rpm
$$

因此相比原来的约 76 rpm：

```text
低速范围扩大约10倍
```

---

# 四、修改 Timer Tick

原软件：

```c
#define HALL_TIMER_TICK_S 0.000001f
```

代表：

$$
1\mu s
$$

修改为：

```c
#define HALL_TIMER_TICK_S 0.00001f
```

即：

$$
10\mu s
$$

因此以后：

```c
delta_t =
    (float)capture
    * HALL_TIMER_TICK_S;
```

统一把 Capture 转换成真实时间。

---

# 五、增加 Hall Speed Valid

新增：

```c
volatile uint8_t hall_speed_valid = 0;
```

含义：

```text
hall_speed_valid = 1
→ 当前Hall速度估计有效

hall_speed_valid = 0
→ 当前速度不能继续作为可信反馈
```

工程上：

> “变量里有一个数值”不等于“这个数值当前有效”。

因此实际控制系统中通常需要：

```text
sensor_valid
speed_valid
position_valid
current_valid
```

等有效性标志。

---

# 六、Hall 速度更新逻辑

Hall 速度依然通过：

$$
\omega_e
=
\pm
\frac{\pi/3}{\Delta t}
$$

计算。

其中：

$$
\frac{\pi}{3}
=
60^\circ_e
$$

方向符号：

```c
direction_sign =
    (hall_direction == HALL_DIR_CW)
    ? 1.0f
    : -1.0f;
```

电角速度：

```c
hall_omega_e =
    direction_sign
    * (PI_F / 3.0f)
    / delta_t;
```

机械角速度：

```c
hall_omega_m =
    hall_omega_e
    / MOTOR_POLE_PAIRS;
```

机械转速：

```c
hall_speed_rpm =
    hall_omega_m
    * 60.0f
    / (2.0f * PI_F);
```

计算成功后：

```c
hall_speed_valid = 1;
```

---

# 七、为什么需要 Hall Timeout

假设最后一次得到：

```text
RPM = 150
```

随后电机停止。

由于：

```text
转子停止
↓
没有Hall边沿
↓
TIM4 Capture中断不再发生
↓
Hall_UpdateSpeed()不再执行
```

所以：

```text
hall_speed_rpm
```

会一直保持：

```text
150 rpm
```

这显然不能代表真实状态。

因此需要：

```text
Hall Timeout
```

---

# 八、记录最后一次 Hall 边沿时间

增加：

```c
volatile uint32_t hall_last_edge_ms = 0;
```

每次 Hall 边沿发生时：

```c
hall_last_edge_ms = HAL_GetTick();
```

`HAL_GetTick()` 单位：

```text
ms
```

用于判断：

```text
距离上一次Hall边沿已经过去多久
```

---

# 九、停车超时

设置：

```c
#define HALL_STOP_TIMEOUT_MS 500U
```

也就是：

$$
500ms
$$

如果 500 ms 内没有新的 Hall 边沿：

```text
速度归零
↓
ωm归零
↓
ωe归零
↓
speed_valid = 0
↓
direction = UNKNOWN
```

典型逻辑：

```c
if ((uint32_t)(HAL_GetTick() - hall_last_edge_ms)
    >= HALL_STOP_TIMEOUT_MS)
{
    hall_speed_rpm = 0.0f;
    hall_omega_m = 0.0f;
    hall_omega_e = 0.0f;

    hall_speed_valid = 0;

    hall_direction = HALL_DIR_UNKNOWN;
    hall_prev_sector = -1;
}
```

---

# 十、为什么停车后 Direction 清零

假设：

```text
先CW
↓
停车
↓
重新CCW
```

如果仍然保存：

```text
Direction = CW
```

那么重新运动初期可能继续使用旧方向。

因此停车后：

```c
hall_direction =
    HALL_DIR_UNKNOWN;

hall_prev_sector = -1;
```

重新运动后：

```text
第一个Hall状态
→ 记录位置

第二个Hall状态
→ 根据前后Sector重新判断方向
```

---

# 十一、为什么停车后 Sector 不清零

停车代表：

```text
速度 = 0
```

但不代表：

```text
位置未知
```

例如：

```text
Hall = 100
Sector = 4
RPM = 0
```

仍然完全合理。

因此：

```text
Speed Invalid
≠
Position Invalid
```

只要 Hall 状态合法：

```text
001
011
010
110
100
101
```

仍然可以知道转子所在的 60° 电角度区间。

---

# 十二、Hall 离散位置为什么不能直接用于 FOC

Hall 一个电周期只有 6 个状态：

$$
360^\circ_e / 6
=
60^\circ_e
$$

如果直接使用：

$$
\theta_e
=
Sector\times60^\circ
$$

得到：

```text
0°
0°
0°
0°
↓
60°
60°
60°
↓
120°
```

这是离散阶跃角度。

而 Park / Inverse Park 更希望获得：

```text
0°
5°
10°
15°
...
55°
60°
65°
...
```

因此需要：

```text
Hall Sector 内插值
```

---

# 十三、Hall 连续角度的基本思想

利用已有：

```text
Hall Sector
+
Direction
+
ωe
+
距离最近一次Hall边沿的时间
```

估算 Sector 内的位置。

基本运动关系：

$$
\boxed{
\theta
=
\theta_0
+
\omega t
}
$$

对应 Hall：

$$
\boxed{
\theta_{hall}
=
\theta_{base}
+
\omega_e\Delta t
}
$$

其中：

```text
θbase
→ 刚进入当前Hall Sector时的边界角

ωe
→ 当前估计电角速度

Δt
→ 距离最新Hall边沿的时间
```

---

# 十四、增加连续相对电角度变量

```c
#define HALL_SECTOR_ANGLE_RAD  (PI_F / 3.0f)
#define TWO_PI_F               (2.0f * PI_F)

volatile float hall_theta_rel = 0.0f;

volatile uint8_t hall_angle_valid = 0;
```

其中：

```text
hall_theta_rel
→ Hall坐标系下的连续相对电角度

hall_angle_valid
→ Hall角度估计当前是否有效
```

---

# 十五、为什么称为“相对电角度”

当前人为规定：

```text
001 → Sector 0
```

但：

```text
Sector 0
```

只是人为编号。

并不能说明：

$$
Hall=001
\Rightarrow
\theta_e=0^\circ
$$

实际转子磁链 d 轴与 Hall 安装位置之间仍存在：

$$
\theta_{offset}
$$

因此当前得到：

```text
hall_theta_rel
```

只能称为：

> Hall 相对电角度。

未来最终真实 FOC 电角度需要：

$$
\boxed{
\theta_e
=
Wrap(
hall\_theta\_rel
+
\theta_{offset}
)
}
$$

其中：

```text
θoffset
```

需要后续 Rotor Alignment 标定。

---

# 十六、角度归一化

连续角度可能：

```text
> 2π
```

或：

```text
< 0
```

因此增加：

```c
static float WrapAngle0To2Pi(float angle)
{
    while (angle >= TWO_PI_F)
    {
        angle -= TWO_PI_F;
    }

    while (angle < 0.0f)
    {
        angle += TWO_PI_F;
    }

    return angle;
}
```

最终保证：

$$
\boxed{
0\leq\theta<2\pi
}
$$

---

# 十七、CW 时的 Sector Base Angle

顺时针定义：

```text
Sector 0
↓
Sector 1
↓
Sector 2
↓
Sector 3
↓
Sector 4
↓
Sector 5
↓
Sector 0
```

因此顺时针进入 Sector 后：

$$
\boxed{
\theta_{base}
=
Sector
\times
\frac{\pi}{3}
}
$$

例如：

```text
Sector 0 →   0°
Sector 1 →  60°
Sector 2 → 120°
Sector 3 → 180°
Sector 4 → 240°
Sector 5 → 300°
```

---

# 十八、CCW 时为什么 Base Angle 不一样

逆时针：

```text
Sector 3
↓
Sector 2
```

刚进入 Sector 2 时，实际经过的边界不是：

```text
120°
```

而是：

```text
180°
```

随后：

```text
180°
↓
170°
↓
160°
↓
...
↓
120°
```

因此 CCW：

$$
\boxed{
\theta_{base}
=
(Sector+1)
\frac{\pi}{3}
}
$$

然后由于：

$$
\omega_e<0
$$

角度会自然向减小方向变化。

---

# 十九、TIM4 CNT 的作用

Hall Capture：

```text
hall_capture
```

表示：

> 上一个完整 Hall Sector 所经历的时间。

而当前：

```c
__HAL_TIM_GET_COUNTER(&htim4)
```

表示：

> 从最近一个 Hall 边沿开始，到现在已经过去的时间。

因此：

```text
hall_capture
→ 用于计算速度

TIM4 CNT
→ 用于Sector内部角度插值
```

两者用途不同。

---

# 二十、Sector 内经过时间

当前 TIM4：

$$
1Count=10\mu s
$$

因此：

```c
elapsed_count =
    __HAL_TIM_GET_COUNTER(&htim4);

elapsed_time =
    (float)elapsed_count
    * HALL_TIMER_TICK_S;
```

得到：

$$
\Delta t
$$

---

# 二十一、Sector 内插值

首先计算：

```c
delta_angle =
    hall_omega_e
    * elapsed_time;
```

即：

$$
\boxed{
\Delta\theta
=
\omega_e\Delta t
}
$$

最终：

```c
hall_theta_rel =
    WrapAngle0To2Pi(
        base_angle
        + delta_angle);
```

---

# 二十二、为什么要限制最大插值角度

Hall 已经明确告诉程序：

> 当前仍然处于某一个 60° Sector。

因此使用速度外推时，不应该让角度无限跑出当前 Sector。

所以限制：

```c
if (delta_angle >
    HALL_SECTOR_ANGLE_RAD)
{
    delta_angle =
        HALL_SECTOR_ANGLE_RAD;
}

if (delta_angle <
    -HALL_SECTOR_ANGLE_RAD)
{
    delta_angle =
        -HALL_SECTOR_ANGLE_RAD;
}
```

即：

$$
|\Delta\theta|
\leq
60^\circ_e
$$

这样可以防止：

```text
转子已经减速
但旧ωe仍然继续外推
```

导致角度远离真实 Hall Sector。

---

# 二十三、低速或停车时的角度处理

当：

```text
hall_speed_valid = 0
```

Hall 仍然可以告诉：

```text
当前在哪个60°区间
```

但无法知道：

```text
在Sector内部的具体位置
```

因此采用 Sector 中心作为粗略位置：

$$
\boxed{
\theta
=
(Sector+0.5)
\times
60^\circ
}
$$

例如：

```text
Sector 0
```

范围：

$$
0^\circ\sim60^\circ
$$

中心：

$$
30^\circ
$$

最大误差大约：

$$
\pm30^\circ_e
$$

---

# 二十四、连续角度为什么必须在高速控制周期更新

如果只在 Hall 中断中计算：

```text
Hall边沿
↓
算一次θ
↓
等待下一个Hall边沿
```

仍然只能得到每 60° 更新一次的离散角度。

因此：

```c
Hall_UpdateElectricalAngle();
```

需要放在：

```text
PWM同步ADC控制周期
```

中执行。

当前控制周期约：

$$
20kHz
$$

即：

$$
50\mu s
$$

更新一次连续角度。

---

# 二十五、Hall 中断与 ADC 中断职责分工

Hall ISR：

```text
Hall Edge
↓
读取Capture
↓
Hall State
↓
Sector
↓
Direction
↓
Speed
```

ADC / FOC ISR：

```text
每50μs
↓
读取TIM4 CNT
↓
Hall_UpdateElectricalAngle()
↓
更新 hall_theta_rel
```

因此：

> Hall 中断负责提供离散位置和速度信息；

> ADC 控制周期负责生成连续电角度。

---

# 二十六、调试中出现的 ThetaHall 不更新问题

最初 UART 中观察到：

```text
ThetaHall = 2.618
```

长期保持不变。

但同时：

```text
Sector变化
RPM变化
ωe变化
```

因此一开始怀疑：

```text
Hall_UpdateElectricalAngle()
```

没有持续执行。

增加：

```c
volatile uint32_t
hall_angle_update_count = 0;
```

发现：

```text
hall_angle_update_count
≈ 99
```

之后停止。

进一步增加 ADC Callback 计数：

```c
adc_callback_test_count
```

发现：

```text
adc_callback_test_count
≈ 2200
```

之后也停止。

因此问题并不是：

```text
Hall角度算法错误
```

而是：

```text
整个ADC同步采样链停止
```

---

# 二十七、最终定位：Vdc Overvoltage Fault

Debugger 检查后发现：

```text
foc_fault_code
=
FOC_FAULT_VDC_OVERVOLTAGE
```

原因是之前软件保护测试阶段设置过：

```text
UV Test Threshold ≈ 0.5 V
OV Test Threshold ≈ 5 V
```

这组数值的目的只是：

```text
验证欠压Fault
验证过压Fault
验证Fault状态机
验证PWM紧急关闭
```

并不是实际母线保护参数。

---

# 二十八、为什么接入 24 V 后立即 Fault

当前使用：

```text
24 V电源
```

而软件仍保留：

```text
Overvoltage threshold ≈ 5 V
```

自然满足：

```text
Vdc > Overvoltage Threshold
```

经过保护确认计数以后：

```text
FOC_FAULT_VDC_OVERVOLTAGE
```

被触发。

保护链：

```text
24V母线
↓
超过旧5V测试阈值
↓
Overvoltage Fault
↓
FOC_TriggerFault()
↓
TIM1 MOE被紧急关闭
↓
TIM1 CH4同步触发停止
↓
Injected ADC停止
↓
ADC ISR停止
↓
Hall_UpdateElectricalAngle()停止
↓
ThetaHall保持最后一个值
```

因此整个异常现象被完整解释。

---

# 二十九、Vdc 阈值修改原则

原来的：

```text
0.5 V / 5 V
```

只属于：

```text
软件保护测试参数
```

接入实际 24 V 母线后必须重新设置。

修改后：

```text
Vdc Fault不再误触发
ADC同步采样恢复
Hall角度更新恢复
```

注意：

> 当前 24 V 调试阈值仍然不应直接视为最终产品保护参数。

最终欠压、过压阈值必须依据：

```text
母线额定电压
MOSFET / IGBT耐压
母线电容耐压
Gate Driver允许范围
电源工作范围
硬件保护设计
```

确定。

---

# 三十、连续 Hall 电角度最终验证

修正 Vdc Fault 阈值以后：

```text
hall_theta_rel
```

开始持续变化。

实测可以观察到：

```text
Sector持续变化

RPM持续变化

ωe持续变化

ThetaHall持续变化

Valid = 1
```

因此：

```text
Hall State
↓
Sector
↓
Direction
↓
Speed
↓
TIM4 CNT
↓
Sector内插值
↓
hall_theta_rel
```

数据链已经正常运行。

---

# 三十一、UART 中 Theta 与 Sector 偶尔看起来不对应

例如可能看到：

```text
ThetaHall ≈ 5.236
Sector = 0
```

其中：

$$
5.236rad
\approx300^\circ
$$

看起来更像 Sector 5。

原因通常不是角度算法错误，而是：

```text
Hall中断
↓
Sector立即更新
↓
UART Flag置位
↓
主循环准备打印

但 hall_theta_rel
需要等下一次ADC ISR更新
```

所以打印瞬间可能出现：

```text
新Sector
+
上一次控制周期的ThetaHall
```

这是两个异步更新变量的显示时序问题。

因此：

> UART 日志不是严格原子快照。

若以后需要精确诊断：

```text
先同时Snapshot
Sector
Theta
RPM
ωe
↓
再UART打印
```

即可。

---

# 三十二、当前 Hall 角度仍然不能直接代替最终 theta_e

虽然：

```text
hall_theta_rel
```

现在已经连续，

但它仍然建立在：

```text
001 → Sector 0
```

这个人为定义上。

真正 FOC 需要：

> 转子永磁体 d 轴相对于定子 α 轴的真实电角度。

因此还缺：

$$
\boxed{
\theta_{offset}
}
$$

最终：

$$
\boxed{
\theta_e
=
Wrap(
hall\_theta\_rel
+
\theta_{offset}
)
}
$$

---

# 三十三、下一阶段

下一阶段正式进行：

```text
Rotor Alignment
↓
建立已知定子磁场方向
↓
转子d轴对齐
↓
读取Hall相对角度
↓
计算Electrical Angle Offset
↓
得到真实 θe
```

随后才能：

```text
theta_e_test
↓
退出

真实 theta_e
↓
Park
↓
Current PI
↓
Inverse Park
↓
Min-Max SVPWM
```

---

# 三十四、当前系统总体完成情况

```text
真实 Vdc                   √

真实 Iu / Iv               √

Hall State                 √

Hall Sector                √

Hall Direction             √

Hall RPM                   √

Hall ωm                    √

Hall ωe                    √

Hall Speed Valid           √

Hall Timeout               √

Hall低速测量范围完善         √

Hall连续相对电角度           √

Absolute Electrical Offset ×

真实最终 θe                ×

Rotor Alignment            ×
```

因此目前已经完成：

> Hall 位置与速度反馈的主体，以及基于 Hall 的连续相对电角度估计。

下一阶段核心问题不再是：

```text
“转子有没有在运动？”
```

而是：

```text
“Hall相对坐标系与真实转子d轴坐标系之间到底差多少角度？”
```

这就是 Rotor Alignment 与 Electrical Angle Offset Calibration 要解决的问题。
