好，Day 3 我们就正式进入你第一周里**最值得学的一天之一：`struct + typedef`**。

因为从这一天开始，你看到的代码会明显更像以后真正的电机控制工程，而不是零散的 C 语法。

你的原计划 Week 1 本来就要求掌握 `struct`、`typedef`，并且专门通过 `PI_Controller_t` 这个结构体来练习结构体、指针和模块化。 半年学习计划.mdMD

今天我们继续沿用前两天的方式：

> **不要求你从零默写，直接看贴近 PMSM/FOC 工程的代码；重点做到看懂、会改、能解释。**

------

# Day 3 最终目标

今天结束以后，你看到：

```
typedef struct
{
    float kp;
    float ki;
    float integral;
    float output;

} PI_Controller_t;
```

你应该能直接理解：

> 这是定义了一种“PI 控制器数据类型”，里面统一存放这个 PI 控制器需要的参数和运行状态。

看到：

```
PI_Controller_t pi_d;
PI_Controller_t pi_q;
```

你应该知道：

> 创建了两个相互独立的 PI 控制器，一个可以给 d 轴，一个可以给 q 轴。

看到：

```
pi_d.kp = 1.0f;
```

你应该知道：

> 修改 `pi_d` 这个 PI 控制器里的 `kp`。

看到：

```
pi->integral = 0.0f;
```

你应该知道：

> `pi` 是一个指向 PI 结构体的指针，通过 `->` 修改这个 PI 对象内部的积分值。

Day 3 真正要打通的就是这几个关系：

```
struct
typedef
对象
.
结构体指针
->
```

------

# 一、为什么电机控制工程里必须学 struct

先假设没有结构体。

以后你有三个 PI：

```
Id PI
Iq PI
Speed PI
```

如果全部写成普通变量，你可能得到：

```
float id_kp;
float id_ki;
float id_integral;
float id_output;

float iq_kp;
float iq_ki;
float iq_integral;
float iq_output;

float speed_kp;
float speed_ki;
float speed_integral;
float speed_output;
```

现在还勉强能看。

但以后再加：

```
限幅
采样时间
误差
参考值
反馈值
Anti-Windup
```

马上会变成：

```
float id_output_max;
float id_output_min;

float iq_output_max;
float iq_output_min;

float speed_output_max;
float speed_output_min;

...
```

非常乱。

所以工程里更希望变成：

```
pi_d
 ├─ kp
 ├─ ki
 ├─ integral
 ├─ output
 ├─ output_max
 └─ output_min

pi_q
 ├─ kp
 ├─ ki
 ├─ integral
 ├─ output
 ├─ output_max
 └─ output_min
```

这就是 `struct` 的价值：

> **把属于同一个对象的数据打包在一起。**

------

# 二、Day 3 第一份完整程序

今天建议你直接运行这一份：

```
#include <stdio.h>


/* =========================
 * PI 控制器结构体
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
 * FOC 电流数据结构体
 * ========================= */

typedef struct
{
    float ia;
    float ib;
    float ic;

    float i_alpha;
    float i_beta;

    float id;
    float iq;

} FOC_Current_t;


/* =========================
 * 电机运行状态结构体
 * ========================= */

typedef struct
{
    float speed_rpm;
    float electrical_angle;

    float vbus;

    unsigned char enable;

} Motor_State_t;


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
 * 复位 PI
 * ========================= */

void PI_Reset(PI_Controller_t *pi)
{
    pi->integral = 0.0f;
    pi->output = 0.0f;
}


/* =========================
 * 打印 PI 参数
 * ========================= */

void PI_Print(PI_Controller_t *pi)
{
    printf("Kp       = %.2f\n", pi->kp);
    printf("Ki       = %.2f\n", pi->ki);
    printf("Integral = %.2f\n", pi->integral);
    printf("Output   = %.2f\n", pi->output);
}


/* =========================
 * 主程序
 * ========================= */

int main(void)
{
    PI_Controller_t pi_d;
    PI_Controller_t pi_q;

    FOC_Current_t current;

    Motor_State_t motor;


    /* 初始化 d 轴 PI */
    PI_Init(&pi_d,
            1.0f,
            20.0f,
           -10.0f,
            10.0f);


    /* 初始化 q 轴 PI */
    PI_Init(&pi_q,
            1.2f,
            25.0f,
           -10.0f,
            10.0f);


    /* 设置 FOC 电流 */
    current.ia = 1.2f;
    current.ib = -0.5f;
    current.ic = -0.7f;

    current.id = 0.1f;
    current.iq = 2.0f;


    /* 设置电机状态 */
    motor.speed_rpm = 1000.0f;
    motor.electrical_angle = 1.57f;
    motor.vbus = 48.0f;
    motor.enable = 1;


    printf("===== D-axis PI =====\n");
    PI_Print(&pi_d);


    printf("\n===== Q-axis PI =====\n");
    PI_Print(&pi_q);


    printf("\n===== FOC Current =====\n");

    printf("Ia = %.2f A\n", current.ia);
    printf("Ib = %.2f A\n", current.ib);
    printf("Ic = %.2f A\n", current.ic);

    printf("Id = %.2f A\n", current.id);
    printf("Iq = %.2f A\n", current.iq);


    printf("\n===== Motor State =====\n");

    printf("Speed = %.2f rpm\n", motor.speed_rpm);
    printf("Angle = %.2f rad\n", motor.electrical_angle);
    printf("Vbus  = %.2f V\n", motor.vbus);
    printf("Enable= %d\n", motor.enable);


    return 0;
}
```

