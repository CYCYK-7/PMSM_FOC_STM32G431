好，Day 5 进入第一周里非常关键的一步：

> **`.h + .c + 函数声明与定义 + 模块化`**

你前 4 天其实已经把最常见的 C 基础扫了一遍：变量、数组、指针、`struct`、`typedef`、`static`、`volatile`、`const`、`extern`。Day 5 的目的，就是把这些东西第一次真正组织成一个“像工程的代码结构”。

你的原计划 Week 1 也明确要求掌握 `.c/.h`、函数声明与定义，并通过 `PI_Controller_t + PI_Init / PI_Run / PI_Reset` 来练习模块化，而不是为了研究 PI 算法本身。 

今天继续保持我们的路线：

> **不要求你从零写。直接给你一套能运行、而且以后 STM32 FOC 工程里非常常见的模块结构。你重点学会看懂“为什么分文件、文件之间怎么联系”。**

------

# Day 5 最终目标

今天结束以后，你看到：

```
pi.h
pi.c
main.c
```

应该知道三者大概是什么关系。

看到：

```
float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback);
```

应该知道这是：

> **函数声明**

看到：

```
float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback)
{
    ...
}
```

应该知道这是：

> **函数定义 / 真正实现**

看到：

```
#include "pi.h"
```

应该知道：

> 当前文件要使用 `pi.h` 里公开出来的类型和函数接口。

最终脑子里建立这一层：

```
main.c
  ↓ 调用

pi.h
  ↓ 告诉外部“这个模块能提供什么”

pi.c
  ↓ 真正实现这些功能
```

这就是 Day 5 的核心。

------

# 一、为什么不能把所有东西都塞进 main.c

刚开始学 C，很容易写成：

```
int main(void)
{
    // ADC
    // PWM
    // PI
    // Clarke
    // Park
    // SVPWM
    // Encoder
    // Fault
    // UART
}
```

几十行的时候没问题。

以后几千行之后基本灾难。

真实电机控制项目会逐渐出现：

```
main.c

adc.c
adc.h

pwm.c
pwm.h

pi.c
pi.h

transform.c
transform.h

svpwm.c
svpwm.h

encoder.c
encoder.h

foc.c
foc.h
```

你原学习计划到 Week 8 甚至已经规划了类似的工程目录：

```
Control/
├── foc.c
├── foc.h
├── pi.c
├── pi.h
├── transform.c
├── transform.h
├── svpwm.c
└── svpwm.h
```

所以 Day 5 不是学形式主义。

它直接关系到：

> **以后你能不能看懂一个真实 STM32 电机控制工程。**

------

# 二、今天建立这个目录

先不用搞复杂。

建立：

```
Day5_PI_Module/

├── main.c
├── pi.c
└── pi.h
```

今天所有内容就围绕这三个文件。

------

# 三、先给你完整的 `pi.h`

```
#ifndef PI_H
#define PI_H


/* =========================
 * PI 控制器数据类型
 * ========================= */

typedef struct
{
    float kp;
    float ki;

    float integral;

    float output_max;
    float output_min;

    float output;

} PI_Controller_t;


/* =========================
 * PI 模块公开函数
 * ========================= */

void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float output_min,
             float output_max);

float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback);

void PI_Reset(PI_Controller_t *pi);


#endif
```

这就是今天非常典型的：

> **头文件。**

------

# 四、再给你完整的 `pi.c`

```
#include "pi.h"


/* =========================
 * 初始化 PI
 * ========================= */

void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float output_min,
             float output_max)
{
    pi->kp = kp;
    pi->ki = ki;

    pi->integral = 0.0f;

    pi->output_min = output_min;
    pi->output_max = output_max;

    pi->output = 0.0f;
}


/* =========================
 * 运行一次 PI
 * ========================= */

float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback)
{
    float error;

    error = ref - feedback;


    /* 积分 */
    pi->integral += error;


    /* PI 输出 */
    pi->output =
        pi->kp * error
        +
        pi->ki * pi->integral;


    /* 输出上限 */
    if (pi->output > pi->output_max)
    {
        pi->output = pi->output_max;
    }


    /* 输出下限 */
    if (pi->output < pi->output_min)
    {
        pi->output = pi->output_min;
    }


    return pi->output;
}


/* =========================
 * 复位 PI
 * ========================= */

void PI_Reset(PI_Controller_t *pi)
{
    pi->integral = 0.0f;
    pi->output = 0.0f;
}
```

