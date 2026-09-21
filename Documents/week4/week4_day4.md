# STM32 学习笔记：FOC 输出管理、Fault 保护与启动条件

## 一、当前阶段位置

上一阶段已经完成：

```text
真实 Vdc
↓
Voltage Limit
↓
SVPWM
↓
FOC State Machine
↓
INIT / CALIBRATION / READY / RUN / FAULT
```

这一阶段继续完善：

```text
FOC状态管理
↓
PWM输出管理
↓
FAULT紧急关闭
↓
FAULT锁存与复位
↓
Vdc欠压 / 过压保护
↓
Start条件检查
```

完成本阶段以后，控制程序已经不再只是“算法可以运行”，而开始具有：

```text
允许启动
正常停止
故障关闭
故障记录
故障复位
重新校准
```

等基本运行逻辑。

---

# 二、为什么 Neutral PWM 不等于真正关闭输出

之前 READY / STOP 状态会使用：

```text
Duty U = 50%
Duty V = 50%
Duty W = 50%
```

即：

$$
V_u=V_v=V_w
$$

所以理论线电压：

$$
V_{uv}=V_{vw}=V_{wu}=0
$$

但是：

> 三相功率器件仍然可能在进行 PWM 开关。

因此：

```text
50% / 50% / 50%
```

只能理解为：

> 零线电压状态。

不能理解为：

> 功率级完全关闭。

所以需要进一步控制 TIM1 的实际输出通道。

---

# 三、TIM1 同时承担两个任务

当前 TIM1 不只负责驱动逆变器。

它同时承担：

```text
CH1 / CH2 / CH3
+
CH1N / CH2N / CH3N
↓
三相功率 PWM
```

以及：

```text
CH4
↓
ADC Injected Trigger
↓
Iu / Iv同步采样
```

因此 TIM1 不能简单整体停止。

否则：

```text
PWM停止
```

同时也会造成：

```text
ADC同步采样停止
```

---

# 四、MOE 的作用

TIM1 是 Advanced Timer。

其中：

```text
MOE
=
Main Output Enable
```

是高级定时器输出体系的总开关。

可以简单理解为：

```text
TIM1内部计数
↓
CCR比较
↓
各通道
↓
MOE
↓
外部输出
```

---

# 五、一个重要的实际结论

在当前 STM32G431 + TIM1_CH4 → ADC Injected Trigger 配置下：

```text
MOE = 0
```

虽然：

```text
TIM1 CEN = 1
```

定时器仍然在计数，

但是实际测试发现：

```text
CH4 → ADC Injected Trigger
```

也停止工作。

表现为：

```text
adc_injected_count = 0
current_offset_count = 0
```

因此：

> READY / Normal Stop 状态不能直接通过清除 MOE 来关闭三相功率输出。

否则会同时破坏同步电流采样。

---

# 六、READY 状态的正确输出管理方式

READY 状态需要：

```text
三相功率输出关闭
```

但是仍然需要：

```text
TIM1运行
CH4运行
ADC同步采样运行
```

所以正确做法是：

```text
MOE = 1

CH1E / CH1NE = 0
CH2E / CH2NE = 0
CH3E / CH3NE = 0

CH4E = 1
```

也就是：

```text
关闭三相功率通道
+
保留ADC Trigger通道
```

---

# 七、正常 PWM Disable

因此 `FOC_PWM_Disable()` 不再清除 MOE。

而是只关闭三相桥对应的 6 个输出：

```c
static void FOC_PWM_Disable(void)
{
    CLEAR_BIT(TIM1->CCER,
              TIM_CCER_CC1E  |
              TIM_CCER_CC1NE |
              TIM_CCER_CC2E  |
              TIM_CCER_CC2NE |
              TIM_CCER_CC3E  |
              TIM_CCER_CC3NE);
}
```

注意：

```text
CC4E
```

没有被清除。

因此：

```text
CH1~3 + Complementary
→ OFF

CH4
→ ON
```

---

# 八、正常 PWM Enable

对应：

