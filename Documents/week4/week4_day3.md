# STM32 学习笔记：真实母线电压采样与 FOC 状态管理

## 一、当前学习位置

上一阶段已经完成：

```text
Current PI
↓
Vd / Vq
↓
Inverse Park
↓
Vα / Vβ
↓
Voltage Vector Limit
↓
Min-Max SVPWM
↓
Duty U/V/W
↓
CCR1 / CCR2 / CCR3
↓
TIM1三相PWM
```

但当时仍存在两个明显的临时量：

```text
theta_e_test      → 人工电角度
SVPWM_TEST_VDC    → 固定24V母线电压
```

本阶段首先把固定的：

```text
SVPWM_TEST_VDC = 24V
```

替换为 ADC 实际采集的母线电压。

随后开始建立正式的：

```text
FOC State Management
```

使程序不再是：

```text
上电
↓
Offset完成
↓
PI自动开始工作
```

而逐渐形成真正电机控制软件的运行状态。

---

# 二、真实 Vdc 为什么必须进入 SVPWM

SVPWM 最终将：

$$
V_\alpha,V_\beta
$$

转换成三相 Duty。

Min-Max SVPWM 中，Duty 与母线电压直接有关：

$$
D
=
0.5+\frac{V'}{V_{dc}}
$$

同时电压矢量的线性调制上限为：

$$
\boxed{
V_{max}
=
\frac{V_{dc}}{\sqrt3}
}
$$

因此：

> SVPWM 必须知道实际母线电压，才能正确进行电压限幅和 Duty 计算。

如果实际：

$$
V_{dc}=24V
$$

而程序错误认为：

$$
V_{dc}=30V
$$

则计算得到的 Duty 会偏小。

反之，如果实际母线电压更高，而程序使用较小的固定值，则计算 Duty 会偏大。

所以固定：

```c
#define SVPWM_TEST_VDC 24.0f
```

只能用于前期算法测试。

真正控制系统必须使用：

```text
ADC测量
↓
真实Vdc
↓
SVPWM
```

---

# 三、当前 Vdc 采样结构

目前电流和母线电压采用不同 ADC 工作方式：

```text
ADC1 Regular + DMA
→ Vdc

ADC1 Injected
→ Iu

ADC2 Injected
→ Iv
```

这体现了 Regular ADC 和 Injected ADC 不同的用途。

---

## Vdc

母线电压变化相对慢，不要求每个 PWM 周期在严格固定位置进行采样。

因此采用：

```text
ADC Regular
+
DMA
```

即可持续获得母线电压。

---

## Iu / Iv

相电流直接用于高速电流环：

```text
Current
↓
Clarke
↓
Park
↓
PI
```

必须和 PWM 在固定位置同步采样。

因此使用：

```text
TIM1 Trigger
↓
ADC Injected
```

---

# 四、Vdc DMA 变量

使用：

```c
volatile uint16_t vdc_adc_raw = 0;
volatile float vdc_bus = 0.0f;
```

其中：

```text
vdc_adc_raw
```

是 ADC DMA 自动更新的原始值。

```text
vdc_bus
```

是根据硬件分压比例换算后的实际母线电压。

---

# 五、Vdc ADC DMA

ADC1 Regular 专门用于母线电压：

```c
HAL_ADC_Start_DMA(&hadc1,
                  (uint32_t *)&vdc_adc_raw,
                  1);
```

因此数据链为：

```text
母线电压
↓
硬件分压
↓
ADC1_IN1
↓
ADC1 Regular
↓
DMA
↓
vdc_adc_raw
```

---

# 六、ADC 原始值转换成实际 Vdc

母线电压经过硬件分压后进入 ADC。

基本关系为：

$$
V_{ADC}
=
\frac{ADC_{raw}}
{4095}
V_{REF}
$$

然后根据分压比例：

$$
V_{dc}
=
V_{ADC}
\times K_{divider}
$$

因此：

$$
\boxed{
V_{dc}
=
ADC_{raw}
\times
VDC\_VOLTS\_PER\_COUNT
}
$$

程序中：

```c
vdc_bus =
    (float)vdc_adc_raw
    * VDC_VOLTS_PER_COUNT;
```

---

# 七、Vdc 宏定义思想

不建议直接把换算系数写成一个没有含义的小数。

更合理的是根据硬件参数定义：

```c
#define ADC_FULL_SCALE       4095.0f
#define ADC_VREF             3.3f

#define VDC_R_HIGH           ...
#define VDC_R_LOW            ...

#define VDC_DIVIDER_RATIO \
    ((VDC_R_HIGH + VDC_R_LOW) / VDC_R_LOW)

#define VDC_VOLTS_PER_COUNT \
    ((ADC_VREF * VDC_DIVIDER_RATIO) / ADC_FULL_SCALE)
```

这样代码可以直接反映：

```text
ADC量程
ADC参考电压
母线分压网络
```

以后硬件修改时，也比较容易维护。

---

# 八、用真实 Vdc 替换测试值

原来的：

```c
SVPWM_LimitVoltage(&v_alpha_cmd,
                   &v_beta_cmd,
                   SVPWM_TEST_VDC);

SVPWM_Run(v_alpha_cmd,
          v_beta_cmd,
          SVPWM_TEST_VDC,
          &svpwm_out);
```

修改为：

```c
SVPWM_LimitVoltage(&v_alpha_cmd,
                   &v_beta_cmd,
                   vdc_bus);

SVPWM_Run(v_alpha_cmd,
          v_beta_cmd,
          vdc_bus,
          &svpwm_out);
```

于是整个链路变成：

```text
ADC1 Regular
↓
vdc_adc_raw
↓
vdc_bus
↓
Voltage Vector Limit
↓
SVPWM
↓
Duty
```

---

# 九、真实 Vdc 验证

当前使用万用表直接测量母线电压，同时在 Debugger 中观察：

```text
vdc_bus
```

实测结果：

```text
万用表 ≈ 2V
Debugger vdc_bus ≈ 2V
```

两者基本一致。

这说明当前：

```text
硬件分压
↓
ADC
↓
DMA
↓
软件换算
↓
vdc_bus
```

这条采样链已经基本打通。

以后如果条件允许，还可以在多个安全低压工作点进一步校验：

```text
5V
12V
24V
...
```

用于验证整个量程的换算线性。

---

# 十、Vdc 不只是给 SVPWM 使用

母线电压以后还会参与：

```text
SVPWM电压限幅
欠压保护
过压保护
母线异常检测
故障状态判断
```

因此：

```text
vdc_bus
```

会成为整个电机控制系统中的重要状态量。

---

# 十一、为什么开始建立 FOC 状态管理

原来的高速控制逻辑大致为：

```text
上电
↓
ADC开始采样
↓
Offset校准
↓
Offset完成
↓
直接执行Clarke
↓
Park
↓
PI
↓
InvPark
↓
SVPWM
```

这种结构用于学习和快速验证可以使用。

但真正控制程序不能：

> Offset 一完成就自动开始电机控制。

因为真正系统还需要判断：

```text
系统是否初始化完成？
Offset是否完成？
是否允许启动？
是否发生Fault？
是否要求停止？
```

所以需要引入：

```text
FOC State Machine
```

---

# 十二、第一版 FOC 状态

当前设计五个基础状态：

```text
INIT

CALIBRATION

READY

RUN

FAULT
```

---

# 十三、INIT

```text
FOC_STATE_INIT
```

表示：

> MCU 和控制程序处于初始化阶段。

这个阶段完成：

```text
Clock
GPIO
ADC
TIM
OPAMP
UART
PI参数
控制变量
```

等初始化。

原则：

```text
INIT状态不能执行真正FOC控制。
```

---

# 十四、CALIBRATION

```text
FOC_STATE_CALIBRATION
```

用于：

```text
Current Offset Calibration
```

目前采集：

```text
1000组同步电流ADC数据
```

计算：

```text
iu_offset_adc
iv_offset_adc
```

此时：

```text
ADC采样       √
Offset计算    √

Current PI    ×
FOC输出       ×
```

校准完成后：

```text
CALIBRATION
↓
READY
```

---

# 十五、READY

```text
FOC_STATE_READY
```

表示：

> 基础条件已经准备好，但还没有正式启动控制。

READY 可以继续做：

```text
ADC采样
↓
电流换算
↓
Clarke
↓
Park
```

因此 Debugger 仍然能够观察：

```text
Iu
Iv
Iα
Iβ
Id
Iq
```

但是：

```text
Current PI ×
InvPark    ×
SVPWM控制  ×
```

这样 PI 不会在系统还没有真正进入控制状态时不断积分。

---

# 十六、RUN

```text
FOC_STATE_RUN
```

是唯一真正执行完整 FOC 控制链的状态。

RUN：

```text
ADC
↓
Current Conversion
↓
Clarke
↓
Park
↓
Current PI
↓
InvPark
↓
Voltage Limit
↓
SVPWM
↓
CCR Update
```

即：

\[
\boxed{
\text{只有RUN状态允许Current PI工作}
}
\]

---

# 十七、FAULT

```text
FOC_STATE_FAULT
```

用于处理未来的：

```text
Over Current
Over Voltage
Under Voltage
Gate Driver Fault
Hardware Break
其他异常
```

当前阶段首先建立软件结构。

真正：

```text
PWM Disable
Gate Driver Disable
TIM1 Break
Fault Reset
```

等保护逻辑后续继续完善。

---

# 十八、FOC 状态枚举

定义：

```c
typedef enum
{
    FOC_STATE_INIT = 0,

    FOC_STATE_CALIBRATION,

    FOC_STATE_READY,

    FOC_STATE_RUN,

    FOC_STATE_FAULT

} FOC_State_t;
```

这样程序不再使用难以理解的：

```text
0
1
2
3
4
```

而使用：

```text
FOC_STATE_READY
FOC_STATE_RUN
...
```

提高代码可读性。

---

# 十九、FOC 状态变量

定义：

```c
volatile FOC_State_t foc_state = FOC_STATE_INIT;

volatile uint8_t foc_start_request = 0;
volatile uint8_t foc_stop_request  = 0;
volatile uint8_t foc_fault_request = 0;
```

这里引入一个新的思想：

> 外部命令不直接随便修改控制状态，而是提出 Request，由状态管理器决定状态是否可以转换。

例如：

```text
foc_start_request = 1
↓
State Manager检查当前状态
↓
READY允许启动
↓
RUN
```

---

# 二十、为什么使用 Request

如果代码中到处直接：

```c
foc_state = FOC_STATE_RUN;
```

那么以后状态越来越多时，很容易发生：

```text
未完成Offset也进入RUN
FAULT时仍然被其他代码切换到RUN
运行逻辑分散
```

因此采用：

```text
Request
↓
State Manager
↓
State Transition
```

把状态转换集中管理。

---

# 二十一、控制器 Reset 函数

建立：

```c
static void FOC_ResetControl(void)
{
    PI_Reset(&pi_id);
    PI_Reset(&pi_iq);

    vd_cmd = 0.0f;
    vq_cmd = 0.0f;

    v_alpha_cmd = 0.0f;
    v_beta_cmd  = 0.0f;
}
```

作用：

```text
PI积分状态
+
控制器输出状态
↓
全部清零
```

适用于：

```text
启动控制前
停止控制时
Fault时
重新初始化时
```

---

# 二十二、中性 PWM

当前阶段使用：

```text
Duty U = 50%
Duty V = 50%
Duty W = 50%
```

作为零线电压测试状态。

函数：

```c
static void FOC_SetNeutralPWM(void)
{
    uint32_t arr;
    uint32_t mid;

    arr =
        __HAL_TIM_GET_AUTORELOAD(&htim1);

    mid =
        (arr + 1U) / 2U;

    __HAL_TIM_SET_COMPARE(&htim1,
                          TIM_CHANNEL_1,
                          mid);

    __HAL_TIM_SET_COMPARE(&htim1,
                          TIM_CHANNEL_2,
                          mid);

    __HAL_TIM_SET_COMPARE(&htim1,
                          TIM_CHANNEL_3,
                          mid);
}
```

因为三相相同：

$$
V_u=V_v=V_w
$$

所以理论线电压：

$$
V_{uv}=V_u-V_v=0
$$

$$
V_{vw}=V_v-V_w=0
$$

$$
V_{wu}=V_w-V_u=0
$$

---

# 二十三、50% Duty 不等于真正安全关断

需要特别注意：

```text
50% / 50% / 50%
```

表示的是：

> 三相平均线电压为零。

但功率器件仍可能在进行 PWM 开关。

所以真正的：

```text
FAULT
DISABLE
```

不能只依赖中性 PWM。

以后还需要：

```text
TIM1 Main Output Disable
Gate Driver Disable
Break Input
硬件保护
```

等机制。

---

# 二十四、FOC State Manager

第一版状态管理器的思想：

```c
static void FOC_StateManager(void)
{
    if (foc_fault_request)
    {
        foc_fault_request = 0;

        FOC_ResetControl();
        FOC_SetNeutralPWM();

        foc_state = FOC_STATE_FAULT;
    }

    switch (foc_state)
    {
        case FOC_STATE_INIT:
        {
            break;
        }

        case FOC_STATE_CALIBRATION:
        {
            break;
        }

        case FOC_STATE_READY:
        {
            if (foc_start_request)
            {
                foc_start_request = 0;

                FOC_ResetControl();

                foc_state = FOC_STATE_RUN;
            }

            break;
        }

        case FOC_STATE_RUN:
        {
            if (foc_stop_request)
            {
                foc_stop_request = 0;

                FOC_ResetControl();
                FOC_SetNeutralPWM();

                foc_state = FOC_STATE_READY;
            }

            break;
        }

        case FOC_STATE_FAULT:
        {
            FOC_ResetControl();
            FOC_SetNeutralPWM();

            break;
        }

        default:
        {
            FOC_ResetControl();
            FOC_SetNeutralPWM();

            foc_state = FOC_STATE_FAULT;

            break;
        }
    }
}
```

---

# 二十五、State Manager 为什么放在 while(1)

状态管理处理的是：

```text
Start
Stop
Fault
状态切换
```

这些并不需要：

```text
20 kHz
```

高速运行。

因此：

```c
while (1)
{
    FOC_StateManager();

    /*
     * UART
     * Low-speed tasks
     */
}
```

是合理的。

---

# 二十六、高速控制仍然放在 ADC ISR

真正需要和 PWM 同步高速执行的是：

```text
Current Sampling
↓
Current Calculation
↓
Clarke
↓
Park
↓
Current PI
↓
InvPark
↓
SVPWM
↓
CCR
```

所以它们仍然位于：

```text
ADC Injected Conversion Complete ISR
```

中。

由：

```text
foc_state
```

决定执行到哪一步。

---

# 二十七、状态机与 ADC ISR 的关系

这是当前阶段最重要的结构。

ADC ISR 可以理解成：

```text
ADC数据到齐
↓
CALIBRATION？
├─ Yes → Offset Calibration → return
└─ No
     ↓
INIT / FAULT？
├─ Yes → return
└─ No
     ↓
Current Conversion
↓
Clarke
↓
Park
↓
READY？
├─ Yes → return
└─ No
     ↓
RUN？
├─ No → return
└─ Yes
     ↓
Current PI
↓
InvPark
↓
Voltage Limit
↓
SVPWM
↓
CCR
```

---

# 二十八、CALIBRATION 的 ISR 结构

```c
if (foc_state == FOC_STATE_CALIBRATION)
{
    iu_offset_sum += iu_raw_sync;
    iv_offset_sum += iv_raw_sync;

    current_offset_count++;

    if (current_offset_count >= CURRENT_OFFSET_SAMPLES)
    {
        iu_offset_adc =
            (uint16_t)
            (iu_offset_sum /
             CURRENT_OFFSET_SAMPLES);

        iv_offset_adc =
            (uint16_t)
            (iv_offset_sum /
             CURRENT_OFFSET_SAMPLES);

        current_offset_done = 1;

        FOC_ResetControl();
        FOC_SetNeutralPWM();

        foc_state = FOC_STATE_READY;
    }

    return;
}
```

流程：

```text
CALIBRATION
↓
采1000次
↓
计算Iu/Iv Offset
↓
READY
```

---

# 二十九、INIT 和 FAULT 的 ISR 截断

在 Offset 处理之后：

```c
if ((foc_state == FOC_STATE_INIT) ||
    (foc_state == FOC_STATE_FAULT))
{
    return;
}
```

含义：

```text
INIT
FAULT
```

都不允许继续执行真正的电流控制。

---

# 三十、READY 为什么继续算到 Park

READY 状态仍然执行：

```text
Current Conversion
↓
Clarke
↓
Park
```

这样可以继续观察：

```text
Iu / Iv
Iα / Iβ
Id / Iq
```

方便系统准备阶段和 Debug。

因此不能在 Park 之前直接判断：

```c
if (foc_state != FOC_STATE_RUN)
{
    return;
}
```

否则 READY 连电流坐标变换都不会执行。

---

# 三十一、READY 在 Park 后停止

Park 完成以后：

```c
if (foc_state == FOC_STATE_READY)
{
    return;
}
```

因此：

```text
READY：

Current      √
Clarke       √
Park         √

PI           ×
InvPark      ×
SVPWM更新     ×
```

---

# 三十二、只有 RUN 才进入 PI

在 PI 前再次检查：

```c
if (foc_state != FOC_STATE_RUN)
{
    return;
}
```

之后才运行：

```c
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
```

因此：

```text
RUN
↓
Current PI
↓
InvPark
↓
Voltage Limit
↓
SVPWM
↓
CCR
```

---

# 三十三、不同状态允许执行到哪里

| 状态        | Offset | Current | Clarke | Park |   PI | InvPark | SVPWM |
| ----------- | -----: | ------: | -----: | ---: | ---: | ------: | ----: |
| INIT        |      × |       × |      × |    × |    × |       × |     × |
| CALIBRATION |      √ |       × |      × |    × |    × |       × |     × |
| READY       |      × |       √ |      √ |    √ |    × |       × |     × |
| RUN         |      × |       √ |      √ |    √ |    √ |       √ |     √ |
| FAULT       |      × |       × |      × |    × |    × |       × |     × |

这是当前第一版 FOC 状态管理最核心的表格。

---

# 三十四、FOC 初始化顺序

状态管理建立以后，程序初始化顺序应该逐渐规范成：

```text
MCU / HAL Init
↓
Clock
↓
GPIO
↓
DMA
↓
ADC
↓
OPAMP
↓
TIM
↓
UART
↓
PI Init
↓
Control Variables Reset
↓
FOC State = CALIBRATION
↓
启动ADC
↓
最后启动TIM1
```

核心原则：

> 在会产生高速 ADC Trigger / ISR 之前，控制器内部变量必须已经初始化完成。

---

# 三十五、当前状态转换关系

第一版状态机：

```text
                Power On
                   ↓
                 INIT
                   ↓
          Software Initialization
                   ↓
             CALIBRATION
                   ↓
           Current Offset完成
                   ↓
                 READY
               ↙       ↖
        Start Request   Stop Request
             ↓               ↑
                    RUN
                     │
                     │ Fault
                     ↓
                   FAULT
```

---

# 三十六、Start Request

当前暂时不接按钮或 UART 命令。

可以通过 Debugger 修改：

```c
foc_start_request = 1;
```

如果当前状态为：

```text
READY
```

则：

```text
READY
↓
FOC_ResetControl()
↓
RUN
```

---

# 三十七、Stop Request

Debug 时可以：

```c
foc_stop_request = 1;
```

如果当前：

```text
RUN
```

则：

```text
RUN
↓
PI Reset
↓
Voltage Command Reset
↓
Neutral PWM
↓
READY
```

---

# 三十八、当前为什么还不能把 RUN 当作真正实机运行

目前仍然使用：

```text
theta_e_test
```

作为人工电角度。

因此当前：

```text
RUN
```

首先用于验证：

```text
状态切换逻辑
PI是否只在RUN工作
Stop后PI是否Reset
SVPWM是否只在RUN更新
```

还不能代表真正的 PMSM 闭环控制。

后续必须逐渐把：

```text
theta_e_test
```

替换为：

```text
真实机械角
↓
极对数
↓
真实电角度
```

---

# 三十九、当前高速控制链

目前整个高速链已经发展为：

```text
TIM1
↓
PWM同步ADC Trigger
↓
ADC1 / ADC2 Injected
↓
Iu / Iv
↓
Current Offset
↓
Current Conversion
↓
Clarke
↓
Park
↓
Current PI
↓
InvPark
↓
Vα / Vβ
↓
真实Vdc
↓
Voltage Vector Limit
↓
Min-Max SVPWM
↓
Duty
↓
CCR1 / CCR2 / CCR3
↓
TIM1
```

同时外部增加：

```text
FOC State Manager
```

决定：

> 这条高速链在当前状态下允许执行到哪一步。

---

# 四十、这一阶段最重要的认识

- 固定 `24V` 只能用于 SVPWM 初期测试。
- 真正 SVPWM 应使用 ADC 测得的实时母线电压。
- Vdc 不仅用于 Duty，还用于电压矢量限幅和未来保护。
- Regular ADC + DMA 适合母线电压这类相对慢变量。
- Injected ADC + PWM Trigger 适合高速同步电流采样。
- 电机控制程序不能在上电后无条件直接进入 PI。
- FOC 应该具有明确运行状态。
- CALIBRATION 只负责 Offset。
- READY 可以采样和观察电流，但不能运行 PI。
- RUN 才允许完整执行 Current PI 和 SVPWM。
- FAULT 应阻止正常 FOC 继续运行。
- Start / Stop / Fault 最好通过 Request 进入统一状态管理。
- 50%/50%/50% PWM 只是零线电压状态，不等于真正功率级关断。
- 高速控制放在同步 ISR，低速状态管理放在主循环。
- 状态判断应该插在控制流水线的不同位置，而不是全部堆在 ISR 开头。

---

# 四十一、当前学习进度

上一阶段：

- [x] Clarke
- [x] Park
- [x] Inverse Park
- [x] Current PI
- [x] PI Anti-Windup 基本思想
- [x] dq 解耦原理
- [x] SVPWM 基本原理
- [x] Min-Max SVPWM
- [x] Voltage Vector Limit
- [x] Duty → CCR
- [x] PWM Preload

本阶段新增：

- [x] ADC Regular + DMA 获取真实 Vdc
- [x] ADC Raw → Vdc 换算
- [x] 万用表与 Debugger 交叉验证 Vdc
- [x] SVPWM 使用真实 Vdc
- [x] FOC 状态机基本概念
- [x] INIT / CALIBRATION / READY / RUN / FAULT
- [x] Start / Stop / Fault Request 思想
- [x] FOC Reset Control
- [x] Neutral PWM 思想
- [x] State Manager 基本结构
- [x] 状态机与高速 ADC ISR 的分工
- [x] 不同状态对控制流水线的执行权限

当前正在完成：

```text
第一版FOC状态管理的完整代码验证
```

后续继续：

```text
FAULT条件
↓
Vdc欠压 / 过压保护
↓
真正PWM Disable / Fault处理
↓
真实转子位置
↓
机械角 → 电角度
↓
替换theta_e_test
↓
真正FOC闭环
```