------

# 五、最后是 `main.c`

```
#include <stdio.h>

#include "pi.h"


int main(void)
{
    PI_Controller_t pi_d;
    PI_Controller_t pi_q;


    float id_ref = 0.0f;
    float id = 0.2f;


    float iq_ref = 2.0f;
    float iq = 0.5f;


    float vd;
    float vq;


    /* =========================
     * 初始化两个 PI
     * ========================= */

    PI_Init(&pi_d,
            1.0f,
            0.1f,
           -10.0f,
            10.0f);


    PI_Init(&pi_q,
            1.2f,
            0.1f,
           -10.0f,
            10.0f);


    /* =========================
     * 模拟一次电流环运行
     * ========================= */

    vd = PI_Run(&pi_d,
                id_ref,
                id);


    vq = PI_Run(&pi_q,
                iq_ref,
                iq);


    printf("===== Current Loop =====\n");

    printf("Id Ref = %.2f A\n", id_ref);
    printf("Id     = %.2f A\n", id);
    printf("Vd     = %.2f V\n", vd);

    printf("\n");

    printf("Iq Ref = %.2f A\n", iq_ref);
    printf("Iq     = %.2f A\n", iq);
    printf("Vq     = %.2f V\n", vq);


    return 0;
}
```

今天就用这三个文件。

------

# 六、先搞懂 `.h` 文件到底干什么

很多初学者会把 `.h` 理解成：

> “就是放一些代码的文件。”

不够准确。

你现在先把 `.h` 理解成：

> **模块对外公开的说明书 / 接口。**

比如 `pi.h`：

```
typedef struct
{
    ...
} PI_Controller_t;
```

告诉外部：

> 我这个 PI 模块有一种 `PI_Controller_t` 数据类型。

然后：

```
void PI_Init(...);

float PI_Run(...);

void PI_Reset(...);
```

告诉外部：

> 这个模块提供三个函数。

所以别人只要：

```
#include "pi.h"
```

就知道：

```
PI_Controller_t 是什么

PI_Init 可以调用

PI_Run 可以调用

PI_Reset 可以调用
```

------

# 七、`.c` 又是干什么

`pi.c`：

```
#include "pi.h"
```

然后：

```
void PI_Init(...)
{
    ...
}
float PI_Run(...)
{
    ...
}
```

这里是真正：

> **实现功能。**

所以可以粗略理解：

```
pi.h
=
“我能干什么”


pi.c
=
“我是怎么干的”
```

这个思想以后非常重要。

------

# 八、用电机工程来理解

假设未来：

```
encoder.h
encoder.c
```

那么：

### `encoder.h`

可能告诉别人：

```
float Encoder_GetSpeed(void);

float Encoder_GetElectricalAngle(void);
```

别人知道：

> 我可以获取速度和电角度。

------

### `encoder.c`

才真正负责：

```
读取定时器计数
↓
处理溢出
↓
算机械角
↓
乘极对数
↓
算电角度
↓
返回结果
```

调用者通常没必要知道所有细节。

这就是模块化。

------

# 九、什么叫“函数声明”

看 `pi.h`：

```
float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback);
```

注意最后：

```
;
```

而且没有：

```
{
    ...
}
```

这叫：

> **函数声明。**

意思大概是告诉编译器：

> 有一个叫 `PI_Run` 的函数，它接收这些参数，返回一个 `float`。

但是这里没有告诉你它内部怎么算。

------

# 十、什么叫“函数定义”

看 `pi.c`：

```
float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback)
{
    float error;

    error = ref - feedback;

    ...

    return pi->output;
}
```

这里有完整：

```
{
    ...
}
```

这叫：

> **函数定义 / 函数实现。**

它真正告诉程序：

> `PI_Run()` 到底要做什么。

------

# 十一、声明 vs 定义

记这个就够：

| 内容     | 作用                             |
| -------- | -------------------------------- |
| 函数声明 | 告诉别人“这个函数存在、怎么调用” |
| 函数定义 | 真正写函数内部怎么执行           |

比如：

```
float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback);
```