```c
static void FOC_PWM_Enable(void)
{
    __HAL_TIM_MOE_ENABLE(&htim1);

    SET_BIT(TIM1->CCER,
            TIM_CCER_CC1E  |
            TIM_CCER_CC1NE |
            TIM_CCER_CC2E  |
            TIM_CCER_CC2NE |
            TIM_CCER_CC3E  |
            TIM_CCER_CC3NE);
}
```

其含义：

```text
MOE = 1
↓
允许高级定时器输出

CC1~3 / CC1N~3N = 1
↓
真正允许三相桥PWM
```

当前由于真实电角度还没有建立：

```c
// FOC_PWM_Enable();
```

仍然保持不启用。

---

# 九、READY 状态当前的真实含义

现在 READY 可以定义为：

```text
Current Sampling      √
Clarke                √
Park                  √

Current PI            ×
SVPWM Power Output    ×

TIM1 Counter          √
CH4 ADC Trigger       √

CH1~3 Power PWM       ×
```

所以 READY 已经真正成为：

> 系统已准备，可以继续测量和监控，但还没有驱动电机。

---

# 十、正常 Stop 和 Fault 必须区别处理

正常停止：

```text
RUN
↓
Stop Request
↓
关闭CH1~CH3功率通道
↓
保留CH4
↓
READY
```

因此使用：

```c
FOC_PWM_Disable();
```

而真正 Fault：

```text
发生危险
↓
安全优先
↓
直接关闭整个高级定时器输出体系
```

此时可以允许 ADC 同步采样暂时停止。

---

# 十一、Emergency Disable

增加：

```c
static void FOC_PWM_EmergencyDisable(void)
{
    __HAL_TIM_MOE_DISABLE_UNCONDITIONALLY(&htim1);
}
```

该函数用于：

```text
FAULT
```

而不是正常 READY / STOP。

区别为：

```text
FOC_PWM_Disable()
→ 正常停止
→ 只关闭三相功率通道
→ CH4继续运行
```

```text
FOC_PWM_EmergencyDisable()
→ 紧急故障关闭
→ MOE = 0
→ 所有输出体系停止
```

---

# 十二、Fault Code

增加故障类型：

```c
typedef enum
{
    FOC_FAULT_NONE = 0,

    FOC_FAULT_VDC_UNDERVOLTAGE,

    FOC_FAULT_VDC_OVERVOLTAGE

} FOC_Fault_t;
```

以及：

```c
volatile FOC_Fault_t foc_fault_code =
    FOC_FAULT_NONE;
```

作用：

```text
foc_state
→ 当前系统是不是FAULT

foc_fault_code
→ 为什么进入FAULT
```

例如：

```text
FOC_STATE_FAULT

+

FOC_FAULT_VDC_OVERVOLTAGE
```

表示：

> 系统当前处于 Fault，并且原因是母线过压。

---

# 十三、FOC_TriggerFault()

建立统一 Fault 入口：

```c
static void FOC_TriggerFault(FOC_Fault_t fault_code)
{
    if ((foc_state != FOC_STATE_FAULT) &&
        (foc_fault_request == 0))
    {
        foc_fault_code = fault_code;

        FOC_PWM_EmergencyDisable();

        foc_fault_request = 1;
    }
}
```

逻辑：

```text
检测到异常
↓
记录Fault Code
↓
立即Emergency Disable
↓
提出Fault Request
↓
State Manager
↓
进入FAULT
```

---

# 十四、为什么 Fault 时先关 PWM

不应该：

```text
检测到Fault
↓
等待while(1)
↓
State Manager
↓
才关闭PWM
```

更合理的是：

```text
检测到Fault
↓
立即切断输出
↓
再由State Manager整理软件状态
```

所以：

```c
FOC_TriggerFault()
```

内部先执行：

```c
FOC_PWM_EmergencyDisable();
```

---

# 十五、Fault 锁存

Fault 一旦发生：

```text
FAULT条件消失
```

不能自动：

```text
FAULT → READY / RUN
```

否则可能造成：

```text
启动
↓
Fault
↓
自动恢复
↓
再次启动
↓
再次Fault
```

所以 FAULT 必须：

> Latch（锁存）。

只有明确收到：

```text
Fault Reset Request
```

才允许恢复。

---

# 十六、Fault Reset Request

增加：

```c
volatile uint8_t foc_fault_reset_request = 0;
```

作用：

