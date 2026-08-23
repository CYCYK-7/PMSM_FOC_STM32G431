好，Day 4 进入第一周里**最有“嵌入式味道”的一天**：

> `static + const + volatile + extern + #define`

这几个东西你以后在 STM32、电机控制、驱动层、控制层代码里会反复看到。你的学习计划里也明确把它们列为 Week 1 必须掌握内容，并特别强调真正理解 `volatile / static / const`。

今天我们继续按前 3 天的方式：

> **不让你从零默写，直接给贴近 PMSM / STM32 / FOC 工程的代码，然后重点看懂和会改。**

---

# Day 4 最终目标

今天结束以后，你看到下面这些，不应该再陌生：

```c
static float integral;
```

知道：

> 这个变量有“保持值”或“限制作用域”的意味，具体取决于它写在哪。

看到：

```c
const float Ts = 0.0001f;
```

知道：

> 这是一个不应该被修改的常量。

看到：

```c
volatile uint16_t adc_raw;
```

知道：

> 这个变量可能被中断、DMA、硬件等“程序正常流程之外”的东西修改，所以不能让编译器随便优化。

看到：

```c
extern float motor_speed;
```

知道：

> 变量在别的 `.c` 文件里真正定义，这里只是声明“它存在”。

看到：

```c
#define PWM_FREQ_HZ 20000U
```

知道：

> 定义一个编译期宏，常用来统一配置参数。

---

# 一、Day 4 先给你完整示例

今天直接用一份“模拟 STM32 电机控制程序”的代码。

```c
#include <stdio.h>
#include <stdint.h>


/* =========================
 * 宏定义
 * ========================= */

#define PWM_FREQ_HZ        20000U
#define CONTROL_FREQ_HZ    20000U

#define MOTOR_ENABLE       1U
#define MOTOR_DISABLE      0U

#define CURRENT_LIMIT_A    5.0f

#define TWO_PI             6.2831853f


/* =========================
 * 常量
 * ========================= */

const float control_ts = 1.0f / CONTROL_FREQ_HZ;


/* =========================
 * 模拟硬件/中断共享变量
 * ========================= */

volatile uint16_t adc_current_raw = 2048U;

volatile uint8_t adc_ready = 0U;

volatile uint32_t control_isr_count = 0U;


/* =========================
 * 普通全局变量
 * ========================= */

float phase_current = 0.0f;

float iq_ref = 0.0f;

uint8_t motor_enable = MOTOR_DISABLE;


/* =========================
 * 模拟ADC中断
 * ========================= */

void ADC_Interrupt_Handler(void)
{
    adc_current_raw = 2148U;

    adc_ready = 1U;
}


/* =========================
 * ADC转换电流
 * ========================= */

float ADC_To_Current(uint16_t adc_raw)
{
    const float adc_offset = 2048.0f;

    const float current_scale = 0.01f;

    float current;

    current =
        ((float)adc_raw - adc_offset)
        * current_scale;

    return current;
}


/* =========================
 * 控制中断
 * ========================= */

void Motor_Control_ISR(void)
{
    static float last_current = 0.0f;

    control_isr_count++;


    if (adc_ready == 1U)
    {
        phase_current =
            ADC_To_Current(adc_current_raw);

        last_current = phase_current;

        adc_ready = 0U;
    }


    printf("Current     = %.2f A\n",
           phase_current);

    printf("Last Current= %.2f A\n",
           last_current);

    printf("ISR Count   = %lu\n",
           (unsigned long)control_isr_count);
}


/* =========================
 * 电流限幅
 * ========================= */

void Current_Limit(void)
{
    if (iq_ref > CURRENT_LIMIT_A)
    {
        iq_ref = CURRENT_LIMIT_A;
    }

    if (iq_ref < -CURRENT_LIMIT_A)
    {
        iq_ref = -CURRENT_LIMIT_A;
    }
}


/* =========================
 * 主程序
 * ========================= */

int main(void)
{
    printf("PWM Frequency     = %u Hz\n",
           PWM_FREQ_HZ);

    printf("Control Frequency = %u Hz\n",
           CONTROL_FREQ_HZ);

    printf("Control Ts        = %.6f s\n",
           control_ts);


    motor_enable = MOTOR_ENABLE;

    iq_ref = 8.0f;

    Current_Limit();

    printf("Iq Ref After Limit = %.2f A\n",
           iq_ref);


    ADC_Interrupt_Handler();

    Motor_Control_ISR();

    Motor_Control_ISR();

    Motor_Control_ISR();


    return 0;
}
```

今天就围绕这份代码理解 5 个关键词。

---

# 二、先学 `const`

看：

