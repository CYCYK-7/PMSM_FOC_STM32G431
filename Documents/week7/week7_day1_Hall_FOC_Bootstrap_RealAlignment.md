# STM32G431 PMSM FOC 学习笔记
# Hall 电角度接入、Bootstrap 启动、按键控制及首次真实励磁

## 一、本阶段学习目标与成果

### 1.1 本阶段起点

上一次 Git 提交时，已经完成：

- TIM1 三相互补 PWM 与 ADC 同步采样
- 电流零偏校准和三相电流重构
- Clarke / Park / InvPark 坐标变换
- PI 电流控制器
- Min-Max SVPWM
- FOC 状态管理与基本故障保护
- Hall 状态检测、方向判断、速度估算、连续电角度估算
- Rotor Alignment 不通电软件验证

但此前还没有真正让自己编写的控制程序驱动电机。

### 1.2 本阶段完成内容

本阶段的核心目标，是从纯软件测试向真实 PMSM 控制过渡。

具体完成了：

1. Hall Offset 的数学计算及角度回绕验证
2. Hall 电角度接入 FOC 控制链
3. Hall 角度失效检测
4. PWM 上电启动顺序调整
5. Bootstrap 自举充电状态机
6. 参考 MCSDK 例程与开发板用户手册
7. Rotor Alignment 真实励磁状态机
8. SW3 板载按键长按启动
9. 电机极对数重新验证
10. 首次真实固定电压矢量励磁实验

**本阶段最重要的成果：**

使用自己编写的 STM32G431 程序，成功向电机施加固定方向的电压矢量，并观察到了真实的机械抖动响应。

这标志着项目已经从 FOC 软件算法验证，正式进入实际电机控制调试阶段。

---

## 二、Hall Offset 软件验证与 FOC 角度接入

### 2.1 为什么 Hall 电角度需要 Offset？

Hall 传感器提供的是转子相对于 Hall 安装位置的角度信息。

而 FOC 使用的电角度，必须与程序定义的定子 αβ 坐标系一致。

因此：

$$
\theta_e=\operatorname{Wrap}(\theta_{\mathrm{Hall}}+\theta_{\mathrm{offset}})
$$

其中：

- $\theta_{\mathrm{Hall}}$：Hall 估算的相对电角度
- $\theta_{\mathrm{offset}}$：Hall 与 FOC 定子坐标系之间的角度偏移
- $\theta_e$：FOC 实际使用的电角度

需要注意，公式采用加法和当前角度方向定义。若实际 Hall 方向与 FOC 正方向相反，还需要调整方向关系。

### 2.2 软件验证方法

由于真实 Offset 尚未标定，首先人为设置：

```c
volatile float hall_offset_test_rad = PI_F / 3.0f;
```

即：

$$
\theta_{\mathrm{offset,test}}=60^\circ
$$

然后在 ADC 注入中断中执行：

```c
Hall_UpdateElectricalAngle();

if (hall_angle_valid)
{
    hall_theta_corrected_test =
        WrapAngle0To2Pi(
            hall_theta_rel + hall_offset_test_rad
        );

    theta_e_foc = hall_theta_corrected_test;
}
```

通过串口连续观察：

```text
Hall
Corrected
FOC
Valid
PWM
```

实际验证：

- Hall 角度能够随手动旋转更新
- Corrected = Hall + Offset，再经过角度归一化
- Corrected 和 FOC 一致
- 角度能够正确跨越 2π
- PWM 保持关闭

### 2.3 Park / InvPark 接口验证

为了确认 Hall 修正角度能够参与坐标变换，构造：

$$
i_\alpha=1,\qquad i_\beta=0
$$

经过 Park：

$$
i_d=\cos\theta_e
$$

$$
i_q=-\sin\theta_e
$$

再经过 InvPark，应恢复：

$$
i_\alpha'=1,\qquad i_\beta'=0
$$

串口结果符合预期。

这说明 Hall 修正角度已经能够正确传递给坐标变换函数。

但必须区分：

