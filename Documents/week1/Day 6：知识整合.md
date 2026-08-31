可以。Day 6 我建议不要再单独学新的 C 语法，而是做一次**“前 5 天知识整合”**。

今天的定位是：

> **拿一个简化的 PMSM FOC 电流环 PI 模块，把数据类型、数组、指针、struct、typedef、const、static、宏、`.h/.c`、函数声明/定义全部串起来。**

但不要把 Day 6 变成“正式学习数字 PI”。你原计划真正系统学习 `Kp/Ki、Sampling Time、Output Limit、Integral Limit、Anti-Windup` 是 Week 6 的任务。

所以今天我们只提前接触最基础的：

```
误差
↓
积分状态
↓
PI计算
↓
输出限幅
```

不调参、不推导带宽、不研究 Anti-Windup。

------

# Day 6 最终目标

今天结束以后，你应该能够看懂这样一段未来非常常见的代码：

```
vd = PI_Run(&pi_d, id_ref, id);
vq = PI_Run(&pi_q, iq_ref, iq);
```

并且能说清楚：

```
pi_d / pi_q
→ 两个不同的 PI 控制器结构体

&pi_d
→ 把 d 轴 PI 的地址交给函数

id_ref
→ d轴电流参考值

id
→ d轴实际反馈值

vd
→ d轴 PI 输出

vq
→ q轴 PI 输出
```

更重要的是，你能够顺着代码找到：

```
main.c
  ↓
PI_Run()
  ↓
pi.c
  ↓
error
integral
output
limit
```

这就是 Day 6 最重要的能力。

------

# 一、今天的工程结构

继续沿用 Day 5，不搞太复杂：

```
Day6_Current_Loop/

├── main.c
├── pi.c
└── pi.h
```

但今天我们把 `PI_Controller_t` 稍微升级一下。

------

# 二、今天使用的 `pi.h`

```
#ifndef PI_H
#define PI_H


typedef struct
{
    /* PI 参数 */
    float kp;
    float ki;

    /* 控制周期 */
    float ts;

    /* 运行状态 */
    float integral;
    float output;

    /* 输出限制 */
    float output_min;
    float output_max;

} PI_Controller_t;


/* 初始化 PI */
void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float ts,
             float output_min,
             float output_max);


/* 执行一次 PI */
float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback);


/* 清除积分和输出 */
void PI_Reset(PI_Controller_t *pi);


#endif
```

今天先观察一个变化。

Day 5 结构体：

```
typedef struct
{
    float kp;
    float ki;

    float integral;

    float output_max;
    float output_min;

    float output;

} PI_Controller_t;
```

今天增加：

```
float ts;
```

为什么？

因为以后数字控制器不是连续运行的。

而是：

```
第1次控制
↓
等待 Ts
↓
第2次控制
↓
等待 Ts
↓
第3次控制
...
```

比如以后：

```
20 kHz 电流环
```

对应：

```
Ts = 1 / 20000

   = 0.00005 s

   = 50 μs
```

所以真正的数字 PI 必须知道自己的控制周期。

------

# 三、今天的 `pi.c`

```
#include "pi.h"


/* =========================
 * PI 内部限幅函数
 * ========================= */

static float PI_Limit(float value,
                      float min,
                      float max)
{
    if (value > max)
    {
        value = max;
    }

    if (value < min)
    {
        value = min;
    }

    return value;
}


/* =========================
 * PI 初始化
 * ========================= */

void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float ts,
             float output_min,
             float output_max)
{
    pi->kp = kp;
    pi->ki = ki;

    pi->ts = ts;

    pi->integral = 0.0f;
    pi->output = 0.0f;

    pi->output_min = output_min;
    pi->output_max = output_max;
}


/* =========================
 * PI 运行一次
 * ========================= */

float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback)
{
    float error;


    /* 1. 计算误差 */
    error = ref - feedback;


    /* 2. 更新积分 */
    pi->integral += error * pi->ts;


    /* 3. PI 输出 */
    pi->output =
        pi->kp * error
        +
        pi->ki * pi->integral;


    /* 4. 输出限幅 */
    pi->output =
        PI_Limit(pi->output,
                 pi->output_min,
                 pi->output_max);


    /* 5. 返回结果 */
    return pi->output;
}


/* =========================
 * PI 复位
 * ========================= */

void PI_Reset(PI_Controller_t *pi)
{
    pi->integral = 0.0f;

    pi->output = 0.0f;
}
```