今天你就围绕这份代码学习。

------

# 三、先理解最核心的 `struct`

最简单的例子：

```
struct Motor
{
    float speed;
    float current;
};
```

意思不是定义两个变量。

而是在定义：

> 一种新的“数据组合方式”。

你可以把它理解成：

```
Motor
│
├─ speed
└─ current
```

然后才能创建：

```
struct Motor motor1;
```

这时候：

```
motor1
│
├─ speed
└─ current
```

才是真正的数据对象。

------

# 四、那 typedef 是干什么的

如果不用 `typedef`，可能要写：

```
struct PI_Controller
{
    float kp;
    float ki;
};

struct PI_Controller pi_d;
```

每次创建变量都要写：

```
struct PI_Controller
```

比较啰嗦。

所以工程中很常见：

```
typedef struct
{
    float kp;
    float ki;

} PI_Controller_t;
```

然后以后直接：

```
PI_Controller_t pi_d;
PI_Controller_t pi_q;
```

非常干净。

------

# 五、`PI_Controller_t` 到底是什么

这一点非常重要。

```
PI_Controller_t
```

不是变量。

它是：

> **一种数据类型。**

就像：

```
float
uint16_t
uint32_t
```

一样。

所以：

```
float speed;
```

表示：

> 创建一个 float 类型变量 speed。

而：

```
PI_Controller_t pi_d;
```

表示：

> 创建一个 PI_Controller_t 类型变量 pi_d。

因此你以后脑子里可以这样看：

```
float
        → 系统自带的数据类型

uint16_t
        → 标准整数数据类型

PI_Controller_t
        → 我们自己定义的数据类型

FOC_Current_t
        → 我们自己定义的数据类型

Motor_State_t
        → 我们自己定义的数据类型
```

这就是 `typedef struct` 最大的价值之一。

------

# 六、为什么很多结构体名字后面有 `_t`

比如：

```
PI_Controller_t
FOC_Current_t
Motor_State_t
```

这里的：

```
_t
```

通常就是在告诉你：

> 这是一个 type，数据类型。

这不是 C 语言强制要求。

只是非常常见的命名习惯。

所以以后你看到：

```
Motor_t
FOC_t
PID_t
Encoder_t
ADC_t
```

你可以优先猜：

> 这大概率是某个结构体类型。

------

# 七、创建两个 PI 到底发生了什么

代码：

```
PI_Controller_t pi_d;
PI_Controller_t pi_q;
```

意味着：

```
pi_d
│
├─ kp
├─ ki
├─ integral
├─ output_min
├─ output_max
└─ output


pi_q
│
├─ kp
├─ ki
├─ integral
├─ output_min
├─ output_max
└─ output
```

虽然它们结构一样，

但是：

> **数据彼此完全独立。**

比如：

```
pi_d.kp = 1.0f;
pi_q.kp = 1.2f;
```

不会互相影响。

这特别适合：

```
Id PI

Iq PI

Speed PI
```

因为它们本质上都是 PI 控制器，

但参数不同、状态不同。

------

# 八、`.` 是什么

如果你有：

```
PI_Controller_t pi_d;
```

那么访问它内部的数据：

```
pi_d.kp
pi_d.ki
pi_d.integral
pi_d.output
```

这个：

```
.
```

可以简单理解为：

> 访问这个结构体对象内部的成员。

例如：

```
pi_d.kp = 1.0f;
```

就是：

> 把 `pi_d` 里面的 `kp` 设置成 1.0。

所以：

