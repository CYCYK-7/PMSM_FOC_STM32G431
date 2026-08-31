好，Day 7 我建议完全不再加新知识，而是做一整天的：

> **第一周总验收 + 代码阅读 + 小型 Debug + Git 收尾**

你这份半年计划里，Week 1 的验收本来就不是“会写完整 C 工程”，而是能解释 `volatile`、`static`，会使用 `struct`，能看懂简单指针，并完成 GitHub 仓库、Commit、Push。

所以 Day 7 的目标不是继续学，而是确认：

> **前 6 天你到底是真懂了，还是只是“看讲解时觉得懂”。**

------

# Day 7 最终目标

今天结束以后，你应该做到：

```
看到一份小型 PMSM/FOC 风格 C 程序
        ↓
能判断文件结构
        ↓
能判断变量是什么
        ↓
能判断 struct / pointer
        ↓
能判断 . 和 ->
        ↓
能判断 const / static / volatile
        ↓
能跟踪一次 PI_Run
        ↓
能找到几个明显 Bug
        ↓
能自己修改并重新运行
```

如果做到这些：

> **Week 1 C 语言部分就直接通过，不再继续纠结 C。**

然后进入 Week 2 的 STM32、GPIO、UART、Timer。

------

# 一、Day 7 时间安排

今天建议 **2～3 小时**。

分四块：

| 阶段 | 内容               | 时间       |
| ---- | ------------------ | ---------- |
| 1    | 第一周知识快速口述 | 20～30 min |
| 2    | FOC 风格代码阅读   | 40～50 min |
| 3    | Debug 挑错题       | 40～50 min |
| 4    | Git + 第一周复盘   | 30～40 min |

------

# 二、第一阶段：闭卷口述 15 个问题

先不要查资料。

直接自己回答。

------

## 1. `uint16_t`

```
uint16_t adc_raw;
```

它是什么？

你应该能说：

> 16 位无符号整数，适合保存 ADC 原始值、PWM 计数值之类的数据。

------

## 2. `float`

```
float iq;
```

为什么电流、速度、电压常用 `float`？

------

## 3. 数组

```
float current_abc[3];
```

能存几个元素？

合法下标是多少？

------

## 4. `&`

```
&iq
```

是什么意思？

------

## 5. `*`

```
float *p;
```

和：

```
*p
```

分别是什么意思？

------

## 6. `struct`

为什么不把：

```
kp
ki
integral
output
```

全都写成散乱变量？

------

## 7. `typedef`

```
PI_Controller_t
```

到底是变量，还是数据类型？

------

## 8. `.`

```
pi_d.kp
```

什么时候用？

------

## 9. `->`

```
pi->kp
```

什么时候用？

------

## 10. `const`

```
const float Ts;
```

代表什么？

------

## 11. `static` 局部变量

```
static uint32_t count;
```

和普通局部变量区别是什么？

------

## 12. `static` 文件级变量

```
static float filter_state;
```

主要意义是什么？

------

## 13. `volatile`

```
volatile uint16_t adc_raw;
```

为什么 ADC / 中断变量常这样定义？

------

## 14. `extern`

```
extern float motor_speed;
```

是不是又创建了一个变量？

------

## 15. `.h` 和 `.c`

你能不能用一句话说清楚：

```
.h
```

和：

```
.c
```

分别干什么？

------

如果这 15 个问题你能直接答出 12 个以上：

> 第一关通过。

------

# 三、第二阶段：完整代码阅读题

下面这段代码，我建议你今天认真读。

不是让你写。

------

## `motor_control.h`

```
#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdint.h>

#define CONTROL_FREQ_HZ     20000.0f
#define CURRENT_LIMIT_A     5.0f

typedef struct
{
    float kp;
    float ki;

    float ts;

    float integral;
    float output;

    float output_min;
    float output_max;

} PI_Controller_t;


typedef struct
{
    float id;
    float iq;

    float vd;
    float vq;

} FOC_Data_t;


void Motor_Control_Init(void);

void Motor_Control_Run(float id_ref,
                       float iq_ref,
                       float id_feedback,
                       float iq_feedback);

#endif
```

------

## `motor_control.c`