只是：

> 宣布有 PI_Run。

而：

```
float PI_Run(...)
{
    ...
}
```

才是真正做 PI。

------

# 十二、为什么 main.c 只需要 include pi.h

看：

```
#include "pi.h"
```

之后：

```
PI_Controller_t pi_d;
```

能认识。

然后：

```
PI_Init(...);
```

能认识。

然后：

```
PI_Run(...);
```

也能认识。

但 `main.c` 根本不需要直接：

```
#include "pi.c"
```

这一点很重要。

正常模块化写法是：

```
main.c
↓
include
↓
pi.h
```

然后编译系统会把：

```
main.c
pi.c
```

一起编译链接。

------

# 十三、为什么通常不要 `#include "pi.c"`

以后如果你看到：

```
#include "pi.c"
```

一般应该提高警惕。

正常习惯是：

```
#include "pi.h"
```

`.c` 文件作为独立源文件参与编译。

所以你以后记：

> **通常 include 头文件，不直接 include `.c` 文件。**

------

# 十四、现在解释 `#ifndef / #define / #endif`

你今天会第一次看到：

```
#ifndef PI_H
#define PI_H

...

#endif
```

这个现在不用深入预处理器。

你只需要知道它的主要目的：

> **防止同一个头文件被重复包含。**

假设：

```
main.c
 ↓
include motor.h
 ↓
motor.h 又 include pi.h
```

同时：

```
main.c
 ↓
自己也 include pi.h
```

那么 `pi.h` 可能被重复读入。

所以：

```
#ifndef PI_H
#define PI_H
```

大概意思：

> 如果 `PI_H` 还没有定义，那就执行下面这些内容。

然后：

```
#define PI_H
```

相当于做一个标记：

> PI_H 已经处理过了。

最后：

```
#endif
```

结束。

------

# 十五、你可以直接把它理解成“头文件保护”

以后看到：

```
#ifndef MOTOR_H
#define MOTOR_H

...

#endif
```

或者：

```
#ifndef FOC_H
#define FOC_H

...

#endif
```

你就知道：

> 防止头文件被重复 include。

不需要自己发明。

很多 IDE 会自动生成。

------

# 十六、为什么结构体一般放在 `.h`

我们现在：

```
typedef struct
{
    float kp;
    float ki;
    ...
} PI_Controller_t;
```

放在：

```
pi.h
```

为什么？

因为：

```
main.c
```

也需要创建：

```
PI_Controller_t pi_d;
```

所以 `main.c` 必须知道：

> `PI_Controller_t` 到底是什么。

因此我们把这个公开数据类型放进：

```
pi.h
```

这样：

```
#include "pi.h"
```

以后就认识它了。

------

# 十七、为什么 PI_Run 的内部细节放 pi.c

`main.c` 只需要知道：

```
vq = PI_Run(&pi_q,
            iq_ref,
            iq);
```

实际上并不需要知道 PI 内部：

```
error = ref - feedback;

pi->integral += error;

pi->output =
    pi->kp * error +
    pi->ki * pi->integral;
```

这些细节。

这就是：

> **把实现细节隐藏在模块内部。**

以后特别重要。

------

# 十八、FOC 以后就是这样逐层调用

未来可能是：

```
void FOC_Run(void)
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

而每一个：

```
Current_Read
Clarke_Run
Park_Run
PI_Run
InvPark_Run
SVPWM_Run
PWM_Update
```

背后都可能来自不同：

```
.c / .h
```

模块。

这就是为什么今天必须搞懂模块化。

------

# 十九、今天顺便理解“调用者不需要知道全部内部细节”

你以后可能在 `foc.c` 里只看到：

```
Park_Run(...);
```

你可能不知道 Park 里面完整代码。

没关系。

如果你现在只是分析 FOC 框架：

> 知道这一步在做 αβ → dq 坐标变换就够了。

真的出现：

```
Park结果错误
```

再进去：

```
transform.c
```

检查实现。

这就是工程调试的思维。

------

# 二十、现在看 main.c 为什么干净多了

如果没有模块化，可能：

```
int main(void)
{
    // 初始化PI几十行

    // 计算error
    // 积分
    // Kp
    // Ki
    // 限幅

    // 再写一份Iq PI

    ...
}
```

而现在：

```
PI_Init(&pi_d, ...);