```
pi_d.kp
```

你直接读成人话：

> d 轴 PI 的 Kp。

------

# 九、这个写法特别适合 FOC

例如：

```
FOC_Current_t current;
```

内部：

```
typedef struct
{
    float ia;
    float ib;
    float ic;

    float i_alpha;
    float i_beta;

    float id;
    float iq;

} FOC_Current_t;
```

于是你以后就可以写：

```
current.ia
current.ib
current.ic

current.i_alpha
current.i_beta

current.id
current.iq
```

是不是比：

```
float ia;
float ib;
float ic;
float i_alpha;
float i_beta;
float id;
float iq;
```

看起来更有组织？

尤其以后项目大起来以后非常明显。

------

# 十、你以后甚至可能看到整个 FOC 对象

比如未来可能设计：

```
typedef struct
{
    FOC_Current_t current;

    PI_Controller_t pi_d;
    PI_Controller_t pi_q;

    float vd;
    float vq;

    float electrical_angle;

} FOC_t;
```

然后：

```
FOC_t foc;
```

访问：

```
foc.current.id

foc.current.iq

foc.pi_d.kp

foc.pi_q.kp

foc.vd

foc.vq
```

这就是以后比较工程化的写法。

Day 3 先让你看到这个趋势就行。

------

# 十一、现在把 Day 2 的指针接进来

我们写：

```
void PI_Reset(PI_Controller_t *pi)
```

这里：

```
PI_Controller_t *pi
```

是什么意思？

和昨天：

```
float *value
```

完全一样。

只是昨天：

```
指向 float
```

今天：

```
指向 PI_Controller_t
```

也就是：

> `pi` 是一个指向 PI 控制器结构体的指针。

------

# 十二、为什么调用时要写 `&pi_d`

调用：

```
PI_Reset(&pi_d);
```

昨天你已经学过：

```
&
=
取地址
```

所以：

```
&pi_d
```

就是：

> `pi_d` 这个结构体对象的地址。

函数：

```
void PI_Reset(PI_Controller_t *pi)
```

需要：

> PI 结构体地址。

所以刚好匹配。

------

# 十三、`.` 和 `->` 是 Day 3 最重要的区别

如果你手里直接拿着一个结构体：

```
PI_Controller_t pi_d;
```

使用：

```
pi_d.kp
```

也就是：

```
对象
.
成员
```

------

如果你手里拿的是结构体指针：

```
PI_Controller_t *pi;
```

使用：

```
pi->kp
```

也就是：

```
结构体指针
->
成员
```

这是今天最重要的一条。

------

# 十四、为什么函数里面写 `pi->kp`

看：

```
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
```

因为这里：

```
pi
```

不是一个结构体对象。

它是：

> 一个指向结构体的指针。

所以用：

```
pi->kp
```

而不是：

```
pi.kp
```

------

# 十五、其实 `->` 本质上是什么

你今天不用背，但理解一下特别有帮助。

这两个基本等价：

```
pi->kp
```

和：

```
(*pi).kp
```

为什么？

因为：

```
pi
 ↓
结构体地址

*pi
 ↓
地址对应的结构体本身

(*pi).kp
 ↓
结构体里面的 kp
```

但是：

```
(*pi).kp
```

太麻烦。

所以 C 给了一个方便写法：

```
pi->kp
```

因此：

> **`->` 可以理解成“通过结构体指针访问成员”。**

这就够了。

------

# 十六、把 `.` 和 `->` 彻底区分

你只需要记这张表：

| 你手里是什么 | 用什么 |
| ------------ | ------ |
| 结构体对象   | `.`    |
| 结构体指针   | `->`   |

例如：

```
PI_Controller_t pi_d;
```

用：

```
pi_d.kp
```

------

如果：

```
PI_Controller_t *pi = &pi_d;
```

用：

```
pi->kp
```

二者访问的是同一个东西。

------

# 十七、为什么 PI_Init 非常适合用结构体指针

我们希望：

```
PI_Init(&pi_d,
        1.0f,
        20.0f,
        -10.0f,
        10.0f);
```

执行以后直接把：

```
pi_d.kp
pi_d.ki
pi_d.integral
pi_d.output_min
pi_d.output_max
pi_d.output
```

全部初始化。

如果不用指针，你很容易需要返回结构体，或者复制来复制去。

而传地址：

```
&pi_d
```

以后函数直接操作真正的：

```
pi_d
```

这就是 Day 2 和 Day 3 开始真正结合起来的地方。

------

# 十八、Day 3 第一个修改实验：修改 d/q PI 参数