```c
const float control_ts = 1.0f / CONTROL_FREQ_HZ;
```

这里：

```c
const
```

可以先理解成：

> 这个变量定义之后，不希望再被修改。

比如：

```c
const float control_ts = 0.00005f;
```

以后如果你写：

```c
control_ts = 0.001f;
```

编译器通常会报错。

因为你已经告诉它：

> `control_ts` 是常量。

---

# 三、为什么电机控制里 `const` 很常见

以后会有很多这种参数：

```c
const float Ts;
const float TWO_PI;
const float SQRT_3;
const float adc_scale;
```

因为这些东西本质上是：

> 运行过程中不应该乱变的固定参数。

例如控制周期：

```text
20 kHz
```

那么：

```text
Ts = 1 / 20000
```

也就是：

```text
0.00005 s
```

这种东西通常就不希望运行时突然被其他代码改掉。

---

# 四、`const` 最值得你理解的工程意义

不是为了语法好看。

而是：

> **防止程序里某个地方误改重要参数。**

比如：

```c
const float motor_rs = 1.2f;
```

如果别的代码误写：

```c
motor_rs = 5.0f;
```

编译阶段就能发现问题。

所以 `const` 是一种：

> 代码保护和约束。

---

# 五、接下来是 `static`

这个要分两种场景。

---

# 六、第一种：函数里的 `static`

看：

```c
void Motor_Control_ISR(void)
{
    static float last_current = 0.0f;

    ...
}
```

这个：

```c
last_current
```

虽然写在函数里面，

但是和普通局部变量不一样。

---

## 普通局部变量

例如：

```c
void Test(void)
{
    float value = 0.0f;

    value++;
}
```

每次进入函数：

```text
value = 0
```

然后：

```text
value = 1
```

函数结束。

下次再调用：

又重新：

```text
value = 0
```

---

## static 局部变量

如果：

```c
void Test(void)
{
    static float value = 0.0f;

    value++;
}
```

第一次：

```text
0 → 1
```

第二次：

```text
1 → 2
```

第三次：

```text
2 → 3
```

也就是说：

> 函数退出以后，值仍然保留。

---

# 七、这在电机控制里特别常见

以后你可能看到：

```c
static float integral;
```

例如 PI：

```c
void PI_Run(void)
{
    static float integral = 0.0f;

    integral += error;
}
```

因为积分项必须：

> 一次控制周期接着下一次控制周期继续累加。

如果每次进入函数都：

```text
integral = 0
```

那 PI 的积分就永远积不起来。

当然，真正工程里更常见的是把 integral 放进结构体。

但 `static` 的思想你必须懂。

---

# 八、我们的例子里为什么 `last_current` 用 static

```c
static float last_current = 0.0f;
```

第一次 ADC 更新：

```text
last_current = 1.0 A
```

后面即使：

```text
adc_ready = 0
```

没有新 ADC 数据，

`last_current` 仍然记得上一次的值。

这就是：

> 保留历史状态。

---

# 九、第二种 static：文件级变量

以后你在 `.c` 文件顶部会看到：

```c
static float motor_speed;
```

如果它写在函数外面，

这里的 `static` 重点不是“保留值”。

因为普通全局变量本来就一直存在。

它此时更重要的作用是：

> **限制这个变量只在当前 `.c` 文件内部使用。**

比如：

```c
/* motor.c */

static float speed_filter_state;
```

那么其他文件一般不能直接访问它。

这很适合：

> 模块内部私有变量。

---

# 十、为什么文件级 static 很重要

假设整个工程里有：

```text
adc.c
pwm.c
foc.c
encoder.c
motor.c
```

如果每个内部变量都暴露出去：

```text
谁都能修改谁
```

项目会越来越乱。

所以：

```c
static float internal_state;
```

可以表达：

> 这是本模块内部状态，外面不要碰。

这就是一种工程封装。

---

# 十一、现在进入最重要的 `volatile`

看：

```c
volatile uint16_t adc_current_raw;
```

以及：

```c
volatile uint8_t adc_ready;
```

这两个非常像以后 STM32 真正程序。

---

# 十二、为什么 ADC 数据常和 volatile 有关系

假设主程序：

```c
while (1)
{
    if (adc_ready == 1)
    {
        ...
    }
}
```

但：

```c
adc_ready
```

不是主循环自己改的。

可能是 ADC 中断：

```c
void ADC_IRQHandler(void)
{
    adc_ready = 1;
}
```

也就是说：

```text
主程序正在运行
      ↓
adc_ready = 0

突然发生ADC中断
      ↓
adc_ready = 1
```

对于主循环来说，

这个变量可能：