**数学接口验证成功，不代表真实 Hall Offset 已经标定准确。**

目前的 $60^\circ$ 仍然是测试值。

### 2.4 Hall 失效保护

新增故障类型：

```c
FOC_FAULT_HALL_INVALID
```

在 ADC 控制回调中：

```c
Hall_UpdateElectricalAngle();

/* 更新修正角度 */

/* 检查 Hall 角度有效性 */
if ((foc_state == FOC_STATE_RUN) &&
    (hall_angle_valid == 0U))
{
    FOC_TriggerFault(FOC_FAULT_HALL_INVALID);
    return;
}
```

核心思想：

在依赖 Hall 角度的 FOC 控制运行中，如果角度无效，就不能继续使用错误的角度执行 Park、PI 和 SVPWM。

**工程知识点：传感器有效性检查应在使用传感器数据进行控制计算之前完成。**

---

## 三、PWM 上电启动顺序调整

### 3.1 原来存在的问题

此前程序初始化时，先启动：

```c
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
```

其他两相同理。

然后才调用：

```c
FOC_PWM_Disable();
```

这种顺序可能使三相 PWM 在初始化过程中出现短暂输出。

### 3.2 修改后的思路

上电时，只启动 TIM1 CH4，用于 ADC 同步触发。

三相功率 PWM 保持关闭：

```c
FOC_PWM_Disable();

if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK)
{
    Error_Handler();
}

FOC_PWM_Disable();
```

验证结果：

```text
foc_state = FOC_STATE_READY

current_offset_done = 1

adc_injected_count 持续增加

TIM1->CCER & 0x0555U = 0
```

全部通过。

### 3.3 理解

电机控制程序应当遵循：

```text
MCU 上电
   ↓
初始化外设
   ↓
电流校准
   ↓
READY
   ↓
等待明确启动命令
   ↓
启动电机
```

而不是 MCU 初始化完成后自动启动电机。

这个设计原则对于后续实现启动、停止、故障恢复都非常重要。

---

## 四、Bootstrap 自举充电

### 4.1 为什么需要 Bootstrap？

开发板使用 FD6288T 三相 MOSFET 栅极驱动器。

高侧 MOSFET 的栅极驱动需要相对于其源极建立足够的驱动电压。

自举电容是常用的高侧驱动供电方式。

在高侧需要导通之前，通常需要先为自举电容充电。

充电时，可以让低侧 MOSFET 导通，使相节点拉到较低电位，建立自举电容充电通路。

注意：

**Bootstrap 不等于单纯执行 HAL_Delay(10)。**

必须结合实际驱动电路，建立充电所需的开关状态。

### 4.2 本项目状态机

新增：

```c
FOC_STATE_BOOTSTRAP_PREPARE

FOC_STATE_BOOTSTRAP_CHARGE
```

采用：

```c
#define BOOTSTRAP_PREPARE_MS  2U
#define BOOTSTRAP_CHARGE_MS   10U
```

执行过程：

```text
READY
  ↓
BOOTSTRAP_PREPARE
  ↓
关闭三相输出
  ↓
CCR1 = CCR2 = CCR3 = 0
  ↓
等待比较值更新
  ↓
BOOTSTRAP_CHARGE
  ↓
使能三个互补输出 CH1N/CH2N/CH3N
  ↓
持续约 10 ms
  ↓
关闭输出
  ↓
恢复中性 PWM
  ↓
READY
```

### 4.3 为什么只使能互补通道？

TIM1 三相输出：

```text
CH1  / CH1N
CH2  / CH2N
CH3  / CH3N
```

在当前 PWM1 和输出极性配置下，CCR=0 配合互补输出使能，用于请求三个下桥臂导通。

对应寄存器使能掩码：

```c
TIM_CCER_CC1NE |
TIM_CCER_CC2NE |
TIM_CCER_CC3NE
```

十六进制：

```text
0x0444
```

十进制：

```text
1092
```

### 4.4 验证结果

实际运行后：

