# STM32 学习笔记：PMSM Hall 位置传感器、Sector 与速度估计

## 一、当前学习位置

上一阶段已经完成：

```text
三相互补 PWM
↓
PWM同步ADC
↓
Iu / Iv
↓
Clarke
↓
Park
↓
Current PI
↓
Inverse Park
↓
SVPWM
↓
真实 Vdc
↓
FOC状态管理
↓
Fault / Start / Stop
```

但整个 FOC 中仍然存在一个重要的临时量：

```text
theta_e_test
```

即：

> Park 和 Inverse Park 使用的电角度仍然是人工生成的测试角度，而不是真实转子位置。

因此本阶段开始建立：

```text
真实转子位置反馈
```

最终目标：

```text
Hall Sensor
↓
Rotor Position
↓
Electrical Angle
↓
替换 theta_e_test
```

---

# 二、电机位置传感器类型确认

电机型号：

```text
SHINANO KENSHI
LA034-040NN07A
DC105V
25.1W
```

结合：

```text
电机线束
开发板原理图
H16接口
HA / HB / HC
“霍尔/编码器”标识
```

最终确定：

> 当前电机采用三路 Hall 位置传感器。

因此不是：

```text
ABZ Incremental Encoder
```

而是：

```text
Hall A
Hall B
Hall C
```

---

# 三、开发板 Hall 接口

开发板 H16 中：

```text
Pin 7   → +5V
Pin 8   → AGND

Pin 9   → HA
Pin 10  → HB
Pin 11  → HC
```

Hall 信号与 MCU 的连接关系：

```text
HA
↓
TIM4_CH1
↓
PB6

HB
↓
TIM4_CH2
↓
PB7

HC
↓
TIM4_CH3
↓
PB8
```

因此：

```text
PB6 → TIM4_CH1
PB7 → TIM4_CH2
PB8 → TIM4_CH3
```

---

# 四、Hall 输入外围电路

原理图中 HA / HB / HC 每路均包含：

```text
10 kΩ 上拉到 3.3V
+
1.8 kΩ 串联电阻
+
10 pF 对地滤波
```

因此 Hall 输入高电平约：

```text
3.3V
```

适合 STM32 GPIO 输入。

Hall 传感器自身使用：

```text
+5V
```

供电。

---

# 五、Hall 接口电压验证

开发板正常供电后，用万用表测得：

```text
H16 Pin7 ≈ 5V

HA/HB/HC空载时
≈ 3.3V
```

其中最初 HA 曾测得：

```text
≈ 1.65V
```

最终确认原因是：

```text
TIM4_CH1
```

仍然保留了以前 PWM 学习阶段的：

```text
50% PWM
```

因为：

$$
3.3V\times50\%
=
1.65V
$$

删除旧 TIM4 PWM Start 后恢复正常。

因此：

> 旧实验代码在外设功能改变以后必须及时清理。

---

# 六、电机线束

电机共有：

```text
3根较粗相线
+
5根Hall细线
```

Hall 线束包括：

```text
Hall VCC
Hall GND
Hall A
Hall B
Hall C
```

当前 Hall Bring-up 阶段：

```text
三相功率线不接
```

只连接 Hall。

因此：

> Hall 调试完全不需要给电机绕组上功率电压。

---

# 七、为什么第一步不直接做 FOC

Hall Bring-up 初期没有直接计算：

```text
θe
```

而是按照：

```text
Hall电平
↓
Hall State
↓
Sector
↓
Direction
↓
Speed
↓
Continuous Angle
```

逐层验证。

这样一旦出现问题，可以明确知道是哪一层出错。

---

# 八、TIM4 Hall Sensor Mode

TIM4 以前用于普通 PWM 学习。

现在正式修改为：

```text
Hall Sensor Interface
```

主要配置：

```text
TIM4 Prescaler       = 169
TIM4 Period          = 65535
Counter Mode         = Up
Clock Division       = DIV1
Auto Reload Preload  = Disable
```

Hall 参数：

```text
IC1 Polarity          = Rising
IC1 Prescaler         = DIV1
IC1 Filter            = 0
Commutation Delay     = 0
```