原来：

```
PI_Init(&pi_d,
        1.0f,
        20.0f,
        -10.0f,
        10.0f);
```

改成：

```
PI_Init(&pi_d,
        2.0f,
        30.0f,
        -10.0f,
        10.0f);
```

预测：

```
pi_d.kp = ?
pi_d.ki = ?
```

应该：

```
2
30
```

同时：

```
pi_q
```

完全不变。

这个实验就是让你体会：

> 两个相同类型的结构体对象，数据彼此独立。

------

# 十九、第二个实验：直接用 `.` 修改

在初始化以后写：

```
pi_d.kp = 5.0f;
```

那么：

```
PI_Print(&pi_d);
```

打印出的 Kp 应该就是：

```
5.0
```

而：

```
pi_q.kp
```

不会变化。

------

# 二十、第三个实验：人为修改积分值，再 Reset

加：

```
pi_d.integral = 100.0f;
pi_d.output = 8.0f;
```

打印一次：

```
PI_Print(&pi_d);
```

然后：

```
PI_Reset(&pi_d);
```

再打印：

```
PI_Print(&pi_d);
```

你应该看到：

```
Reset 前：

integral = 100
output = 8


Reset 后：

integral = 0
output = 0
```

但是：

```
kp
ki
output_max
output_min
```

不应该变。

为什么？

因为：

```
PI_Reset()
```

只写了：

```
pi->integral = 0.0f;
pi->output = 0.0f;
```

这就是以后 Debug 很重要的思维：

> **函数只会修改它代码里真正写到的东西。**

------

# 二十一、第四个实验：故意把 `->` 写成 `.`

比如把：

```
pi->kp = kp;
```

故意改成：

```
pi.kp = kp;
```

看看编译器怎么报错。

为什么？

因为：

```
pi
```

是：

```
PI_Controller_t *
```

也就是一个指针。

不是直接的：

```
PI_Controller_t
```

所以不能：

```
pi.kp
```

这次错误非常值得你亲自看一次。

以后见到类似编译错误你就知道：

> 我是不是把结构体和结构体指针搞混了。

------

# 二十二、第五个实验：故意把调用里的 `&` 去掉

原来：

```
PI_Reset(&pi_d);
```

改：

```
PI_Reset(pi_d);
```

再编译。

函数需要：

```
PI_Controller_t *pi
```

也就是：

> PI 结构体的地址。

但你给的是：

```
pi_d
```

也就是：

> PI 结构体本身。

所以类型不匹配。

这和昨天：

```
Set_Value(&iq_ref)
```

完全一样。

只是对象从：

```
float
```

升级成：

```
struct
```

------

# 二十三、第六个实验：创建第三个 PI

自己加：

```
PI_Controller_t pi_speed;
```

然后：

```
PI_Init(&pi_speed,
        0.05f,
        1.0f,
        -5.0f,
        5.0f);
```

打印：

```
PI_Print(&pi_speed);
```

这样你已经提前模拟：

```
Id PI

Iq PI

Speed PI
```

三个控制器。

以后 Week 13 真正做速度环时，你会发现思路还是一样。

------

# 二十四、今天再认识一个非常真实的电机结构体

比如：

```
typedef struct
{
    float rs;
    float ls;
    float psi_f;

    unsigned char pole_pairs;

} Motor_Parameter_t;
```

然后：

```
Motor_Parameter_t motor_param;
```

写：

```
motor_param.rs = 1.20f;
motor_param.ls = 0.008f;
motor_param.psi_f = 0.100f;
motor_param.pole_pairs = 4;
```

这是不是和你论文、Simulink 里的：

```
Rs
Ls
Ψf
Pole Pairs
```

非常像？

所以以后你完全可能把电机参数统一整理成：

```
motor_param
```

而不是散落很多变量。

------

# 二十五、甚至可以进一步组织整个 Motor Control

以后你可能看到类似：

```
typedef struct
{
    Motor_Parameter_t param;

    FOC_Current_t current;

    PI_Controller_t pi_d;
    PI_Controller_t pi_q;
    PI_Controller_t pi_speed;

    float speed_ref;

    float electrical_angle;

} Motor_Control_t;
```

创建：

```
Motor_Control_t motor;
```

然后：

```
motor.param.rs

motor.param.psi_f

motor.current.id

motor.current.iq

motor.pi_d.kp

motor.pi_q.ki

motor.pi_speed.output

motor.speed_ref
```

这就是结构体“套结构体”。

今天不要求你会设计这种架构。

但你至少应该开始感觉到：