> 在自己没看到代码修改它的情况下突然变化。

---

# 十三、编译器为什么会“自作聪明”

编译器为了提高效率，可能会想：

```text
这个变量我刚刚读过了

附近又没代码修改它

那我是不是不用每次都重新去内存读？
```

对普通变量，这种优化很多时候没问题。

但对：

```text
中断
DMA
硬件寄存器
```

相关变量就危险了。

因为它们可能被：

> 其他执行环境修改。

所以我们写：

```c
volatile uint8_t adc_ready;
```

相当于告诉编译器：

> **别假设这个值不会变，每次需要的时候认真读取。**

---

# 十四、Day 4 你怎样理解 volatile 最合适

暂时不要背复杂定义。

你先记：

> `volatile` = 这个变量可能被“当前代码看不到的地方”修改。

常见来源：

```text
中断
DMA
硬件寄存器
其他执行上下文
```

例如：

```c
volatile uint16_t adc_raw;
volatile uint8_t uart_rx_done;
volatile uint32_t encoder_count;
volatile uint8_t fault_flag;
```

---

# 十五、一个以后非常真实的场景

比如：

```c
volatile uint8_t control_flag = 0;
```

定时器中断：

```c
void TIM_IRQHandler(void)
{
    control_flag = 1;
}
```

主循环：

```c
while (1)
{
    if (control_flag == 1)
    {
        control_flag = 0;

        Motor_Control();
    }
}
```

这个：

```c
control_flag
```

就很典型适合 `volatile`。

---

# 十六、非常重要：volatile 不等于线程安全

这点你现在只要有个印象。

`volatile` 只是：

> 告诉编译器这个值可能随时变化。

它**不代表**：

```text
原子操作
线程安全
不会产生竞争
```

现在不用深入。

只要别形成：

> “加了 volatile 就什么并发问题都解决了”

这种错误认识。

---

# 十七、接下来是 `extern`

假设未来你开始分文件。

例如：

```text
motor.c
motor.h
main.c
```

在 `motor.c`：

```c
float motor_speed = 0.0f;
```

这里真正创建了变量。

---

在 `motor.h`：

```c
extern float motor_speed;
```

意思：

> `motor_speed` 在其他地方已经定义，这里只是告诉别的文件“有这个变量”。

---

然后 `main.c`：

```c
#include "motor.h"

printf("%f", motor_speed);
```

就能访问。

---

# 十八、`extern` 最容易理解成一句话

```c
extern float motor_speed;
```

不要理解成：

> 又创建了一个 motor_speed。

而是：

> **告诉编译器，motor_speed 在其他文件里存在。**

真正定义：

```c
float motor_speed = 0.0f;
```

一般只应该有一次。

---

# 十九、为什么要 extern

因为以后你的程序不会全部写在：

```text
main.c
```

而会逐渐分成：

```text
adc.c
pwm.c
foc.c
pi.c
encoder.c
motor.c
```

有些变量可能需要跨文件共享。

这时候就会碰到：

```c
extern
```

所以 Day 5 学 `.c/.h` 时，这个会马上用上。

---

# 二十、最后一个：`#define`

看：

```c
#define PWM_FREQ_HZ 20000U
```

你可以先简单理解：

> 给 `20000U` 起一个有意义的名字。

以后代码：

```c
PWM_FREQ_HZ
```

比直接写：

```c
20000
```

好很多。

---

# 二十一、为什么不要满代码写“魔法数字”

例如：

```c
if (current > 5.0f)
{
    current = 5.0f;
}
```

以后你看：

```text
5.0 是什么？
```

可能不清楚。

所以：

```c
#define CURRENT_LIMIT_A 5.0f
```

然后：

```c
if (current > CURRENT_LIMIT_A)
```

马上知道：

> 电流限幅。

这就是可读性。

---

# 二十二、以后电机程序里宏会非常多

比如：

```c
#define PWM_FREQ_HZ        20000U

#define ADC_MAX_COUNT      4095U

#define MOTOR_POLE_PAIRS   4U

#define CURRENT_LIMIT_A    5.0f

#define SPEED_LIMIT_RPM    3000.0f
```

以后 STM32 工程还会有很多：

```c
#define
```

由 HAL、CMSIS、芯片头文件自动定义。

你不用背。

只要能看懂。

---

# 二十三、`#define` 和 `const` 有什么区别

你现在不用学编译器底层细节。

先用工程视角理解：

### `#define`

更像：

> 编译前文本替换 / 宏配置。

例如：

```c
#define PWM_FREQ_HZ 20000U
```

---

### `const`

是真正有类型的对象：

```c
const float control_ts = 0.00005f;
```