PI_Init(&pi_q, ...);

vd = PI_Run(&pi_d,
            id_ref,
            id);

vq = PI_Run(&pi_q,
            iq_ref,
            iq);
```

一下就能看懂：

```
初始化 d轴 PI

初始化 q轴 PI

运行 d轴 PI

运行 q轴 PI
```

这就是：

> **抽象。**

------

# 二十一、Day 5 先不要纠结 PI 算法是否完善

现在这个：

```
pi->integral += error;
```

其实并不是以后真实数字 PI 最完整的写法。

通常至少还会涉及：

```
采样时间 Ts

Ki × Ts

积分限幅

Anti-Windup

输出饱和策略
```

但这些属于你计划里的 Week 6。半年学习计划.mdMD

所以今天：

> **不要因为 PI 代码不够工程完善而跑偏。**

Day 5 的主角不是 PI。

主角是：

```
.h
.c
声明
定义
#include
模块化
```

------

# 二十二、Day 5 实验 1：改 PI 参数

在：

```
PI_Init(&pi_d,
        1.0f,
        0.1f,
        -10.0f,
        10.0f);
```

把：

```
Kp = 1
```

改成：

```
Kp = 2
```

运行。

观察：

```
Vd
```

是不是发生变化。

重点不是 PI 调参。

是理解：

```
main.c
修改参数
 ↓
调用 PI_Run
 ↓
真正执行的是 pi.c 里的代码
```

------

# 二十三、实验 2：修改 pi.c，而 main.c 不动

找到：

```
pi->output =
    pi->kp * error
    +
    pi->ki * pi->integral;
```

临时改成：

```
pi->output =
    pi->kp * error;
```

也就是暂时不要积分项。

`main.c` 一行都不改。

重新编译运行。

你会看到输出改变。

这次实验特别值得做。

因为它证明：

> `main.c` 只负责调用接口，而算法实现真的在 `pi.c`。

------

# 二十四、实验 3：把 pi.h 的声明删掉

临时把：

```
float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback);
```

从：

```
pi.h
```

删除。

然后重新编译。

看看：

```
main.c
```

调用：

```
PI_Run(...)
```

时编译器会怎么提示。

目的：

> 让你体会为什么 `.h` 里必须有公开函数的声明。

测试完记得恢复。

------

# 二十五、实验 4：不 include pi.h

在 `main.c`：

```
#include "pi.h"
```

临时注释掉：

```
// #include "pi.h"
```

然后编译。

此时编译器会开始不认识：

```
PI_Controller_t
```

以及：

```
PI_Init
PI_Run
```

这能帮助你真正理解：

> `#include "pi.h"` 不是装饰。

它是让 `main.c` 获得 PI 模块公开接口。

------

# 二十六、实验 5：新增一个函数

现在在：

```
pi.h
```

增加：

```
float PI_GetOutput(PI_Controller_t *pi);
```

然后：

```
pi.c
```

增加：

```
float PI_GetOutput(PI_Controller_t *pi)
{
    return pi->output;
}
```

最后 `main.c`：

```
printf("PI Q Output = %.2f\n",
       PI_GetOutput(&pi_q));
```

这一次你不用从零设计。

直接照着做。

目的只有一个：

> 体验新增一个模块接口需要改哪些地方。

流程是：

```
pi.h
↓
声明

pi.c
↓
实现

main.c
↓
调用
```

这条链今天一定理解。

------

# 二十七、实验 6：把内部函数隐藏起来

这是 Day 4 的 `static` 和 Day 5 的模块化结合。

假设在：

```
pi.c
```

里面加：

```
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
```

然后在：

```
PI_Run()
```

里面：

```
pi->output =
    PI_Limit(pi->output,
             pi->output_min,
             pi->output_max);
```

但：

```
pi.h
```

里面**不声明** `PI_Limit()`。

为什么？

因为：

> `PI_Limit()` 是 PI 模块内部自己使用的函数，不希望其他模块直接调用。

所以：

```
static float PI_Limit(...)
```

可以理解成：

> PI 模块的“私有函数”。

这就是昨天学的：

```
static
```

开始真正落地。

------

# 二十八、这就是以后非常重要的 Public / Private 思维