```text
Bootstrap 完成标志 = 1

Bootstrap 持续时间 ≈ 10 ms

bootstrap_ccer_snapshot = 1092

结束后 TIM1->CCER & 0x0555U = 0
```

状态机按预期完成。

需要注意，这些结果验证的是控制流程和 MCU 寄存器状态，不等同于直接测量了自举电容电压。

### 4.5 从 MCSDK 例程学到什么？

参考厂家 MCSDK 示例，启动流程通常包括：

```text
电流 Offset 校准
    ↓
Bootstrap 自举电容充电
    ↓
FOC 控制状态初始化
    ↓
PWM 使能
    ↓
电机运行
```

由此认识到：

电机控制软件不仅包括 PI、Park 和 SVPWM 算法，还必须正确管理功率驱动器的启动时序。

---

## 五、实际电机参数与硬件信息确认

### 5.1 当前实际使用的电机

电机铭牌：

```text
LA034-040NN07A
DC105V
25.1W
```

三相绕组线间电阻：

| 测量组合 | 结果    |
| -------- | ------- |
| 红—白    | 约 15 Ω |
| 白—蓝    | 约 15 Ω |
| 蓝—红    | 约 15 Ω |

三相直流电阻基本对称。

### 5.2 极对数重新确认

之前设置：

```c
#define MOTOR_POLE_PAIRS 2.0f
```

为了排除极对数测量错误，重新使用 Hall 边沿计数：

```c
hall_edge_count
```

手动转动电机机械一整圈，实际增加：

```text
12 次 Hall 状态跳变
```

一个完整电周期对应 6 次 Hall 跳变。

因此：

$$
p=\frac{12}{6}=2
$$

**最终确认电机极对数为 2。**

### 5.3 相线连接约定

目前采用：

```text
OUTU → 红

OUTV → 白

OUTW → 蓝
```

这是一套人为固定的三相命名约定，并非已经查明的厂家原始 UVW 颜色定义。

Hall 信号已经能够正确检测：

- 六个有效状态
- 正反转方向
- Hall 边沿
- Hall 相对电角度

之前采用：

```text
001 → Sector 0
```

作为 Hall 扇区编号的参考起点。

但 Hall 扇区编号本身不是物理零电角度。

后续仍需确定 Hall 与实际定子磁场的方向关系，以及真实 Offset。

### 5.4 为什么不能直接使用用户手册中的电机参数？

开发板用户手册配套电机标注为：

```text
极对数：4
相电阻：6.97 Ω
相电感：5.35 mH
```

而当前实际电机极对数已经实测为 2，线间电阻也与手册配套电机参数不同。

因此：

**开发板相同，并不代表电机参数相同。**

应采用当前实际电机的参数与标定结果。

---

## 六、真实 Rotor Alignment 状态机

### 6.1 为什么需要 Rotor Alignment？

FOC 的 Park 与 InvPark 需要转子的实际电角度。

但目前：

```c
hall_offset_test_rad = PI_F / 3.0f;
```

是人为设置的测试值。

为了建立实际定子磁场和转子位置之间的关系，需要进一步进行转子位置标定。

当前第一步采用固定方向电压矢量励磁：

$$
V_\alpha=V_{\mathrm{align}}
$$

$$
V_\beta=0
$$

从而建立固定方向的定子磁场。

### 6.2 新增状态

```c
FOC_STATE_ALIGN_PREPARE

FOC_STATE_ALIGN_ACTIVE
```

与之前的：

```c
FOC_STATE_ALIGN_TEST
```

严格区分。

二者区别：

| 状态          | 功能                           |
| ------------- | ------------------------------ |
| ALIGN_TEST    | 只执行软件计算，不使能功率输出 |
| ALIGN_PREPARE | 预置实际励磁的 PWM 比较值      |
| ALIGN_ACTIVE  | 实际向电机输出固定矢量         |

### 6.3 真实定位流程