有：

```text
float
```

这个类型。

---

你现在可以粗略遵循：

### 配置、开关、固定宏

常见：

```c
#define
```

### 有明确数据类型的只读变量

常见：

```c
const
```

不需要追究绝对规则。

---

# 二十四、Day 4 现在把五个东西放一起

你可以这样记：

| 关键词 | 你现在先理解成 |
|---|---|
| `const` | 不允许随意修改 |
| `static` 局部 | 函数退出后值保留 |
| `static` 文件级 | 只给当前 `.c` 文件内部使用 |
| `volatile` | 可能被中断/DMA/硬件等修改 |
| `extern` | 变量在别处定义 |
| `#define` | 宏定义/统一配置 |

这张表今天最重要。

---

# 二十五、Day 4 修改实验 1：测试 const

原来：

```c
const float control_ts =
    1.0f / CONTROL_FREQ_HZ;
```

故意在 `main()` 加：

```c
control_ts = 0.001f;
```

编译。

你应该看到报错。

目的不是折腾错误。

而是让你真正体会：

> `const` 不允许你这样修改。

---

# 二十六、实验 2：测试 static 保留值

加一个函数：

```c
void Static_Test(void)
{
    static uint32_t count = 0U;

    count++;

    printf("Static Count = %lu\n",
           (unsigned long)count);
}
```

然后：

```c
Static_Test();
Static_Test();
Static_Test();
```

预测：

```text
1
2
3
```

---

# 二十七、实验 3：去掉 static

把：

```c
static uint32_t count = 0U;
```

改：

```c
uint32_t count = 0U;
```

再次执行三次。

预测：

```text
1
1
1
```

为什么？

因为每次函数进入都会重新：

```text
count = 0
```

这组对比一定做。

它比背定义有效得多。

---

# 二十八、实验 4：测试 volatile 场景

我们的：

```c
ADC_Interrupt_Handler();
```

会：

```c
adc_current_raw = 2148U;
adc_ready = 1U;
```

然后：

```c
Motor_Control_ISR();
```

才会读取 ADC。

你先按照逻辑追踪：

```text
最开始：

adc_current_raw = 2048
adc_ready = 0


中断后：

adc_current_raw = 2148
adc_ready = 1


控制中断执行后：

phase_current = ?

adc_ready = ?
```

因为：

```text
2148 - 2048 = 100
```

再乘：

```text
0.01
```

所以：

```text
phase_current = 1.0 A
```

然后：

```text
adc_ready = 0
```

---

# 二十九、实验 5：测试 #define

把：

```c
#define CURRENT_LIMIT_A 5.0f
```

改：

```c
#define CURRENT_LIMIT_A 3.0f
```

而：

```c
iq_ref = 8.0f;
```

运行。

最终：

```text
iq_ref = 3 A
```

你会发现：

> 以后很多系统参数只需要改一个宏，就可以统一改变行为。

---

# 三十、实验 6：创建一个电机参数宏

自己加入：

```c
#define MOTOR_POLE_PAIRS 4U
```

然后：

```c
printf("Pole Pairs = %u\n",
       MOTOR_POLE_PAIRS);
```

未来你会经常看到：

```c
#define POLE_PAIRS
```

或者放进：

```c
Motor_Parameter_t
```

结构体里。

现在只是先习惯。

---

# 三十一、非常贴近你未来工程的一份组合代码

以后可能出现：

```c
#define CONTROL_FREQ_HZ 20000U

const float Ts =
    1.0f / CONTROL_FREQ_HZ;

volatile uint16_t adc_raw;

static float current_filter_state;
```

这四行你以后应该能直接读成：

```text
CONTROL_FREQ_HZ
→ 控制频率配置宏

Ts
→ 固定控制周期

adc_raw
→ 可能被ADC/DMA/中断修改的数据

current_filter_state
→ 当前模块内部需要长期保存的状态
```

如果你能做到这一点，Day 4 就已经很有价值了。

---

# 三十二、再提前看一下以后真实 STM32 风格

例如：

```c
volatile uint8_t adc_conversion_done = 0U;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    adc_conversion_done = 1U;
}
```

你现在还没学 HAL。

但你已经能理解最重要的部分：

```text
ADC 完成
 ↓
回调函数被执行
 ↓
adc_conversion_done = 1
 ↓
主程序/控制代码看到这个变化
```

所以：

```c
volatile
```

非常合理。

到了 Week 4 ADC/DMA 时，这个概念会再次出现。

---

# 三十三、以后 PWM 也会看到宏

比如：

```c
#define PWM_FREQ_HZ 20000U

#define DEADTIME_NS 500U
```