```text
FAULT
↓
收到明确Reset
↓
重新建立系统正常状态
```

---

# 十七、为什么 Fault Reset 后重新 Calibration

Fault Reset 后没有直接：

```text
FAULT → READY
```

而是：

```text
FAULT
↓
CALIBRATION
↓
重新采集Current Offset
↓
READY
```

原因：

> Fault 后系统运行状态可能发生变化，重新进行一次电流零点校准更稳妥。

因此：

$$
\boxed{
FAULT
\rightarrow
CALIBRATION
\rightarrow
READY
}
$$

---

# 十八、Fault Reset 时恢复 ADC Trigger

FAULT 中：

```text
MOE = 0
```

导致当前 CH4 ADC Trigger 也停止。

但 CALIBRATION 必须依靠：

```text
CH4
↓
ADC Injected
↓
Current Offset
```

所以 Fault Reset 时需要：

```text
CH1~3保持Disable
```

同时：

```text
MOE重新Enable
```

让：

```text
CH4重新运行
```

从而恢复：

```text
ADC同步采样
```

---

# 十九、Fault Reset 基本流程

```text
FAULT
↓
Fault Reset Request
↓
Current Offset状态清零
↓
PI Reset
↓
三相功率通道保持Disable
↓
MOE重新Enable
↓
CH4恢复ADC Trigger
↓
Fault Code清零
↓
CALIBRATION
↓
Offset重新0→1000
↓
READY
```

---

# 二十、Vdc 保护

真实母线电压已经可以通过：

```text
ADC1 Regular
↓
DMA
↓
vdc_adc_raw
↓
vdc_bus
```

获得。

因此建立：

```text
Vdc
↓
Protection Logic
↓
FOC_TriggerFault()
```

---

# 二十一、当前测试用 Vdc 阈值

由于目前实际测试母线约：

$$
V_{dc}\approx2V
$$

为了验证软件逻辑，使用临时测试变量：

```c
volatile float vdc_uv_trip = 0.5f;
volatile float vdc_ov_trip = 5.0f;
```

即：

```text
0.5V < 2V < 5V
```

正常不会触发保护。

需要特别注意：

> 0.5V 和 5V 只是当前软件功能测试参数。

它们不是以后真实电机系统的正式保护阈值。

正式参数需要根据：

```text
实际母线额定电压
MOSFET耐压
Gate Driver
电源范围
电机工作条件
```

重新确定。

---

# 二十二、Fault 去抖 / 确认时间

ADC 可能产生瞬时噪声。

所以不能：

```text
只出现一次异常ADC
↓
立即FAULT
```

增加：

```c
#define VDC_FAULT_CONFIRM_SAMPLES 100U
```

以及：

```c
volatile uint16_t vdc_uv_counter = 0;
volatile uint16_t vdc_ov_counter = 0;
```

当前控制周期：

$$
T_s=50\mu s
$$

即约：

$$
20kHz
$$

连续 100 次：

$$
100\times50\mu s
=
5ms
$$

因此：

> 电压异常持续约 5 ms 才确认 Fault。

---

# 二十三、过压保护

READY 和 RUN 都检测过压。

原因：

> 即使电机没有启动，母线电压过高本身也是异常状态。

逻辑：

```text
Vdc > Over Voltage Threshold
↓
OV Counter++
↓
连续100次
↓
FOC_FAULT_VDC_OVERVOLTAGE
↓
FAULT
```

---

# 二十四、欠压保护

欠压当前只在：

```text
RUN
```

状态检测。

因为：

```text
READY + Vdc≈0
```

可能只是：

> 功率电源还没有接通。

这并不一定是 Fault。

但是：

```text
RUN + Vdc过低
```

意味着：

> 系统正在运行过程中失去正常母线供电。

此时才定义为：

```text
Under-voltage Fault
```

---

# 二十五、READY 欠压与 RUN 欠压的区别

```text
READY
+
Vdc不足
↓
不允许启动
```

但：

```text
不一定进入FAULT
```

而：

```text
RUN
+
Vdc不足
↓
Under-voltage Fault
```

这是两个不同概念：

```text
Start Condition
```

和：

```text
Runtime Protection
```

不能混为一谈。

---