------

# 四、今天重点看懂 PI_Run 的执行顺序

不要先看公式。

把它当程序流程看：

```
PI_Run()
  │
  ├─ 读取 ref
  │
  ├─ 读取 feedback
  │
  ├─ error = ref - feedback
  │
  ├─ 更新 integral
  │
  ├─ 计算 output
  │
  ├─ 限幅
  │
  └─ return output
```

如果是 q 轴：

```
vq = PI_Run(&pi_q,
            iq_ref,
            iq);
```

那么你可以直接替换成人话：

```
目标 Iq
  ↓

与实际 Iq 比较
  ↓

产生误差
  ↓

q轴 PI 计算
  ↓

得到 Vq
```

这已经开始非常接近真实 FOC 了。

------

# 五、为什么今天积分项改成这样

Day 5：

```
pi->integral += error;
```

今天：

```
pi->integral += error * pi->ts;
```

后者更接近数字控制的实际含义。

因为积分：

$ \int e(t)dt $

在离散程序里，可以粗略理解成不断累加：

$ e[k]T_s $

所以：

```
error * pi->ts
```

就是：

> 这一个控制周期对积分贡献了多少。

现在不用继续推导。

你只要理解：

> **控制器是一次一次运行的，所以时间间隔 Ts 很重要。**

------

# 六、这里把 Day 4 的 static 接回来了

看：

```
static float PI_Limit(...)
```

为什么有：

```
static
```

因为：

```
PI_Limit()
```

只是：

```
pi.c
```

内部自己使用。

我们不希望：

```
main.c
```

直接：

```
PI_Limit(...);
```

所以它属于：

> PI 模块内部辅助函数。

而：

```
PI_Init()
PI_Run()
PI_Reset()
```

才是：

> 对外公开接口。

------

# 七、今天的 `main.c`

今天这份代码我故意让它模拟多次电流环运行。

```
#include <stdio.h>

#include "pi.h"


#define CONTROL_FREQ_HZ    20000.0f
#define CONTROL_TS         (1.0f / CONTROL_FREQ_HZ)

#define CURRENT_LOOP_NUM   8


int main(void)
{
    /* =========================
     * 创建 Id / Iq 两个 PI
     * ========================= */

    PI_Controller_t pi_d;
    PI_Controller_t pi_q;


    /* =========================
     * 电流参考
     * ========================= */

    const float id_ref = 0.0f;
    const float iq_ref = 2.0f;


    /* =========================
     * 模拟采样得到的实际 Id
     * ========================= */

    float id_feedback[CURRENT_LOOP_NUM] =
    {
        0.30f,
        0.25f,
        0.20f,
        0.15f,
        0.10f,
        0.05f,
        0.02f,
        0.00f
    };


    /* =========================
     * 模拟采样得到的实际 Iq
     * ========================= */

    float iq_feedback[CURRENT_LOOP_NUM] =
    {
        0.00f,
        0.40f,
        0.80f,
        1.20f,
        1.50f,
        1.70f,
        1.90f,
        2.00f
    };


    float vd;
    float vq;


    /* =========================
     * PI 初始化
     * ========================= */

    PI_Init(&pi_d,
            1.0f,
            20.0f,
            CONTROL_TS,
           -10.0f,
            10.0f);


    PI_Init(&pi_q,
            1.2f,
            20.0f,
            CONTROL_TS,
           -10.0f,
            10.0f);


    printf("Control Frequency = %.0f Hz\n",
           CONTROL_FREQ_HZ);

    printf("Control Ts = %.6f s\n\n",
           CONTROL_TS);


    /* =========================
     * 模拟连续 8 个控制周期
     * ========================= */

    for (int i = 0;
         i < CURRENT_LOOP_NUM;
         i++)
    {
        vd = PI_Run(&pi_d,
                    id_ref,
                    id_feedback[i]);


        vq = PI_Run(&pi_q,
                    iq_ref,
                    iq_feedback[i]);


        printf("===== Cycle %d =====\n",
               i + 1);


        printf("Id Ref = %.2f A\n",
               id_ref);

        printf("Id     = %.2f A\n",
               id_feedback[i]);

        printf("Vd     = %.4f V\n",
               vd);


        printf("Iq Ref = %.2f A\n",
               iq_ref);

        printf("Iq     = %.2f A\n",
               iq_feedback[i]);

        printf("Vq     = %.4f V\n",
               vq);


        printf("D Integral = %.6f\n",
               pi_d.integral);

        printf("Q Integral = %.6f\n\n",
               pi_q.integral);
    }


    return 0;
}
```