你可以把 `pi.h` 里的东西理解成：

> **Public：给别人用。**

比如：

```
PI_Init();
PI_Run();
PI_Reset();
```

而 `pi.c` 里：

```
static float PI_Limit(...);
```

则属于：

> **Private：模块自己用。**

非常像：

```
外部世界

       ↓
┌─────────────────┐
│      pi.h       │
│                 │
│ PI_Init         │
│ PI_Run          │
│ PI_Reset        │
└─────────────────┘
       ↓
┌─────────────────┐
│      pi.c       │
│                 │
│ 真正算法实现     │
│                 │
│ PI_Limit        │
│ 内部状态         │
└─────────────────┘
```

这已经是很典型的软件工程思想了。

------

# 二十九、那 extern 在这种结构里怎么出现？

Day 4 学过：

```
extern
```

今天给你看真实多文件场景。

例如：

### `motor.c`

```
float motor_speed = 0.0f;
```

真正定义变量。

------

### `motor.h`

```
extern float motor_speed;
```

公开声明。

------

### `main.c`

```
#include "motor.h"

printf("%f", motor_speed);
```

就可以使用。

所以：

```
motor.c
↓
真正创建

motor.h
↓
extern 声明

main.c
↓
include motor.h 后使用
```

------

# 三十、但是不是所有变量都应该 extern？

不是。

这是非常重要的工程意识。

比如：

```
float pi_internal_temp;
```

如果只有：

```
pi.c
```

自己使用，

最好可能直接：

```
static float pi_internal_temp;
```

而不是：

```
extern
```

到处共享。

原则可以先理解成：

> **能不暴露就尽量不暴露。**

以后你的 FOC 工程越大，这条越重要。

------

# 三十一、你以后可能会看到一个完整模块长这样

例如：

```
Control/

├── pi.c
├── pi.h

├── transform.c
├── transform.h

├── svpwm.c
├── svpwm.h

└── foc.c
    foc.h
```

其中：

```
pi
↓
负责 PI


transform
↓
负责 Clarke / Park / InvPark


svpwm
↓
负责 Vαβ → Duty


foc
↓
负责把它们串起来
```

所以：

```
foc.c
```

可能大量：

```
#include "pi.h"
#include "transform.h"
#include "svpwm.h"
```

然后：

```
void FOC_Run(void)
{
    Clarke_Run(...);

    Park_Run(...);

    PI_Run(...);

    InvPark_Run(...);

    SVPWM_Run(...);
}
```

到时候你就不会被一堆 `.h/.c` 文件吓到。

------

# 三十二、Day 5 再给你一个以后非常真实的例子

未来可能有：

### `transform.h`

```
#ifndef TRANSFORM_H
#define TRANSFORM_H

void Clarke_Run(float ia,
                float ib,
                float *i_alpha,
                float *i_beta);

void Park_Run(float i_alpha,
              float i_beta,
              float theta,
              float *id,
              float *iq);

#endif
```

------

### `transform.c`

```
#include "transform.h"

void Clarke_Run(...)
{
    ...
}

void Park_Run(...)
{
    ...
}
```

------

### `foc.c`

```
#include "transform.h"

Clarke_Run(...);

Park_Run(...);
```

虽然你现在还没正式学 Clarke/Park，

但是文件组织方式已经完全能看懂了。

------

# 三十三、今天不要学太深的内容

Day 5 暂时不要展开：

```
链接器工作原理
编译器完整流程
Makefile
CMake
静态库
动态库
多重 include 深层原理
头文件依赖管理
前向声明
复杂宏
函数指针接口
```

以后真遇到再学。

你现在只要知道：

```
.c
.h
#include
函数声明
函数定义
模块公开接口
模块内部实现
```

就够。

------

# 三十四、Day 5 推荐时间安排

建议今天 **2～2.5 小时**。

## 第 1 阶段：30 分钟

建立：

```
main.c
pi.c
pi.h
```

把我给你的完整代码跑起来。

目标：

> 先确认三个文件能正确协同工作。

------

## 第 2 阶段：30 分钟

只看：

```
pi.h
```

搞懂：

```
typedef struct ...
```

和：

```
PI_Init(...);
PI_Run(...);
PI_Reset(...);
```

都是模块对外公开的东西。