# 二十六、Vdc Protection 调用位置

两相同步 ADC 数据到齐后：

```c
vdc_bus =
    (float)vdc_adc_raw
    * VDC_VOLTS_PER_COUNT;

adc_injected_count++;

FOC_CheckVdcProtection();

if (foc_fault_request)
{
    return;
}
```

如果本次 ISR 检测到 Fault：

```text
Emergency Disable
↓
本次ISR立即return
```

不再继续：

```text
Current PI
InvPark
SVPWM
CCR Update
```

---

# 二十七、Over Voltage 验证

正常：

```text
Vdc ≈ 2V
Vdc_OV = 5V
```

因此：

```text
OV Counter = 0
State = READY
```

通过 Debugger 将：

```text
vdc_ov_trip
```

改为：

```text
1V
```

此时：

$$
2V>1V
$$

因此：

```text
OV Counter
↓
100
↓
FOC_FAULT_VDC_OVERVOLTAGE
↓
FAULT
↓
MOE = 0
```

该实验已经验证成功。

---

# 二十八、Under Voltage 验证

欠压只在 RUN 检测。

正常：

```text
Vdc ≈ 2V
UV Threshold = 0.5V
```

不会 Fault。

软件进入 RUN 后，将：

```text
vdc_uv_trip
```

临时改为：

```text
3V
```

此时：

$$
2V<3V
$$

因此：

```text
UV Counter
↓
100
↓
FOC_FAULT_VDC_UNDERVOLTAGE
↓
FAULT
```

该实验已经验证成功。

---

# 二十九、Start Condition

以前：

```text
READY
↓
Start Request
↓
直接RUN
```

不够合理。

现在改成：

```text
READY
↓
Start Request
↓
检查启动条件
↓
满足
↓
RUN
```

---

# 三十、FOC_CanStart()

建立：

```c
static uint8_t FOC_CanStart(void)
{
    if (current_offset_done == 0)
    {
        return 0;
    }

    if ((foc_fault_request != 0) ||
        (foc_fault_code != FOC_FAULT_NONE))
    {
        return 0;
    }

    if (vdc_bus <= vdc_uv_trip)
    {
        return 0;
    }

    if (vdc_bus >= vdc_ov_trip)
    {
        return 0;
    }

    return 1;
}
```

它回答：

> 当前系统是否真正具备进入 RUN 的条件？

---

# 三十一、当前 Start 条件

现在包括：

```text
Current Offset完成
+
没有Fault
+
Vdc高于欠压启动阈值
+
Vdc低于过压阈值
```

即：

$$
V_{UV}<V_{dc}<V_{OV}
$$

同时：

```text
current_offset_done = 1
```

以及：

```text
foc_fault_code = NONE
```

---

# 三十二、Start Request 新流程

现在：

```text
READY
↓
foc_start_request
↓
FOC_CanStart()
     │
 ┌───┴────┐
 ↓        ↓
false    true
 ↓        ↓
READY    RUN
```

如果条件不足：

> Start Request 被拒绝，但系统可以继续停留在 READY。

---

# 三十三、Start Condition 验证

正常状态：

```text
Vdc ≈ 2V
UV = 0.5V
OV = 5V
Offset Done = 1
No Fault
```

所以：

```text
Start Request
↓
RUN
```

验证成功。

人为将：

```text
UV Threshold = 3V
```

此时：

$$
V_{dc}<V_{UV}
$$

然后发送 Start Request：

```text
READY
↓
Start被拒绝
↓
仍然READY
```

并且：

```text
不会进入FAULT
```

该实验已经验证成功。

---

# 三十四、当前完整状态机

目前：

```text
                   Power On
                      ↓
                CALIBRATION
                      ↓
                  Offset Done
                      ↓
                    READY
                      ↓
                Start Request
                      ↓
                FOC_CanStart()
                  ↙         ↘
              false          true
                ↓              ↓
              READY           RUN
                               │
                     ┌─────────┴─────────┐
                     ↓                   ↓
                   Stop                Fault
                     ↓                   ↓
                   READY               FAULT
                                         ↓
                                   Fault Reset
                                         ↓
                                   CALIBRATION
                                         ↓
                                       READY
```

---

# 三十五、不同状态下的硬件行为