```
#include "motor_control.h"


static PI_Controller_t pi_d;
static PI_Controller_t pi_q;

static FOC_Data_t foc_data;


static float Limit(float value,
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


static float PI_Run(PI_Controller_t *pi,
                    float ref,
                    float feedback)
{
    float error;

    error = ref - feedback;

    pi->integral += error * pi->ts;

    pi->output =
        pi->kp * error
        +
        pi->ki * pi->integral;

    pi->output =
        Limit(pi->output,
              pi->output_min,
              pi->output_max);

    return pi->output;
}


void Motor_Control_Init(void)
{
    const float ts =
        1.0f / CONTROL_FREQ_HZ;


    pi_d.kp = 1.0f;
    pi_d.ki = 20.0f;
    pi_d.ts = ts;

    pi_d.integral = 0.0f;
    pi_d.output = 0.0f;

    pi_d.output_min = -10.0f;
    pi_d.output_max = 10.0f;


    pi_q.kp = 1.2f;
    pi_q.ki = 20.0f;
    pi_q.ts = ts;

    pi_q.integral = 0.0f;
    pi_q.output = 0.0f;

    pi_q.output_min = -10.0f;
    pi_q.output_max = 10.0f;
}


void Motor_Control_Run(float id_ref,
                       float iq_ref,
                       float id_feedback,
                       float iq_feedback)
{
    foc_data.id = id_feedback;
    foc_data.iq = iq_feedback;


    foc_data.vd =
        PI_Run(&pi_d,
               id_ref,
               foc_data.id);


    foc_data.vq =
        PI_Run(&pi_q,
               iq_ref,
               foc_data.iq);
}
```

------

## `main.c`

```
#include <stdio.h>

#include "motor_control.h"


int main(void)
{
    float id_ref = 0.0f;
    float iq_ref = 2.0f;


    Motor_Control_Init();


    Motor_Control_Run(id_ref,
                      iq_ref,
                      0.2f,
                      0.5f);


    printf("Motor control executed.\n");

    return 0;
}
```

------

# 四、你要按这个顺序读代码

今天不要从第一行机械往下读。

按照工程阅读习惯。

------

## 第一步：先看 main.c

看到：

```
Motor_Control_Init();
```

先理解：

> 初始化电机控制模块。

然后：

```
Motor_Control_Run(...);
```

理解：

> 执行一次电机控制。

这时候甚至不需要立刻知道里面怎么实现。

------

# 五、第二步：去 `.h` 看接口

看到：

```
void Motor_Control_Init(void);
```

和：

```
void Motor_Control_Run(...);
```

你应该意识到：

> 这是模块对外公开的功能。

但是：

```
PI_Run()
```

为什么 `.h` 里没有？

因为它是：

```
static float PI_Run(...)
```

属于：

> `motor_control.c` 内部私有函数。

这就是 Day 4 + Day 5 知识结合。

------

# 六、第三步：进入 Motor_Control_Run

看：

```
foc_data.id = id_feedback;
foc_data.iq = iq_feedback;
```

你应该读成：

> 将反馈得到的 d/q 轴电流保存到 FOC 数据结构体中。

------

然后：

```
foc_data.vd =
    PI_Run(&pi_d,
           id_ref,
           foc_data.id);
```

你现在应该能完整解释：

```
foc_data.vd
→ 保存 d轴电压输出

PI_Run
→ 运行一次PI控制器

&pi_d
→ 把 d轴 PI 控制器地址传进去

id_ref
→ d轴目标电流

foc_data.id
→ d轴实际反馈
```

------

# 七、你今天要能画出这个程序的数据流

```
id_ref
      \
       \
        → D轴 PI → vd
       /
id


iq_ref
      \
       \
        → Q轴 PI → vq
       /
iq
```

也就是：

```
Id_ref - Id
      ↓
Error_d
      ↓
PI_d
      ↓
Vd


Iq_ref - Iq
      ↓
Error_q
      ↓
PI_q
      ↓
Vq
```

现在已经开始非常像以后真实 FOC 电流环。

------

# 八、第三阶段：Debug 挑错题

这是今天最重要的一部分。

我给你一些典型错误。

你先自己判断：

> 编译错误？

还是：

> 程序能运行，但逻辑错误？

------

# Bug 1

```
PI_Run(pi_d,
       id_ref,
       id_feedback);
```

原本应该：