------

# 八、为什么我今天故意用了数组

因为 Day 2 学过：

```
float data[3];
```

今天真正把数组拿到类似控制场景里。

比如：

```
float iq_feedback[8] =
{
    0.0f,
    0.4f,
    0.8f,
    1.2f,
    1.5f,
    1.7f,
    1.9f,
    2.0f
};
```

你可以把它理解成连续 8 个采样时刻：

```
第1次采样 → Iq = 0.0 A

第2次采样 → Iq = 0.4 A

第3次采样 → Iq = 0.8 A

...

第8次采样 → Iq = 2.0 A
```

这里不是真实 PMSM 仿真，只是为了模拟：

> **控制器连续多次获得反馈数据。**

这一点要区分清楚。

------

# 九、以后真正 STM32 中不会有这个测试数组

以后真实程序：

```
ADC
 ↓
Ia / Ib
 ↓
Clarke
 ↓
Park
 ↓
Id / Iq
```

所以未来可能是：

```
vd = PI_Run(&pi_d,
            id_ref,
            motor.current.id);

vq = PI_Run(&pi_q,
            iq_ref,
            motor.current.iq);
```

而不是：

```
iq_feedback[i]
```

今天这个数组只是拿来模拟传感器反馈。

------

# 十、今天第一次认真看 `for`

前面已经见过，但今天正式看一下。

```
for (int i = 0;
     i < CURRENT_LOOP_NUM;
     i++)
{
    ...
}
```

假设：

```
#define CURRENT_LOOP_NUM 8
```

就是：

```
i = 0
↓
运行一次

i = 1
↓
运行一次

i = 2
↓
运行一次

...

i = 7
↓
运行一次

结束
```

总共：

```
8次
```

为什么是：

```
0 ~ 7
```

因为数组：

```
float iq_feedback[8];
```

有效下标也是：

```
0 ~ 7
```

Day 2 的数组知识正好接回来。

------

# 十一、今天把所有前 5 天知识串起来

看看这份代码里其实已经有：

### Day 1：数据类型

```
float
int
```

------

### Day 2：数组

```
float iq_feedback[8];
```

------

### Day 2：指针

```
PI_Controller_t *pi
```

以及：

```
&pi_d
&pi_q
```

------

### Day 3：struct

```
PI_Controller_t
```

------

### Day 3：`.` 和 `->`

主程序：

```
pi_d.integral
```

因为：

```
pi_d
=
结构体对象
```

所以：

```
.
```

而 `pi.c`：

```
pi->integral
```

因为：

```
pi
=
结构体指针
```

所以：

```
->
```

------

### Day 4：const

```
const float id_ref = 0.0f;
```

------

### Day 4：static

```
static float PI_Limit(...);
```

------

### Day 4：#define

```
#define CONTROL_FREQ_HZ 20000.0f
```

------

### Day 5：模块化

```
pi.h
pi.c
main.c
```

今天其实就是一次综合复习。

------

# 十二、今天重点理解 Id 和 Iq 两个 PI 是完全独立的

创建：

```
PI_Controller_t pi_d;
PI_Controller_t pi_q;
```

它们虽然都是：

```
PI_Controller_t
```

但是：

```
pi_d.integral
```

和：

```
pi_q.integral
```

完全不是一个变量。

所以运行：

```
PI_Run(&pi_d, ...);
```

修改的是：

```
pi_d
```

内部状态。

运行：

```
PI_Run(&pi_q, ...);
```

修改的是：

```
pi_q
```

内部状态。

这个概念非常重要。

以后速度环再加：

```
PI_Controller_t pi_speed;
```