```text
READY
  ↓
接收真实定位请求
  ↓
BOOTSTRAP_PREPARE
  ↓
BOOTSTRAP_CHARGE
  ↓
ALIGN_PREPARE
  ↓
固定 Vα / Vβ
  ↓
SVPWM 计算
  ↓
预置 CCR1/2/3
  ↓
ALIGN_ACTIVE
  ↓
使能三相功率 PWM
  ↓
到达定位时间或发生故障
  ↓
关闭 PWM
  ↓
恢复中性值
  ↓
READY
```

定位使用固定电压矢量，不让尚未整定完成的电流 PI 覆盖该电压指令。

### 6.4 固定电压矢量计算

相关函数：

```c
FOC_RealAlign_PrepareVector();
```

核心过程：

```c
float va = ALIGN_V_ALPHA_V;
float vb = 0.0f;

SVPWM_LimitVoltage(&va, &vb, vdc_bus);

SVPWM_Run(va, vb, vdc_bus, &out);
```

随后转换占空比并预置：

```c
TIM1->CCR1
TIM1->CCR2
TIM1->CCR3
```

真正使能功率 PWM 之前，先等待比较值更新。

### 6.5 定位完成标志的含义

```c
foc_real_align_done = 1;
```

意味着预定励磁流程正常完成。

但是：

**不意味着转子已经准确到达目标位置，也不意味着真实 Hall Offset 已经得到。**

这一点非常重要。

---

## 七、SW3 板载按键启动

### 7.1 为什么不继续使用 Debugger 启动？

最初通过 Debugger 修改：

```c
foc_real_align_request = 1;
```

这种方式适合初期开发，但不适合长期作为电机启动方式。

因为 STM32 的 Flash 会保存程序，上电后 MCU 会自动执行。

更合理的工程设计是：

```text
上电
 ↓
READY
 ↓
等待用户命令
 ↓
明确按键启动
 ↓
执行电机控制流程
```

### 7.2 当前按键硬件

开发板 SW3：

```text
MCU PC13
```

按键状态：

```text
松开：0

按下：1
```

使用 GPIO 输入方式读取：

```c
HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
```

### 7.3 实现的功能

新增：

```c
FOC_CheckStartButton();
```

在 `main()` 的 `while(1)` 中，放在：

```c
FOC_StateManager();
```

之前执行。

原因：

按键函数只产生控制请求，状态机负责判断条件并执行真正的启动流程。

使用：

```c
#define SW3_DEBOUNCE_MS   30U
#define SW3_LONG_PRESS_MS 500U
```

实现：

- 30 ms 消抖
- 长按约 500 ms 触发
- 一次长按只触发一次
- 必须松开后才允许再次触发
- READY 状态才接受启动请求
- 上电时持续按住不会自动启动

### 7.4 调试变量

```c
sw3_long_press_count

sw3_align_request_count
```

分别记录有效长按次数及定位请求次数。

### 7.5 验证结果

| 测试操作       | 实验结果     |
| -------------- | ------------ |
| 上电不按按键   | 不启动       |
| 短按           | 不触发请求   |
| 长按约 0.7 s   | 触发一次     |
| 持续按住       | 不重复触发   |
| 松开后重新长按 | 可以再次触发 |

全部通过。

### 7.6 本步骤的工程意义

这一步体现了软件分层思想：

```text
命令输入层
    ↓
启动请求
    ↓
状态管理层
    ↓
控制执行层
    ↓
PWM 功率输出
```

未来串口、CAN、上位机也可以产生启动请求，而不需要重新编写底层控制算法。

---

## 八、第一次真实电机励磁实验

### 8.1 实验目标

本次实验不要求电机连续旋转。

目标是验证：

1. 自己的 STM32 程序能够实际使能三相 PWM
2. SVPWM 能够产生固定方向电压矢量
3. 电机能够表现出真实机械响应
4. ADC 能够记录励磁期间的电流读数
5. 定位结束后 PWM 自动关闭

### 8.2 初始实验参数

```c
#define ALIGN_POWER_ALLOWED 1U

#define ALIGN_PREPARE_MS  2U

#define ALIGN_ACTIVE_MS   100U

#define ALIGN_V_ALPHA_V   0.5f
```