```
PI_Run(&pi_d,
       id_ref,
       id_feedback);
```

问题在哪里？

答案：

> `PI_Run()` 要的是 `PI_Controller_t *`，即结构体地址，而 `pi_d` 是结构体本身。

属于：

> 类型/编译错误。

------

# Bug 2

函数里：

```
pi.integral += error * pi.ts;
```

但：

```
pi
```

定义是：

```
PI_Controller_t *pi
```

问题？

应该：

```
pi->integral
pi->ts
```

这是：

> 把结构体指针误当成结构体对象。

------

# Bug 3

```
float iq_feedback[3];

iq_feedback[3] = 2.0f;
```

问题？

数组：

```
[3]
```

合法的是：

```
0
1
2
```

所以：

```
iq_feedback[3]
```

已经越界。

这种最危险，因为：

> 有时还能编译运行，但可能产生不可预测错误。

------

# Bug 4

```
void PI_Run(PI_Controller_t *pi,
            float ref,
            float feedback)
{
    float integral = 0.0f;

    integral += error;
}
```

如果目标是实现连续 PI 积分，这里有什么逻辑问题？

因为：

```
integral
```

每次进入函数都重新：

```
= 0
```

不能保存历史。

更合理：

```
pi->integral
```

存进结构体。

------

# Bug 5

```
const float current_limit = 5.0f;

current_limit = 10.0f;
```

问题？

`const` 不允许这样改。

------

# Bug 6

```
static float PI_Run(...);
```

然后想在：

```
main.c
```

直接调用：

```
PI_Run(...);
```

为什么不合适？

因为文件级：

```
static
```

意味着：

> 这个函数只在当前 `.c` 内可见。

------

# Bug 7

`motor_control.h`：

```
float motor_speed = 0.0f;
```

然后很多 `.c` 都 include 这个头文件。

这里以后容易有什么问题？

因为头文件里这样写相当于可能：

> 在多个源文件中重复定义变量。

更典型的做法是：

### `motor_control.c`

```
float motor_speed = 0.0f;
```

### `motor_control.h`

```
extern float motor_speed;
```

当然更好的工程设计有时甚至不直接暴露变量，而是通过函数接口访问。

------

# Bug 8

```
#define CURRENT_LIMIT_A 5.0f
```

代码某处却直接：

```
if (iq_ref > 12.7f)
```

你应该产生一个工程上的警觉：

> `12.7f` 是什么？

这就是所谓：

> Magic Number，魔法数字。

如果它真是限流值，更应该统一配置。

------

# 九、第四阶段：一个综合修改任务

今天建议你做这个。

给原来的：

```
Motor_Control_Run()
```

加电流参考限幅。

例如增加内部函数：

```
static float Current_Ref_Limit(float current_ref)
{
    if (current_ref > CURRENT_LIMIT_A)
    {
        current_ref = CURRENT_LIMIT_A;
    }

    if (current_ref < -CURRENT_LIMIT_A)
    {
        current_ref = -CURRENT_LIMIT_A;
    }

    return current_ref;
}
```

然后：

```
void Motor_Control_Run(...)
{
    iq_ref =
        Current_Ref_Limit(iq_ref);

    ...
}
```

------

# 十、测试三种情况

### Case 1

```
Iq_ref = 2 A
```

结果：

```
2 A
```

------

### Case 2

```
Iq_ref = 8 A
```

结果应该：

```
5 A
```

------

### Case 3

```
Iq_ref = -8 A
```

结果应该：

```
-5 A
```

这个任务可以同时复习：

```
#define
static
函数
if
float
模块内部函数
```

非常适合 Day 7。

------

# 十一、第五阶段：看一眼未来真正的 FOC 框架

今天最后可以看这段，不需要实现：

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

现在你虽然还不会：

```
Current_Read
Clarke
Park
InvPark
SVPWM
PWM
```

但 C 语言结构已经基本不陌生了。

你应该看得出：

```
函数调用
↓
数据输入
↓
结构体地址
↓
控制器执行
↓
输出
```

这说明 Week 1 的目的已经实现。

------

# 十二、Day 7 不要继续学习这些

今天不要因为看到 FOC 框架就跳去学：