并使能：

```text
TIM4 Global Interrupt
```

---

# 九、TIM4 当前计数分辨率

STM32 Timer Clock：

$$
170MHz
$$

Prescaler：

$$
PSC=169
$$

所以：

$$
f_{TIM4}
=
\frac{170MHz}{169+1}
=
1MHz
$$

因此：

$$
\boxed{
1\ Count=1\mu s
}
$$

所以 Hall Capture：

```text
18756
```

表示相邻 Hall 边沿时间约：

$$
18.756ms
$$

---

# 十、启动 Hall Sensor Interface

程序初始化完成后：

```c
if (HAL_TIMEx_HallSensor_Start_IT(&htim4) != HAL_OK)
{
    Error_Handler();
}
```

从此 TIM4 的用途从：

```text
旧：
TIM4 PWM实验
```

正式变成：

```text
TIM4 Hall Position / Speed Interface
```

---

# 十一、Hall 基础变量

```c
volatile uint8_t hall_a = 0;
volatile uint8_t hall_b = 0;
volatile uint8_t hall_c = 0;

volatile uint8_t hall_state = 0;

volatile uint32_t hall_edge_count = 0;
volatile uint32_t hall_capture = 0;
```

规定：

```text
bit2 → HA
bit1 → HB
bit0 → HC
```

因此：

```c
hall_state =
    (hall_a << 2) |
    (hall_b << 1) |
     hall_c;
```

例如：

```text
HA HB HC
1  0  1
```

得到：

```text
hall_state = 101₂ = 5
```

---

# 十二、Hall 状态读取

```c
static void Hall_UpdateState(void)
{
    hall_a =
        (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6)
         == GPIO_PIN_SET);

    hall_b =
        (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)
         == GPIO_PIN_SET);

    hall_c =
        (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8)
         == GPIO_PIN_SET);

    hall_state =
        (hall_a << 2) |
        (hall_b << 1) |
         hall_c;
}
```

---

# 十三、Hall Sensor 的 6 个有效状态

三路 Hall 理论上共有：

$$
2^3=8
$$

种组合：

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

正常三相 Hall 电机只有：

```text
6个有效状态
```

而：

```text
000
111
```

通常属于非法状态。

实际手转电机时成功观察到 6 个合法状态循环变化。

---

# 十四、使用 UART 代替 Debugger 观察 Hall

Hall 状态变化属于：

> 时间序列数据。

Debugger 需要：

```text
Run
↓
Pause
↓
观察
↓
Run
```

不适合连续观察。

因此改用：

```text
TIM4 ISR
↓
保存数据
↓
设置Flag
↓
立即退出

main while
↓
UART打印
```

重要原则：

> 不在 Hall ISR 中直接执行阻塞式 UART。

---

# 十五、Hall 边沿回调

Hall Sensor Mode 中：

```text
Hall状态变化
↓
TIM4 Capture
↓
HAL_TIM_IC_CaptureCallback()
```

回调中执行：

```text
读取 Capture
↓
Edge Count++
↓
读取 Hall State
↓
Sector
↓
Direction
↓
Speed
```

---

# 十六、真实 Hall 顺序

实测规定：

> 从电机出轴方向观察，顺时针定义为 CW。

顺时针：

```text
001
↓
011
↓
010
↓
110
↓
100
↓
101
↓
001
```

十进制：

```text
1 → 3 → 2 → 6 → 4 → 5 → 1
```

逆时针则完全相反：

```text
001
↓
101
↓
100
↓
110
↓
010
↓
011
↓
001
```

即：

```text
1 → 5 → 4 → 6 → 2 → 3 → 1
```

两者互为反序。

这说明：

```text
Hall接线
TIM4 Hall Mode
Hall Interrupt
Hall State
```

均正常。

---

# 十七、Hall State → Sector 映射

最终采用：

```text
001 → Sector 0
011 → Sector 1
010 → Sector 2
110 → Sector 3
100 → Sector 4
101 → Sector 5
```

即：