> **为什么大型嵌入式工程离不开 struct。**

------

# 二十六、Day 3 你暂时不要深入什么

今天不要钻：

```
结构体内存对齐
padding
union
bit-field
匿名结构体
复杂嵌套
柔性数组
```

这些不是你现阶段主战场。

Day 3 只学：

```
struct
typedef
创建对象
.
结构体指针
->
```

足够。

------

# 二十七、今天的时间安排

建议 **2～2.5 小时**。

## 第 1 阶段：30 分钟

运行完整代码。

先只看：

```
PI_Controller_t
FOC_Current_t
Motor_State_t
```

把它们理解成：

> 自定义数据类型。

------

## 第 2 阶段：30 分钟

只研究：

```
PI_Controller_t pi_d;

pi_d.kp
pi_d.ki
```

彻底理解：

```
对象
.
成员
```

------

## 第 3 阶段：40 分钟

研究：

```
PI_Init(&pi_d, ...);
```

重点看：

```
&pi_d

PI_Controller_t *pi

pi->kp
```

把 Day 2 的指针和 Day 3 的结构体连起来。

------

## 第 4 阶段：30 分钟

完成几个修改实验：

- 改 Kp/Ki
- 改 integral
- Reset
- 增加 Speed PI

------

## 第 5 阶段：20 分钟

故意制造两个错误：

```
pi.kp
```

代替：

```
pi->kp
```

以及：

```
PI_Reset(pi_d);
```

代替：

```
PI_Reset(&pi_d);
```

观察编译错误。

------

# 二十八、Day 3 的核心验收题

今天结束以后，不查资料回答下面这些。

### 1

```
typedef struct
{
    float kp;
    float ki;

} PI_Controller_t;
```

这段代码是在创建一个变量，还是定义一种数据类型？

正确理解应该是：

> 定义一种数据类型。

------

### 2

```
PI_Controller_t pi_d;
```

是什么意思？

------

### 3

下面两个：

```
pi_d.kp
```

和：

```
pi_d.ki
```

分别是什么？

------

### 4

为什么：

```
PI_Controller_t pi_d;
PI_Controller_t pi_q;
```

可以拥有不同的 Kp？

------

### 5

```
PI_Controller_t *pi
```

是什么？

------

### 6

```
&pi_d
```

是什么？

------

### 7

什么时候用：

```
.
```

？

------

### 8

什么时候用：

```
->
```

？

------

### 9

为什么：

```
PI_Init(&pi_d, ...)
```

可以直接改变：

```
pi_d.kp
pi_d.ki
```

？

------

### 10

看到：

```
motor.current.iq
```

你能不能判断大概是什么意思？

如果这 10 个问题你能答出来 8 个以上，Day 3 已经完全达标。

------

# 二十九、Day 3 最值得记住的一张关系图

```
typedef struct
{
    float kp;
    float ki;
} PI_Controller_t;

          ↓

定义了一种类型

PI_Controller_t


          ↓

创建对象

PI_Controller_t pi_d;


          ↓

直接访问对象成员

pi_d.kp
pi_d.ki


          ↓

取得对象地址

&pi_d


          ↓

结构体指针

PI_Controller_t *pi


          ↓

通过指针访问成员

pi->kp
pi->ki
```

你只要把这条链弄清楚，今天就成功了。

------

# 三十、Day 3 对你以后最重要的实际意义

你以后真正写 PMSM FOC 时，大概率不会是几十个毫无组织的：

```
float ...
float ...
float ...
float ...
```

而是逐渐形成：

```
Motor Parameters
        ↓
Motor_Parameter_t

Current Data
        ↓
FOC_Current_t

PI Controller
        ↓
PI_Controller_t

Encoder
        ↓
Encoder_t

Motor State
        ↓
Motor_State_t

FOC
        ↓
FOC_t
```

这就是从：

> “会写几个 C 变量”

开始走向：

> **“能看懂真正的嵌入式电机控制工程代码”。**

而且你的 Week 1 原计划本身就是希望借 `PI_Controller_t` 这个练习，真正理解结构体、指针、模块化和 `.c/.h`，并不是为了研究 PI 算法本身。半年学习计划.mdMD

**Day 3 今天做到这里就够，不要继续加新知识。**

Day 4 我们就会进入嵌入式 C 非常核心的：

**`static + const + volatile + extern + #define`**

那一天开始，你会真正理解为什么 STM32 工程里经常出现 `volatile uint16_t adc_value`、`static float integral`、`extern Motor_t motor` 这类代码。