- Clarke
- Park
- SVPWM
- PWM
- ADC
- DMA
- Timer
- 中断
- STM32 HAL
- 电流 PI 参数整定

明天/Week 2 开始自然会进入 STM32。

今天只验收：

> **C 语言是否已经足够支撑你进入 STM32。**

------

# 十三、今天做一次 Git 收尾

你的原 Week 1 计划要求建立 GitHub 仓库并完成 Commit/Push。

你现在已经有仓库了，所以今天建议把 Week 1 C 代码整理一下：

```
Firmware/
└── C_Learning/
    ├── Day01_BasicTypes/
    ├── Day02_ArrayPointer/
    ├── Day03_StructTypedef/
    ├── Day04_EmbeddedKeywords/
    ├── Day05_Module/
    ├── Day06_CurrentLoop/
    └── Day07_Review/
```

不一定必须完全照这个目录，但思路类似即可。

------

# 十四、今天的 Git 流程

先：

```
git status
```

看哪些文件改过。

然后：

```
git add -A
```

然后：

```
git commit -m "Complete Week 1 embedded C fundamentals"
```

最后：

```
git push
```

这个 Commit 很适合作为：

> **第一周 C 基础正式完成节点。**

------

# 十五、今天顺便看一次 git log

虽然你计划正式学习 `git log` 是 Week 3，Week 1 并不要求，但现在你已经用过命令行了，可以简单运行：

```
git log --oneline
```

不用深入。

你只要看看：

```
abc123 Complete Week 1 embedded C fundamentals
def456 Add basic PI controller module
...
```

开始感受：

> 我的项目确实在一点点形成历史。

就够了。

------

# 十六、Day 7 第一周复盘

你原半年计划本身就要求每周进行复盘，包括掌握内容、AI 辅助代码、实机实验、问题、Git Commit、稳定版本等。

第一周可以简化写成：

```
# Week 1 Review

## 1. 本周掌握

- 基本数据类型
- 数组
- 一级指针
- struct
- typedef
- static
- const
- volatile
- extern
- #define
- .c / .h
- 函数声明与定义
- 简单模块化

## 2. 目前还只是知道

- 复杂指针
- 编译链接过程
- 真实 STM32 中断
- HAL
- DMA

## 3. AI 辅助

- 示例代码
- C语法解释
- PI模块示例
- Debug解释

## 4. 我能解释的关键代码

- PI_Controller_t
- PI_Run(&pi_q,...)
- pi->integral
- volatile adc_raw
- static内部函数

## 5. 本周 Git

- Commit：
  Complete Week 1 embedded C fundamentals

## 6. 当前稳定版本

- Week 1 embedded C complete

## 7. 下周重点

- STM32工程流程
- GPIO
- UART
- Timer
- Interrupt
- Debugger
```

不用写成长篇周报。

------

# 十七、第一周最终验收标准

如果今天结束以后，下面这些你基本都能做到：

- 能判断 `uint8_t / uint16_t / uint32_t / float` 为什么这样选；
- 看懂简单数组；
- 看懂一级指针；
- 明白 `&` 和 `*`；
- 看懂 `struct + typedef`；
- 区分 `.` 和 `->`；
- 能解释 `static / const / volatile / extern`；
- 知道 `#define` 是什么；
- 能理解 `.h/.c` 模块化；
- 能跟踪一次 `PI_Run()`；
- 能找到数组越界、忘记 `&`、`.`/`->` 用错等明显错误；
- 会做简单修改并重新运行；
- 会 `git status / add / commit / push`。

那么：

> **你的 Week 1 已经合格，而且已经足够进入 STM32 实战。**

不要因为不会：

```
二级指针
malloc
链表
复杂宏
链接器
Makefile
```

就觉得 C 还没学完。

你这半年路线的优先级本来就是“实机 Debug > FOC 核心原理 > STM32 实时控制 > Embedded C”，而不是把 C 本身学成主业。半年学习计划.mdMD

------

# Day 7 最关键的一句话

今天你不要问自己：

> “我会不会写 C？”

而是问：

> **“如果 Week 2 开始出现 GPIO、UART、Timer、中断代码，我会不会因为最基础的 C 语法看不懂而卡住？”**

如果答案已经基本是：

> **不会。**

那第一周就结束。

下一周就应该直接进入真正的 STM32G431 开发环境。