软件过流设置：

```c
#define CURRENT_OC_TRIP_A 0.25f

#define CURRENT_OC_CONFIRM_SAMPLES 2U
```

这里的 0.25 A 是软件判定阈值，不是独立硬件电流限制。

### 8.3 实验一：0.5 V、100 ms

实验条件：

```text
Vα = 0.5 V

Vβ = 0

时间 = 100 ms
```

结果：

```text
foc_state = READY

foc_real_align_done = 1

align_max_abs_phase_current = 0.1099 A

align_final_sector = 2

foc_fault_code = FOC_FAULT_NONE

TIM1->CCER & 0x0555U = 0
```

第一次观察时以为电机没有反应。

后续重新测试发现，实际上存在很轻微但肉眼可见的抖动。

因此，应该记录为：

**0.5 V、100 ms 已经能够使电机产生轻微机械响应。**

### 8.4 PWM 实际使能状态检查

为了确认定位期间 PWM 确实被 MCU 使能，新增：

```c
align_pwm_ccer_snapshot

align_pwm_moe_snapshot
```

在：

```c
FOC_PWM_Enable();
```

之后立即保存寄存器状态。

实验结果：

```text
align_pwm_ccer_snapshot = 1365

align_pwm_moe_snapshot = 1
```

其中：

```text
1365 = 0x0555
```

对应六路 TIM1 PWM 输出使能位。

说明 MCU 已执行预期的三相 PWM 使能操作。

结束之后：

```text
TIM1->CCER & 0x0555U = 0
```

说明 PWM 已恢复关闭状态。

注意：寄存器检查不能代替外部实际栅极波形测量。

### 8.5 实验二：1.0 V、100 ms

仅将：

```c
#define ALIGN_V_ALPHA_V 0.5f
```

修改为：

```c
#define ALIGN_V_ALPHA_V 1.0f
```

持续时间仍然为 100 ms。

实验结果：

```text
align_max_abs_phase_current = 0.17584 A

align_final_sector = 2

foc_real_align_done = 1

foc_fault_code = FOC_FAULT_NONE

align_pwm_ccer_snapshot = 1365

align_pwm_moe_snapshot = 1
```

机械现象：

相比 0.5 V，电机内部的轻微抖动更加容易感受到。

电流峰值读数随电压指令提高而增大。

### 8.6 实验三：1.0 V、250 ms

保持电压：

```c
#define ALIGN_V_ALPHA_V 1.0f
```

将时间调整为：

```c
#define ALIGN_ACTIVE_MS 250U
```

实验现象：

- 电机出现更加明显的轻微抖动
- 肉眼可以观察到类似左右晃动的微小动作
- 没有实现连续旋转

本次没有另外记录可确认的峰值电流数值，不应自行补充。

### 8.7 三组实验汇总

| 实验 | 电压矢量幅值 | 持续时间 | 最大相电流读数 | 机械响应           |
| ---- | ------------ | -------- | -------------- | ------------------ |
| A    | 0.5 V        | 100 ms   | 0.1099 A       | 极轻微抖动         |
| B    | 1.0 V        | 100 ms   | 0.17584 A      | 抖动更易察觉       |
| C    | 1.0 V        | 250 ms   | 未记录         | 肉眼可见的轻微晃动 |

这里最大相电流读数是软件记录的采样峰值，其中可能包含 ADC 噪声，不能直接理解为稳态励磁电流。

---

## 九、为什么固定磁场只让电机轻微晃动？

本次电压指令：

$$
V_\alpha=V_{\mathrm{align}}
$$

$$
V_\beta=0
$$

电压矢量方向保持固定。

因此：

```text
三相逆变器
    ↓
产生固定方向的定子磁场
    ↓
转子受到朝该磁场方向对齐的电磁转矩
    ↓
转子可能发生微小位移或晃动
```

但不会持续产生旋转磁场。

如果转子原本已经接近平衡位置，或者励磁较弱、机械摩擦较大，都可能只观察到轻微晃动。