| Hall | State | Sector |
| ---- | ----: | -----: |
| 001  |     1 |      0 |
| 011  |     3 |      1 |
| 010  |     2 |      2 |
| 110  |     6 |      3 |
| 100  |     4 |      4 |
| 101  |     5 |      5 |

实现：

```c
static int8_t Hall_StateToSector(uint8_t state)
{
    switch (state)
    {
        case 1: return 0;    // 001
        case 3: return 1;    // 011
        case 2: return 2;    // 010
        case 6: return 3;    // 110
        case 4: return 4;    // 100
        case 5: return 5;    // 101

        default:
            return -1;
    }
}
```

---

# 十八、Sector 0 不等于真实 0° 电角度

当前：

```text
001 → Sector 0
```

只是人为定义的：

```text
相对 Sector 编号
```

不能理解为：

$$
001=0^\circ_e
$$

Hall 状态和真实转子磁链之间还存在：

```text
Electrical Angle Offset
```

因此以后真正电角度需要：

```text
Relative Hall Angle
+
Angle Offset
```

这个 Offset 后续通过：

```text
Rotor Alignment
```

确定。

---

# 十九、极对数测量

三路 Hall 每一个完整电周期：

$$
360^\circ_e
$$

发生：

$$
6
$$

次 Hall 边沿。

如果电机极对数为：

$$
p
$$

那么机械转一圈包含：

$$
p
$$

个电周期。

因此一机械圈 Hall 边沿数：

$$
\boxed{
N_{Hall}=6p
}
$$

所以：

$$
\boxed{
p=\frac{N_{Hall}}6
}
$$

通过实际机械转动测量，确定当前电机：

$$
\boxed{
p=2
}
$$

即：

```text
2 Pole Pairs
4 Poles
```

因此：

$$
\boxed{
\theta_e=2\theta_m
}
$$

---

# 二十、一次 Hall 跳变对应多少角度

一个 Hall 电周期：

```text
6次跳变
```

所以一次跳变对应：

$$
\frac{360^\circ_e}{6}
=
60^\circ_e
$$

当前：

$$
p=2
$$

因此对应机械角：

$$
\frac{60^\circ_e}{2}
=
30^\circ_m
$$

所以：

> 当前电机每发生一次 Hall 状态变化，机械转子大约转过 30°。

机械一圈：

$$
360^\circ_m
$$

因此：

$$
360/30=12
$$

次 Hall 边沿。

也就是：

$$
6p=12
$$

---

# 二十一、方向判断

定义：

```c
typedef enum
{
    HALL_DIR_UNKNOWN = 0,

    HALL_DIR_CW  = 1,

    HALL_DIR_CCW = -1

} Hall_Direction_t;
```

顺时针 Sector：

```text
0 → 1 → 2 → 3 → 4 → 5 → 0
```

因此：

```c
new_sector ==
((old_sector + 1) % 6)
```

表示：

```text
CW
```

逆时针：

```text
0 → 5 → 4 → 3 → 2 → 1 → 0
```

因此：

```c
new_sector ==
((old_sector + 5) % 6)
```

表示：

```text
CCW
```

---

# 二十二、方向判断验证

实测：

```text
顺时针
→ Dir = CW
```

逆时针：

```text
→ Dir = CCW
```

并且 Hall Sector 顺序和实际旋转方向完全一致。

因此：

```text
Hall Direction Detection
```

验证完成。

---

# 二十三、Hall Capture 的意义

TIM4 每次 Hall 状态变化时捕获：

```text
hall_capture
```

它表示：

> 前后两个 Hall 边沿之间经历了多少 Timer Count。

当前：

$$
1Count=1\mu s
$$

因此：

```text
Capture = 18756
```

表示：

$$
\Delta t
=
18756\mu s
=
0.018756s
$$

---

# 二十四、为什么可以通过 Hall 算转速

机械一圈共有：

$$
6p
$$

次 Hall 边沿。

所以一次 Hall 边沿代表机械转过：

$$
\frac{1}{6p}
$$

圈。

如果一次边沿间隔：

$$
\Delta t
$$

那么机械转速（转/秒）：