还是同样逻辑。

------

# 十三、为什么 id_ref 设置为 0

今天顺便让代码更贴近 SPMSM FOC：

```
const float id_ref = 0.0f;
```

以后你会经常看到：

\[ i_d^*=0 \]

在常见 SPMSM 基速以下 FOC 场景里，这是非常典型的控制方式。

你现在不用展开 MTPA、弱磁。

先理解：

```
d轴参考电流
≈ 0
```

即可。

------

# 十四、iq_ref 为什么给 2 A

```
const float iq_ref = 2.0f;
```

你现在可以粗略理解：

> q 轴电流主要与电磁转矩相关。

所以：

```
Iq_ref
```

可以理解成：

> 控制器希望电机产生对应某个转矩需求的 q 轴电流。

今天不用推转矩公式。

后面 FOC 正式学习再讲。

------

# 十五、第一次控制周期发生什么

例如第一组：

```
Iq_ref = 2.0 A

Iq = 0 A
```

所以：

```
error = ref - feedback;
```

就是：

```
2 - 0 = 2 A
```

这个误差比较大。

所以 PI：

```
Vq输出
```

也会相对较大。

------

# 十六、随着 Iq 接近参考值

数组后面：

```
Iq = 1.5
Iq = 1.7
Iq = 1.9
Iq = 2.0
```

因此误差：

```
0.5
0.3
0.1
0
```

逐渐变小。

你运行程序的时候重点观察：

```
Iq
error（可以自己加打印）
Vq
integral
```

怎么变化。

不要求分析控制性能。

只是开始建立：

```
反馈接近参考
↓
误差减小
↓
控制器输入发生变化
```

这种感觉。

------

# 十七、Day 6 实验 1：把 error 打印出来

目前：

```
PI_Run()
```

里面：

```
float error;
```

但是 `main.c` 看不到它，因为：

```
error
```

是：

> `PI_Run()` 的局部变量。

这是 Day 1 局部变量重新出现。

你可以临时在 `pi.c` 里：

```
#include <stdio.h>
```

然后：

```
printf("Error = %.2f\n", error);
```

观察每个周期。

测试完以后可以删掉。

目的：

> 理解局部变量只能在这个函数内部直接使用。

------

# 十八、实验 2：改变 Iq_ref

原来：

```
const float iq_ref = 2.0f;
```

改：

```
const float iq_ref = 3.0f;
```

其他所有东西不变。

预测：

```
误差整体更大
```

所以：

```
Vq
```

应该发生明显变化。

重点不是“3A更好吗”。

而是理解：

```
ref变化
↓
PI输入变化
↓
输出变化
```

------

# 十九、实验 3：改变 Kp

原来：

```
PI_Init(&pi_q,
        1.2f,
        20.0f,
        CONTROL_TS,
        -10.0f,
        10.0f);
```

把：

```
Kp = 1.2
```

改成：

```
Kp = 3.0
```

其他不变。

看看：

```
Vq
```

怎么变化。

这里不要开始调参。

今天只理解：

> Kp 参数真的被存进 `pi_q.kp`，而 `PI_Run()` 真正在使用它。

------

# 二十、实验 4：测试输出饱和

把：

```
iq_ref = 20.0f;
```

而输出范围：

```
-10.0f
10.0f
```

运行。

如果 PI 算出来：

```
20 V
```

最终应该被限制：

```
10 V
```

因为：

```
PI_Limit(...)
```

在工作。

这个特别贴近真实电机控制。

------

# 二十一、为什么真实 FOC 必须限压

以后：

```
Vd
Vq
```

不是想给多少就给多少。

因为逆变器受：

```
DC Bus Voltage
```

限制。

比如：

```
Vdc = 48 V
```

逆变器能输出的电压矢量范围有限。

所以后面 FOC 里：

```
电压限幅
```

是非常重要的。

今天我们的：

```
output_max = 10.0f;
```

只是为了学习程序结构。

它不是正式电机电压限制算法。

------

# 二十二、实验 5：故意制造“积分一直累加”

把：

```
iq_feedback
```

全部改成：

```
float iq_feedback[CURRENT_LOOP_NUM] =
{
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f
};
```

也就是说：

```
Iq_ref = 2 A

Iq一直 = 0
```