因此：

**固定矢量励磁实验的目的不是让电机连续旋转。**

目前的实验能够支持：

- 控制程序已进入实际功率输出阶段
- 电机对固定电压矢量有机械响应
- PWM 状态能够按预定时间关闭
- 软件故障机制未在上述实验中触发

但暂时不能确认：

- 转子已经稳定对齐至目标电角度
- Hall Offset 已完成准确标定
- 三相电流环已经真正闭环
- 电机能够持续稳定旋转

---

## 十、本阶段学习到的关键工程知识

### 10.1 电机控制不只是数学算法

实际 FOC 项目必须综合考虑：

```text
外设初始化
    ↓
启动状态机
    ↓
功率驱动
    ↓
电流采样
    ↓
传感器反馈
    ↓
控制算法
    ↓
故障保护
```

此前学过的 Clarke/Park、PI、SVPWM 只是其中的一部分。

### 10.2 状态机是电机控制工程的重要基础

本阶段进一步建立：

```text
READY
   ├── ALIGN_TEST
   │
   ├── BOOTSTRAP_PREPARE
   │       ↓
   │   BOOTSTRAP_CHARGE
   │       ↓
   │   ALIGN_PREPARE
   │       ↓
   │   ALIGN_ACTIVE
   │       ↓
   │      READY
   │
   └── RUN（后续真实闭环运行）
```

故障时能够进入 FAULT。

需要重视请求标志、状态切换、启动条件和中途取消，避免启动任务残留。

### 10.3 软件计算完成，不代表物理过程完成

例如：

```c
foc_real_align_done = 1;
```

只是定位流程的完成标志。

不能把它直接解释成转子已经准确对齐。

实际电机控制必须结合：

- 电流反馈
- Hall 反馈
- 机械响应
- 故障状态

共同判断。

### 10.4 真实调试应当一次改变一个参数

本阶段采用：

```text
0.5 V / 100 ms
       ↓
1.0 V / 100 ms
       ↓
1.0 V / 250 ms
```

通过逐步调整定位电压和持续时间，观察电机机械响应的差异。

这比一次同时修改大量控制参数，更有利于定位问题。

### 10.5 对测试结论应保持准确

电机有轻微抖动：

不等于 FOC 已经成功闭环。

最大电流增加：

不等于电流采样已完成动态精度校准。

Hall 状态有效：

不等于 Hall Offset 已经准确标定。

工程开发中的实验结论必须与实际证据对应。

---

## 十一、当前项目进度

| 模块                    | 当前状态             |
| ----------------------- | -------------------- |
| STM32G431 外设初始化    | 已完成               |
| TIM1 三相 PWM           | 已完成软件配置       |
| ADC 同步电流采样        | 已完成               |
| 电流零偏校准            | 已完成               |
| Clarke / Park / InvPark | 已完成               |
| Min-Max SVPWM           | 已完成               |
| PI 电流控制器           | 已实现，实机尚未整定 |
| Hall 状态及速度估计     | 已验证               |
| Hall 电角度计算与方向   | 已验证               |
| Hall Offset 数学处理    | 已验证               |
| 真实 Hall Offset 标定   | 未完成               |
| FOC 状态管理            | 已建立               |
| Bootstrap 启动流程      | 已完成程序验证       |
| SW3 长按启动            | 已完成               |
| 真实固定矢量励磁        | 已产生机械响应       |
| 连续开环旋转            | 未完成               |
| 真实电流闭环 FOC        | 未完成               |
| 速度闭环 FOC            | 未完成               |

### 本阶段结论

**已完成：STM32G431 PMSM 首次真实固定电压矢量励磁实验。**

程序能够通过 SW3 按键发出启动请求，依次完成 Bootstrap、PWM 使能、固定电压矢量励磁与自动关闭，并观察到实际电机的轻微机械响应。

这是目前项目从软件算法向实机控制过渡的重要阶段成果。

---

## 十二、下一阶段：低速开环旋转