$$
n_{r/s}
=
\frac{1}{6p\Delta t}
$$

转换为 rpm：

$$
1min=60s
$$

所以：

$$
n_{rpm}
=
60
\cdot
\frac{1}{6p\Delta t}
$$

得到：

$$
\boxed{
n=
\frac{60}{6p\Delta t}
}
$$

整理：

$$
\boxed{
n=
\frac{10}{p\Delta t}
}
$$

---

# 二十五、公式中的 60 是什么

公式：

$$
n=
\frac{60}{6p\Delta t}
$$

分子中的：

$$
60
$$

不是：

```text
Hall的60°电角度
```

而是：

$$
\boxed{
1min=60s
}
$$

它只是用于：

```text
转/秒
↓
转/分钟
```

的单位转换。

需要避免混淆：

```text
60°e
→ 一次Hall跳变对应的电角度

60
→ 每分钟60秒
```

---

# 二十六、当前电机转速公式

由于：

$$
p=2
$$

所以：

$$
n=
\frac{10}{2\Delta t}
$$

即：

$$
\boxed{
n=
\frac{5}{\Delta t}
}
$$

其中：

$$
\Delta t
$$

单位必须是秒。

例如：

```text
Capture = 18756 us
```

则：

$$
\Delta t
=
0.018756s
$$

所以：

$$
n
=
\frac{5}{0.018756}
\approx266.6rpm
$$

实际 UART：

```text
RPM ≈ 266.6
```

完全一致。

---

# 二十七、机械角速度

RPM 与机械角速度：

$$
\omega_m
=
n
\frac{2\pi}{60}
$$

单位：

```text
rad/s
```

---

# 二十八、电角速度

电角速度和机械角速度关系：

$$
\boxed{
\omega_e=p\omega_m
}
$$

当前：

$$
p=2
$$

因此：

$$
\boxed{
\omega_e=2\omega_m
}
$$

---

# 二十九、速度计算

当前建立：

```text
hall_speed_rpm
hall_omega_m
hall_omega_e
```

其中：

```text
hall_speed_rpm
→ 机械转速 rpm

hall_omega_m
→ 机械角速度 rad/s

hall_omega_e
→ 电角速度 rad/s
```

---

# 三十、速度计算代码思想

首先：

```c
delta_t =
    (float)capture
    * HALL_TIMER_TICK_S;
```

然后根据方向：

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

因为一次 Hall 跳变：

$$
60^\circ_e
=
\frac{\pi}{3}
$$

所以：

$$
\boxed{
\omega_e=
\pm
\frac{\pi/3}{\Delta t}
}
$$

---

# 三十一、机械角速度

```c
hall_omega_m =
    hall_omega_e
    / MOTOR_POLE_PAIRS;
```

即：

$$
\boxed{
\omega_m=
\frac{\omega_e}{p}
}
$$

---

# 三十二、机械转速

```c
hall_speed_rpm =
    hall_omega_m
    * 60.0f
    / (2.0f * PI_F);
```

即：

$$
\boxed{
n=
\omega_m
\frac{60}{2\pi}
}
$$

---

# 三十三、速度方向约定

当前规定：