运行。

观察：

```
pi_q.integral
```

是不是每个周期都增加。

这就是积分的最直观体现：

```
误差一直存在
↓
每一周期都累计一点
↓
integral越来越大
```

现在先看到现象就行。

------

# 二十三、顺便提前理解为什么以后需要 Anti-Windup

如果：

```
误差长期存在
```

积分一直：

```
增加
增加
增加
增加
```

但输出可能早就：

```
达到限幅
```

例如：

```
Vq要求 = 100

实际最大只能 = 10
```

然而积分还继续累积。

这就可能产生：

> Integral Windup，积分饱和/积分累积过度。

你原计划会在 Week 6 正式学习 Anti-Windup。

**今天不要实现。**

你只要知道：

> 为什么以后 PI 还要进一步完善。

------

# 二十四、实验 6：测试 PI_Reset

在循环运行结束以后：

```
printf("Before Reset:\n");

printf("D Integral = %.6f\n",
       pi_d.integral);

printf("Q Integral = %.6f\n",
       pi_q.integral);
```

然后：

```
PI_Reset(&pi_d);
PI_Reset(&pi_q);
```

再：

```
printf("After Reset:\n");

printf("D Integral = %.6f\n",
       pi_d.integral);

printf("Q Integral = %.6f\n",
       pi_q.integral);
```

你应该看到：

```
Before Reset

有积分值


After Reset

0
0
```

这也很贴近以后实际状态机。

比如：

```
Motor STOP
↓
PI_Reset()
```

避免下次启动继续使用旧积分状态。

------

# 二十五、这和以后状态机有什么关系

以后可能：

```
if (motor_state == MOTOR_STOP)
{
    PI_Reset(&pi_d);
    PI_Reset(&pi_q);
}
```

或者电机故障：

```
if (fault_flag == 1)
{
    PI_Reset(&pi_d);
    PI_Reset(&pi_q);

    PWM_Disable();
}
```

所以：

```
PI_Reset()
```

不是为了写一个好看的函数。

是真实工程会用到的。

------

# 二十六、实验 7：把两个 PI 参数设成不同

例如：

```
PI_Init(&pi_d,
        1.0f,
        20.0f,
        ...);
```

而：

```
PI_Init(&pi_q,
        2.0f,
        30.0f,
        ...);
```

然后观察：

```
pi_d.kp = 1
pi_q.kp = 2

pi_d.ki = 20
pi_q.ki = 30
```

进一步确认：

> 同一种结构体类型，可以创建很多拥有不同数据的控制器对象。

------

# 二十七、今天最重要的“代码阅读训练”

我给你这样一段：

```
vq = PI_Run(&pi_q,
            iq_ref,
            iq_feedback[i]);
```

你以后不能只说：

> “这是调用一个函数。”

你最好能逐步读成：

```
PI_Run
↓
执行一次 PI 控制


&pi_q
↓
使用 q轴 PI 控制器本身


iq_ref
↓
目标 q轴电流


iq_feedback[i]
↓
本周期采样得到的实际 q轴电流


return
↓
得到控制器输出 Vq


vq =
↓
把结果保存到 q轴电压指令
```

这就是你未来阅读 FOC 工程真正应该形成的能力。

------

# 二十八、再提前看看以后真实控制中断大概什么样

现在我们的：

```
for (...)
{
    vd = PI_Run(...);
    vq = PI_Run(...);
}
```

只是 PC 测试。

真正 STM32 上更可能：

```
void Motor_Control_ISR(void)
{
    Current_Read();

    Clarke_Run();

    Park_Run();


    vd = PI_Run(&pi_d,
                id_ref,
                id);


    vq = PI_Run(&pi_q,
                iq_ref,
                iq);


    InvPark_Run();

    SVPWM_Run();

    PWM_Update();
}
```

然后：

```
每 50 μs
```

中断执行一次。

也就是说我们今天：

```
for(...)
```

模拟的概念，其实未来是：

```
Timer / ADC Interrupt
```

不断触发。

这也是为什么 Day 2～6 这些 C 基础必须在 Week 2 之前搞懂。

------

# 二十九、Day 6 你不要扩展这些东西

今天不要学：