## CALIBRATION

```text
TIM1 Counter       ON
MOE                ON
CH1~3 Power PWM    OFF
CH4                ON
ADC Sync           ON
Current Offset     ON
```

---

## READY

```text
TIM1 Counter       ON
MOE                ON
CH1~3 Power PWM    OFF
CH4                ON
ADC Sync           ON
Clarke / Park      ON
PI                  OFF
```

---

## RUN

当前仍为软件测试 RUN：

```text
FOC Algorithm       ON
PI                  ON
SVPWM               ON

真实三相PWM输出      仍暂未Enable
```

因为：

```text
真实电角度尚未接入
```

---

## FAULT

```text
Emergency Disable
↓
MOE = 0
↓
三相输出关闭
↓
CH4同步ADC暂时停止
↓
Fault锁存
```

---

# 三十六、为什么真实 PWM 仍然没有 Enable

目前系统中：

```text
Iu / Iv
→ 真实

Vdc
→ 真实

ADC Trigger
→ 真实

Current PI
→ 已建立

SVPWM
→ 已建立

Fault / Start / Stop
→ 已建立
```

但是：

```text
theta_e_test
```

仍然是人工电角度。

所以当前不能真正执行：

```c
FOC_PWM_Enable();
```

否则：

> 软件磁场方向和真实转子方向没有建立正确关系。

可能造成：

```text
大电流
抖动
堵转
反转
PI饱和
```

---

# 三十七、当前剩余的最大“假量”

目前最关键的临时变量已经变成：

```text
theta_e_test
```

下一阶段需要建立：

```text
Position Sensor
↓
Mechanical Angle
↓
θm
↓
Pole Pairs
↓
Electrical Angle
↓
θe
```

最终替换：

```text
theta_e_test
```

---

# 三十八、本阶段最重要的认识

- Neutral PWM 不等于真正关闭功率输出。
- TIM1 同时承担三相 PWM 和 ADC Trigger。
- 当前配置下，直接清除 MOE 会导致 CH4 ADC Trigger 停止。
- READY / Normal Stop 应只关闭 CH1~CH3 及互补输出。
- CH4 必须继续保持运行。
- FAULT 可以使用 Emergency Disable，直接清除 MOE。
- Fault 应记录具体原因。
- Fault 应锁存，而不是条件消失后自动恢复。
- Fault Reset 后应重新执行 Current Offset Calibration。
- 运行中 Protection 与启动前 Start Condition 是两个不同概念。
- READY 时母线不足可以拒绝启动，而不一定直接 Fault。
- RUN 中发生欠压才属于运行故障。
- Vdc Protection 应进行连续采样确认，避免单次噪声误触发。
- 所有启动条件应该集中在 `FOC_CanStart()` 中管理。
- 当前正式三相输出仍不能 Enable，因为真实转子位置尚未建立。

---

# 三十九、当前学习进度

已完成：

- [x] 三相互补 PWM
- [x] Dead Time
- [x] PWM Preload
- [x] PWM同步ADC
- [x] Iu / Iv 电流采样
- [x] Current Offset Calibration
- [x] Clarke
- [x] Park
- [x] Current PI
- [x] Inverse Park
- [x] SVPWM
- [x] Voltage Vector Limit
- [x] 真实 Vdc
- [x] FOC State Machine
- [x] Normal PWM Disable
- [x] Emergency PWM Disable
- [x] READY状态保留CH4采样
- [x] Fault Code
- [x] Fault Latch
- [x] Fault Reset
- [x] Fault Reset后重新Calibration
- [x] Vdc Over-voltage Protection
- [x] Vdc Under-voltage Protection
- [x] Fault Confirm Counter
- [x] Start Condition
- [x] FOC_CanStart()

当前状态：

```text
底层硬件
+
FOC电流控制主体
+
SVPWM
+
状态管理
+
基础保护
```

已经基本形成。

下一阶段：

```text
真实转子位置传感器
↓
Encoder / Hall硬件识别
↓
TIM位置接口
↓
机械角 θm
↓
极对数
↓
电角度 θe
↓
编码器零位偏置
↓
替换 theta_e_test
```

这将是正式进入真实电机之前的关键阶段。