```text
CW
→ 正方向

CCW
→ 负方向
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

实际串口验证结果符合这一约定。

---

# 三十四、实际速度验证

例如：

```text
Capture = 18756 us
```

程序输出：

```text
RPM ≈ 266.6
We  ≈ 55.83 rad/s
```

理论：

$$
n=
\frac5{0.018756}
\approx266.6rpm
$$

然后：

$$
\omega_m
=
266.6
\times
\frac{2\pi}{60}
\approx27.92rad/s
$$

由于：

$$
p=2
$$

所以：

$$
\omega_e
=
2\times27.92
\approx55.84rad/s
$$

与实际程序输出一致。

因此：

```text
Hall Capture
↓
RPM
↓
ωm
↓
ωe
```

计算链已经验证正确。

---

# 三十五、当前 Hall 数据链

目前已经形成：

```text
Hall A / B / C
↓
PB6 / PB7 / PB8
↓
TIM4 Hall Sensor Mode
↓
Hall Edge Interrupt
↓
hall_state
↓
Hall State → Sector
↓
Direction
↓
Capture Period
↓
Mechanical RPM
↓
ωm
↓
ωe
```

---

# 三十六、Hall 阶段当前完成情况

已经完成：

- [x] 确认电机使用三路 Hall
- [x] 确认 Hall 接口供电
- [x] 确认 Hall A/B/C 板级连接
- [x] TIM4_CH1 / CH2 / CH3 引脚确认
- [x] PB6 / PB7 / PB8
- [x] TIM4 Hall Sensor Mode
- [x] Hall Interrupt
- [x] UART Hall 实时观察
- [x] 6 个有效 Hall 状态
- [x] 正反转状态顺序
- [x] Hall State → Sector
- [x] 001 → Sector 0
- [x] CW / CCW方向识别
- [x] 极对数测量
- [x] Pole Pairs = 2
- [x] Hall Capture
- [x] RPM计算
- [x] 机械角速度
- [x] 电角速度
- [x] 正反转速度符号验证

因此：

> Hall 基础位置与速度反馈主体已经建立完成。

---

# 三十七、Hall 部分还没有完成的内容

下面这些属于下一阶段完善内容，不影响当前阶段收口。

## 1. 停车超时检测

当前如果最后一次：

```text
RPM = 100
```

然后电机停止，

由于没有新的 Hall 边沿：

```text
RPM可能停留在最后一个值
```

后续需要：

```text
Hall Timeout
↓
速度归零
```

---

## 2. 低速 Timer Overflow

当前：

```text
TIM4 = 1 MHz
ARR = 65535
```

一次最大直接测量：

$$
65.536ms
$$

过低转速时可能发生 Timer Overflow。

后续需要：

```text
降低Timer计数频率
```

或：

```text
增加Overflow处理
```

---

## 3. Speed Valid

后续增加：

```text
hall_speed_valid
```

用于说明：

```text
当前Hall速度是否可信
```

---

## 4. Hall Sector 内角度插值

目前 Hall 只能直接得到：

```text
0
1
2
3
4
5
```

也就是：

$$
60^\circ_e
$$

一级的离散位置。

FOC 需要：

```text
连续 θe
```

因此后续需要：

```text
Hall Sector
+
ωe
+
距离上一Hall边沿的时间
↓
Sector内插值
↓
Continuous θe
```

---

## 5. 绝对电角度 Offset

当前：

```text
001 → Sector 0
```

只是相对编号。

并不知道：

```text
转子真实磁链d轴
```

和 Hall Sector 之间的绝对角度关系。

后续需要：

```text
Rotor Alignment
↓
Electrical Angle Offset
```

---

## 6. 替换 theta_e_test

Hall 连续角度和 Alignment 完成以后：

```text
theta_e_test
```

才能最终退出。

变成：

```text
真实Hall估计 θe
↓
Park
↓
InvPark
```

---

# 三十八、当前系统整体位置

目前：

```text
真实 Iu / Iv        √

真实 Vdc            √

真实 Hall State     √

真实 Hall Sector    √

真实 Direction      √

真实 RPM            √

真实 ωe             √

连续真实 θe         尚未完成
```

所以当前距离真实 FOC 最大的剩余任务就是：

```text
Hall离散位置
↓
连续电角度
↓
Alignment
↓
真正接入Park / InvPark
```

---

# 三十九、本阶段最重要的认识

- 三路 Hall 每个电周期产生 6 次状态跳变。
- 一个 Hall Sector 对应 60° 电角度。
- 极对数决定电角度和机械角度之间的比例。
- 当前电机极对数为 2。
- 因此一次 Hall 跳变对应 30°机械角。
- 机械一圈共有 12 次 Hall 边沿。
- Hall 状态顺序可以判断旋转方向。
- Hall Capture 可以计算边沿时间。
- 边沿时间可以计算机械转速。
- 机械转速可以进一步得到机械角速度和电角速度。
- Sector 编号只是相对位置，不等于真实绝对电角度。
- 真正 FOC 仍需连续电角度和 Alignment Offset。