- PI 参数整定
- 电流环带宽
- 电机 RL 模型调参
- 解耦补偿
- 前馈
- Anti-Windup 实现
- Id/Iq 实际计算
- Clarke/Park 公式
- SVPWM
- 中断配置
- STM32 HAL
- MCU 定时器

否则 Day 6 很容易失控。

今天仍然是：

> **C 语言综合训练。**

------

# 三十、Day 6 推荐时间安排

今天建议 **2.5～3 h**。

## 第 1 阶段：30 分钟

把：

```
pi.h
pi.c
main.c
```

建立好并运行。

先确认程序没有编译错误。

------

## 第 2 阶段：30 分钟

重点研究：

```
PI_Controller_t
```

看：

```
Kp
Ki
Ts
Integral
Output
Limit
```

为什么要放在同一个结构体。

------

## 第 3 阶段：40 分钟

专门跟踪一次：

```
vq = PI_Run(&pi_q,
            iq_ref,
            iq_feedback[i]);
```

进入：

```
pi.c
```

逐行看：

```
ref
feedback
error
integral
output
limit
return
```

这是今天最重要的一部分。

------

## 第 4 阶段：30～40 分钟

做 3 个实验：

1. 改 `Iq_ref`
2. 改 `Kp`
3. 制造输出限幅

不需要全部七个实验都做。

------

## 第 5 阶段：30 分钟

做：

```
Iq一直等于0
```

观察积分累加。

然后执行：

```
PI_Reset()
```

看看积分清零。

------

# 三十一、Day 6 验收题

今天结束后，不查资料回答。

### 1

为什么 PI 结构体里需要：

```
float integral;
```

而不能每次 PI_Run 都重新置 0？

------

### 2

```
float ts;
```

代表什么？

------

### 3

20 kHz 控制频率对应：

```
Ts大约多少？
```

答案：

```
0.00005 s
=
50 μs
```

------

### 4

```
PI_Controller_t pi_d;
PI_Controller_t pi_q;
```

是不是两个独立控制器？

------

### 5

为什么：

```
PI_Run(&pi_q, ...)
```

需要：

```
&pi_q
```

？

------

### 6

为什么 `pi.c` 里面：

```
pi->integral
```

用 `->`？

------

### 7

而 `main.c`：

```
pi_q.integral
```

为什么用 `.`？

------

### 8

```
static float PI_Limit(...)
```

为什么用 `static`？

------

### 9

输出限幅的意义是什么？

------

### 10

为什么 PI_Reset 通常需要清空：

```
integral
output
```

？

------

### 11

数组：

```
float iq_feedback[8];
```

这里为什么合法下标是：

```
0～7
```

？

------

### 12

下面这句能不能完整用人话解释：

```
vq = PI_Run(&pi_q,
            iq_ref,
            iq_feedback[i]);
```

如果这 12 个问题你能答出 9～10 个，**Day 6 就已经完全达标。**

------

# Day 6 最值得记住的一张图

```
                    第 k 次控制周期

Iq_ref ─────────┐
                │
                ▼
            ┌────────┐
Iq ────────►│   PI   │
            └────────┘
                │
                ▼
               Vq


程序内部：

PI_Run(&pi_q, iq_ref, iq)

        ↓

error = ref - feedback

        ↓

integral += error × Ts

        ↓

output = Kp × error
       + Ki × integral

        ↓

Output Limit

        ↓

return Vq
```

------

# Day 6 做完之后，你第一周其实已经基本打通了

到这里你已经接触：

```
变量
数组
指针
struct
typedef
.
->
const
static
volatile
extern
#define
.h/.c
函数声明
函数定义
模块化
多次控制周期
```

你的 Week 1 原目标本身并不是“精通 C”，而是补齐电控软件开发最常用的 C 基础，并最终能使用结构体、看懂简单指针、理解 `volatile/static` 等。半年学习计划.mdMD

所以 **Day 7 我不建议再学新语法**。Day 7 最适合做一次“第一周总验收”：我可以直接给你一份**小型 PMSM FOC 风格代码阅读题 + 故障代码 Debug 题**，看看前 6 天到底是真看懂了，还是只是“看讲解时感觉懂了”。这会比继续塞新知识更有价值。