------

## 第 3 阶段：30 分钟

对照：

```
pi.h
```

和：

```
pi.c
```

专门找：

```
声明
      VS
定义
```

例如：

```
float PI_Run(...);
```

对应：

```
float PI_Run(...)
{
    ...
}
```

------

## 第 4 阶段：30 分钟

研究：

```
#include "pi.h"
```

把它临时删掉一次。

再把：

```
PI_Run()
```

声明临时删掉一次。

观察报错。

------

## 第 5 阶段：30 分钟

做：

```
PI_GetOutput
```

或者：

```
static PI_Limit
```

其中一个扩展实验。

目的：

> 第一次真正体验“新增模块接口”和“内部私有函数”的区别。

------

# 三十五、Day 5 验收题

今天结束后，不查资料回答。

### 1

`.h` 文件主要干什么？

------

### 2

`.c` 文件主要干什么？

------

### 3

```
float PI_Run(...);
```

为什么最后有：

```
;
```

而没有函数体？

------

### 4

```
float PI_Run(...)
{
    ...
}
```

属于声明还是定义？

------

### 5

为什么：

```
main.c
```

通常：

```
#include "pi.h"
```

而不是：

```
#include "pi.c"
```

？

------

### 6

`pi.h` 里的：

```
PI_Controller_t
```

为什么 `main.c` 能使用？

------

### 7

```
#ifndef PI_H
#define PI_H
...
#endif
```

主要干什么？

------

### 8

为什么：

```
PI_Limit()
```

这种内部辅助函数可以只放在 `pi.c`，甚至加：

```
static
```

？

------

### 9

如果：

```
main.c
```

要调用：

```
PI_Run()
```

它首先需要在哪里看到函数声明？

------

### 10

看到：

```
adc.c / adc.h
pwm.c / pwm.h
encoder.c / encoder.h
```

你能不能大概理解：

> `.h` 是接口，`.c` 是实现？

如果 10 个里能答 8 个，Day 5 就已经达标。

------

# 三十六、Day 5 最应该记住的一张图

```
                外部调用

main.c
  │
  │ #include "pi.h"
  ↓

pi.h
  │
  ├── PI_Controller_t
  ├── PI_Init()
  ├── PI_Run()
  └── PI_Reset()
  │
  │ 告诉外面“我提供什么”
  ↓

pi.c
  │
  ├── PI_Init真正实现
  ├── PI_Run真正实现
  ├── PI_Reset真正实现
  └── static内部函数
       │
       ↓
   模块内部细节
```

一句话：

> **`.h` 管接口，`.c` 管实现。**

虽然真实 C 工程还有很多细节，但你第一阶段这样理解非常够用。

------

# 三十七、Day 5 对你以后最直接的价值

以后你打开 STM32CubeIDE / VSCode 的电机工程，看到：

```
Core/
Drivers/
Application/
BSP/
Control/
```

里面几十个 `.c/.h`，

你不应该再想：

> “为什么有这么多文件，我该从哪个开始看？”

你会开始有顺序：

```
先找 foc.c / motor.c
        ↓
看主控制流程
        ↓
发现 PI_Run
        ↓
去 pi.h 看接口
        ↓
需要看细节再进 pi.c
        ↓
发现 SVPWM_Run
        ↓
去 svpwm.h / svpwm.c
```

这其实是一种非常重要的：

> **代码阅读能力。**

而这正符合你“不是以默写代码为目标，而是要能看懂、会用、会 Debug”的路线。

------

## Day 5 今天做到哪里就停

今天最终要留下一个能编译运行的：

```
PI_Module/

├── main.c
├── pi.c
└── pi.h
```

并且你能清楚解释：

```
为什么有 pi.h
为什么有 pi.c
main.c 怎么找到 PI_Run
声明和定义有什么区别
为什么 PI_Limit 可以是 static
```

这几个问题搞明白，Day 5 就结束。

下一步 **Day 6** 就可以第一次把前 5 天的知识真正汇总起来：我们会把 PI 模块稍微做得更像电机控制工程，加入 `Ts`、输出限幅、积分状态，并用多次控制周期模拟 `Id/Iq` 电流环调用。这样你会第一次看到一个非常简化但结构已经接近真正 FOC 控制代码的“小工程”。