到了 Week 3：

```text
TIM1
PWM
互补PWM
Dead Time
```

你会重新看到这些配置。

所以 Day 4 学的不是孤立知识。

---

# 三十四、今天不要深入这些

暂时不要学：

```text
volatile const
const 指针
指向 const 的指针
static inline
宏函数
条件编译复杂玩法
#ifdef
#ifndef 深入原理
链接器
编译单元
storage class 细节
```

Day 5 `.c/.h` 会简单碰到：

```c
#ifndef
#define
#endif
```

到时候再讲。

---

# 三十五、Day 4 时间安排

建议今天 **2.5 小时左右**。

## 第 1 阶段：30 分钟

运行完整程序。

重点先认识：

```c
const
static
volatile
#define
```

不要一次研究太深。

---

## 第 2 阶段：30 分钟

专门做：

```text
static 对比实验
```

一个：

```c
static uint32_t count
```

一个：

```c
uint32_t count
```

观察：

```text
1 2 3
```

和：

```text
1 1 1
```

---

## 第 3 阶段：40 分钟

重点理解：

```c
volatile uint8_t adc_ready;
```

围绕：

```text
ADC中断
 ↓
adc_ready改变
 ↓
控制程序读取
```

理解。

这是今天最值得花时间的地方。

---

## 第 4 阶段：30 分钟

研究：

```c
const
#define
```

改变：

```c
CURRENT_LIMIT_A
CONTROL_FREQ_HZ
```

观察结果。

---

## 第 5 阶段：20～30 分钟

简单看：

```c
extern
```

不用做复杂多文件工程。

Day 5 我们会真正通过：

```text
pi.c
pi.h
main.c
```

把 `extern`、函数声明、头文件一次串起来。

---

# 三十六、Day 4 验收题

今天结束后，不查资料回答。

### 1

```c
const float Ts = 0.00005f;
```

是什么意思？

---

### 2

为什么：

```c
Ts = 0.001f;
```

不应该允许？

---

### 3

函数内部：

```c
static uint32_t count;
```

和普通：

```c
uint32_t count;
```

最大区别是什么？

---

### 4

文件顶部：

```c
static float filter_state;
```

主要有什么工程意义？

---

### 5

```c
volatile uint16_t adc_raw;
```

为什么 ADC 相关变量经常这样写？

---

### 6

`volatile` 是否意味着：

> 多线程/中断下绝对安全？

答案：

> 不是。

---

### 7

```c
extern float speed;
```

是不是又创建了一个 `speed`？

答案：

> 不是。

---

### 8

`extern` 的核心意义是什么？

---

### 9

```c
#define CURRENT_LIMIT_A 5.0f
```

有什么好处？

---

### 10

为什么：

```c
CURRENT_LIMIT_A
```

比代码里到处写：

```c
5.0f
```

更好维护？

如果能答出 8 个以上，Day 4 完全可以通过。

---

# 三十七、Day 4 最值得记住的一张图

```text
const
↓
这个值不应该再改


static（函数内）
↓
记住上一次的值


static（文件级）
↓
只让本 .c 文件使用


volatile
↓
这个值可能被中断 / DMA / 硬件等改变


extern
↓
真正变量在其他文件


#define
↓
统一定义配置和宏
```

---

# 三十八、Day 4 和你以后 FOC 工程的对应关系

以后你可能真的看到：

```c
#define PWM_FREQ_HZ 20000U

const float control_ts = 0.00005f;

volatile uint16_t adc_ia_raw;
volatile uint16_t adc_ib_raw;

static float speed_filter_state;

extern Motor_Control_t motor;
```

你今天学完以后应该能大概解释：

> `PWM_FREQ_HZ` 是 PWM 配置；`control_ts` 是固定控制周期；ADC 原始值可能由硬件/中断/DMA更新，所以用 `volatile`；滤波器内部状态只在本模块使用，用 `static`；`motor` 在别的文件定义，通过 `extern` 跨文件使用。

这已经是非常典型的嵌入式代码阅读能力了。

---

## Day 4 最重要的优先级

如果今天时间不够，优先级按这个顺序：

> **`volatile` > `static` > `const` > `extern` > `#define`**

尤其是 `volatile`，你以后 Week 2 学中断、Week 4 学 ADC/DMA、再往后学实时控制 ISR 时会不断遇到。

而 Day 5 下一步正好进入：

> **`.h + .c + 函数声明/定义 + 模块化`**

到时候我们会第一次把 PI 控制器真正拆成 `pi.h / pi.c / main.c`，让你看懂 STM32 工程为什么有那么多文件，而不是所有代码都塞在 `main.c` 里。