下一阶段的目标是：

**让电机不再只产生轻微定位晃动，而是在自己编写的程序控制下实现第一次连续旋转。**

核心思想：

此前定子电压矢量固定：

$$
\theta_s=0
$$

下一阶段让定子电压矢量随时间旋转：

$$
\theta_s(t)=\omega_e t
$$

得到：

$$
V_\alpha=V_{\mathrm{test}}\cos\theta_s
$$

$$
V_\beta=V_{\mathrm{test}}\sin\theta_s
$$

然后通过 SVPWM 产生三相电压。

下一阶段主要任务：

1. 增加独立的 OPEN_LOOP 状态。
2. 实现旋转电角度的周期更新。
3. 生成旋转电压矢量。
4. 保留电流监测、超时关断及故障处理。
5. 观察电机实际旋转方向。
6. 对照 Hall 方向和速度反馈。
7. 在实验稳定后，再考虑真实 Hall Offset 和闭环 FOC。

本阶段的固定矢量励磁程序应作为 Git 检查点保留。

---

# 十三、本阶段 Git 提交

## 13.1 为什么现在提交？

这次提交要保存一个重要的实机里程碑：

```text
上一次 Git
   ↓
Rotor Alignment 软件验证
   ↓
Hall 电角度接入 FOC
   ↓
Bootstrap 启动
   ↓
SW3 按键启动
   ↓
真实固定电压矢量励磁
   ↓
电机出现机械响应
   ↓
本次 Git
```

下一步的低速开环旋转涉及新的角度更新方式和功率输出逻辑。

如果出现问题，我们可以随时回到这个已经测试过的版本。

## 13.2 保存笔记

建议将本篇笔记保存为：

```text
Documents/
└── STM32G431_Hall_FOC_Bootstrap_RealAlignment.md
```

也可以放入你原来存放阶段笔记的目录，保持原有文件结构即可。

## 13.3 提交 Git

在 VSCode 或 Git Bash 中打开项目仓库根目录。

首先检查：

```bash
git status
```

确认本阶段实际修改的文件。

然后：

```bash
git add -A
```

注意：提交前确认没有误加入 Debug 编译产物、大型临时文件或不需要的备份。

检查暂存区：

```bash
git diff --cached --stat
```

确认内容正确后：

```bash
git commit -m "Implement Hall FOC startup and first motor alignment test"
```

最后推送：

```bash
git push
```

## 13.4 验证提交成功

执行：

```bash
git status
```

预期：

```text
On branch main

Your branch is up to date with 'origin/main'.

nothing to commit, working tree clean
```

实际分支名称以当前仓库为准。

还可以执行：

```bash
git log -1 --oneline
```

查看最近一次提交。

## 13.5 本次 Git Commit 的意义

本次提交对应：

**Hall-FOC startup framework and first physical motor excitation**

代表：

- Hall 角度软件链路完成
- Bootstrap 状态机完成
- SW3 按键启动完成
- 真实固定矢量励磁产生机械响应

下一次建议在低速开环连续旋转成功后提交：

```text
Implement low-speed open-loop motor rotation
```

---

# 十四、本阶段个人学习总结

经过这一阶段，我已经不再只是分别验证 STM32 外设和 FOC 数学模块，而是开始将它们组成真正的电机控制系统。

本阶段最重要的收获是：

1. 理解了上电初始化与实际电机启动的区别。
2. 了解了三相驱动器自举充电的基本原理。
3. 学会利用状态机组织复杂的电机启动流程。
4. 学会通过按键发送启动请求，而不是依赖 Debugger。
5. 理解了 Hall 角度有效、角度偏置和实际电角度之间的关系。
6. 第一次观察到自己编写的程序产生真实电机机械响应。
7. 认识到寄存器状态、软件计算结果与真实物理响应不能混为一谈。
8. 开始通过控制参数变化和真实实验现象分析电机响应。

**下一阶段的唯一核心目标：让电机在低速开环旋转磁场驱动下，实现第一次连续转动。**
