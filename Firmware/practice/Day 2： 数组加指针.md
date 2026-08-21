好，Day 2 就进入你第一周里非常关键的一块：

> **数组 + 指针**

而且我会继续按照我们 Day 1 确定的学习方式来，不让你花时间从零默写代码，而是直接给你**贴近以后 FOC / STM32 电机控制场景**的代码，然后重点训练：

> 看得懂、会改、知道为什么这么写、出问题知道往哪看。

你的半年计划里 Week 1 本身就明确要求掌握数组、指针，而且第一周验收只是“能看懂简单指针”，并不是让你去学复杂指针技巧。

所以今天我们严格控制范围：

**只学这些：**

- 数组是什么
- `[]` 怎么访问
- 指针是什么
- `&`
- `*`
- 指针作为函数参数
- 为什么函数参数里经常出现 `float *`
- 为什么以后你会看到 `PI_Run(&pi_d, ...)`

暂时不学：

- 二级指针
- 函数指针
- 指针数组
- 动态内存
- `malloc`
- 链表

------

# Day 2 的最终目标

今天结束以后，你看到：

```
float current_abc[3];
```

能够知道它是在存：

```
Ia
Ib
Ic
```

看到：

```
float *value;
```

知道这是一个指针。

看到：

```
&iq
```

知道是：

> iq 的地址。

看到：

```
*value
```

知道是：

> 访问这个地址里面真正的数据。

最重要的是以后看到：

```
Current_Limit(&iq_ref, 3.0f);
```

你不能觉得这是某种很神秘的语法。

你应该马上理解：

> 把 `iq_ref` 的地址交给函数，所以这个函数可以直接修改真正的 `iq_ref`。

这就是今天的核心。

------

# 第一部分：先从真实电机变量理解数组

以后你有三相电流：

```
Ia
Ib
Ic
```

最笨的写法当然可以：

```
float ia;
float ib;
float ic;
```

这完全没问题。

但很多时候，也可以写成：

```
float current_abc[3];
```

然后：

```
current_abc[0] = 1.2f;
current_abc[1] = -0.5f;
current_abc[2] = -0.7f;
```

对应：

```
current_abc[0] → Ia

current_abc[1] → Ib

current_abc[2] → Ic
```

今天你先记住一个非常重要的规则：

> **C 语言数组从 0 开始。**

所以：

```
float current_abc[3];
```

里面实际有三个元素：

```
[0]
[1]
[2]
```

不是：

```
[1]
[2]
[3]
```

------

# 二、先给你完整的 Day 2 示例程序

今天直接使用一个很接近以后 FOC 数据处理的例子。

```
#include <stdio.h>
#include <stdint.h>


/* =========================
 * 三相电流
 * ========================= */

float current_abc[3] =
{
    1.20f,
   -0.50f,
   -0.70f
};


/* =========================
 * αβ 电流
 * ========================= */

float current_alpha_beta[2] =
{
    0.0f,
    0.0f
};


/* =========================
 * dq 电流
 * ========================= */

float current_dq[2] =
{
    0.0f,
    0.0f
};


/* =========================
 * 修改变量值
 * ========================= */

void Set_Value(float *value, float new_value)
{
    *value = new_value;
}


/* =========================
 * 电流限幅
 * ========================= */

void Current_Limit(float *current_ref, float limit)
{
    if (*current_ref > limit)
    {
        *current_ref = limit;
    }

    if (*current_ref < -limit)
    {
        *current_ref = -limit;
    }
}


/* =========================
 * 打印三相电流
 * ========================= */

void Print_ABC_Current(float current[3])
{
    printf("Ia = %.2f A\n", current[0]);
    printf("Ib = %.2f A\n", current[1]);
    printf("Ic = %.2f A\n", current[2]);
}


/* =========================
 * 主程序
 * ========================= */

int main(void)
{
    float iq_ref = 5.0f;

    printf("===== Original ABC Current =====\n");

    Print_ABC_Current(current_abc);


    printf("\n===== Pointer Test =====\n");

    printf("Original iq_ref = %.2f A\n", iq_ref);

    Set_Value(&iq_ref, 4.0f);

    printf("After Set_Value = %.2f A\n", iq_ref);


    printf("\n===== Current Limit Test =====\n");

    iq_ref = 8.0f;

    printf("Before Limit = %.2f A\n", iq_ref);

    Current_Limit(&iq_ref, 3.0f);

    printf("After Limit  = %.2f A\n", iq_ref);

    return 0;
}
```

这份程序今天基本够用了。

------

# 三、先看数组：为什么这样写

这里：

```
float current_abc[3] =
{
    1.20f,
   -0.50f,
   -0.70f
};
```

表示：

```
current_abc[0] = 1.20

current_abc[1] = -0.50

current_abc[2] = -0.70
```

你可以把它理解成一排三个盒子：

```
current_abc

┌──────────┬──────────┬──────────┐
│   1.20   │  -0.50   │  -0.70   │
├──────────┼──────────┼──────────┤
│   [0]    │   [1]    │   [2]    │
└──────────┴──────────┴──────────┘
     Ia         Ib         Ic
```

这已经非常贴合电机控制了。

------

# 四、以后 FOC 里数组很可能怎么出现

例如：

```
float current_abc[3];
float current_alpha_beta[2];
float current_dq[2];
```

你可以直接对应：

```
current_abc

Ia
Ib
Ic


current_alpha_beta

Iα
Iβ


current_dq

Id
Iq
```

也可能有人写：

```
float voltage_dq[2];
```

对应：

```
Vd
Vq
```

或者：

```
float duty_abc[3];
```

对应：

```
Duty_A
Duty_B
Duty_C
```

所以数组对你不是抽象的数据结构。

它很容易直接对应：

> **多个同类物理量。**

------

# 五、数组第一天你只需要注意一个坑

这个：

```
float current_abc[3];
```

只能正常访问：

```
current_abc[0];
current_abc[1];
current_abc[2];
```

如果你写：

```
current_abc[3] = 5.0f;
```

就错了。

虽然编译器有时候不一定直接阻止你，但你已经访问了数组范围之外的内存。

这种问题叫：

> 数组越界。

以后嵌入式里面很危险。

因为它可能不是马上报错，而是：

```
程序莫名其妙异常

某个变量突然被改掉

MCU跑飞

结果偶尔错
```

所以你以后看到：

```
float abc[3];
```

脑子里马上想到：

```
0
1
2
```

------

# 六、接下来进入今天最重要的：指针

先看：

```
float iq_ref = 5.0f;
```

现在内存里有一个变量：

```
iq_ref

┌────────────┐
│    5.0     │
└────────────┘
```

但这个数据肯定存储在 MCU 的某个内存位置。

比如假设它的地址是：

```
0x20000100
```

这个地址是什么？

你可以简单理解：

> 变量在内存里的“门牌号”。

------

# 七、`&iq_ref` 是什么

代码：

```
&iq_ref
```

意思：

> 取得 `iq_ref` 的地址。

例如：

```
iq_ref = 5.0
```

而：

```
&iq_ref
```

可能是：

```
0x20000100
```

因此：

```
iq_ref
```

是：

> 数据。

而：

```
&iq_ref
```

是：

> 数据所在的位置。

这两个一定要区分。

------

# 八、那什么是指针？

看：

```
float *value;
```

这句话表示：

> `value` 是一个指针，它可以保存一个 `float` 类型变量的地址。

比如：

```
float iq_ref = 5.0f;

float *value;

value = &iq_ref;
```

现在：

```
iq_ref
  ↓
5.0
```

而：

```
value
```

保存：

```
iq_ref 的地址
```

可以画成：

```
value
  │
  │ 保存地址
  ↓
┌─────────────┐
│ 0x20000100  │
└─────────────┘
       │
       │ 指向
       ↓
┌─────────────┐
│ iq_ref=5.0  │
└─────────────┘
```

所以这种变量为什么叫：

> Pointer，指针

因为它：

> **指向另外一个变量。**

------

# 九、`*value` 又是什么意思？

这也是今天最容易混的地方。

如果：

```
value = &iq_ref;
```

那么：

```
value
```

表示：

> iq_ref 的地址。

而：

```
*value
```

表示：

> 沿着这个地址找到真正的数据。

也就是：

```
*value
    ↓

iq_ref
    ↓

5.0
```

所以：

```
*value = 10.0f;
```

实际上就是：

```
iq_ref = 10.0f;
```

------

# 十、把 `&` 和 `*` 放一起理解

你今天只要记住：

```
&
↓
拿地址


*
↓
根据地址找到数据
```

比如：

```
float iq_ref = 5.0f;

float *p = &iq_ref;
```

现在：

```
iq_ref
```

是：

```
5.0
```

------

```
&iq_ref
```

是：

```
iq_ref 的地址
```

------

```
p
```

也是：

```
iq_ref 的地址
```

------

```
*p
```

又变回：

```
5.0
```

所以整个关系：

```
iq_ref
   ↓
 数据 5.0

&iq_ref
   ↓
 地址

p
   ↓
 保存这个地址

*p
   ↓
 地址对应的数据
   ↓
 5.0
```

------

# 十一、为什么电机控制函数经常使用指针？

现在回到我们的函数：

```
void Set_Value(float *value, float new_value)
{
    *value = new_value;
}
```

调用：

```
Set_Value(&iq_ref, 4.0f);
```

一步一步看。

原来：

```
iq_ref = 5.0
```

调用：

```
Set_Value(&iq_ref, 4.0f);
```

相当于：

> 把 iq_ref 的地址交给 `Set_Value()`。

所以函数里的：

```
value
```

实际上指向：

```
iq_ref
```

然后执行：

```
*value = new_value;
```

就是修改：

```
iq_ref
```

最后：

```
iq_ref = 4.0
```

------

# 十二、为什么不能简单写成这样？

你可能会问：

```
void Set_Value(float value, float new_value)
{
    value = new_value;
}
```

然后：

```
Set_Value(iq_ref, 4.0f);
```

为什么不行？

因为这种情况下：

```
iq_ref
    ↓
把数值 5.0 复制一份
    ↓
value
```

函数里面操作的是：

> 一个副本。

大概就是：

```
原来的：

iq_ref = 5


函数得到复制：

value = 5


修改：

value = 4


函数结束：

value 消失


原来的 iq_ref：

仍然 = 5
```

这就是为什么：

```
float value
```

和：

```
float *value
```

差别很大。

------

# 十三、这对以后 PI 控制器非常重要

你以后一定会看到这种：

```
PI_Run(&pi_d, id_ref, id);
```

这里：

```
&pi_d
```

核心思想和今天的：

```
&iq_ref
```

完全一样。

只是 `pi_d` 以后不是一个简单 `float`，而是一个：

```
struct
```

里面可能装着：

```
Kp
Ki
integral
output
limit
```

所以：

```
PI_Run(&pi_d, ...)
```

就是：

> 把整个 `pi_d` 控制器的地址交给函数，让函数直接修改这个 PI 控制器内部的数据。

例如：

```
pi_d.integral

pi_d.output
```

这就是为什么今天指针必须先懂。

------

# 十四、再看一个非常贴合电机控制的例子：电流限幅

我们的代码：

```
void Current_Limit(float *current_ref, float limit)
{
    if (*current_ref > limit)
    {
        *current_ref = limit;
    }

    if (*current_ref < -limit)
    {
        *current_ref = -limit;
    }
}
```

以后真实电机里很常见。

假设：

```
Iq_ref = 8 A
```

但是为了保护电机：

```
最大允许：

±3 A
```

那么：

```
Current_Limit(&iq_ref, 3.0f);
```

执行前：

```
iq_ref = 8 A
```

判断：

```
if (*current_ref > limit)
```

也就是：

```
8 > 3
```

成立。

所以：

```
*current_ref = limit;
```

最终：

```
iq_ref = 3 A
```

------

# 十五、为什么这里很适合用指针

因为这个函数的目的就是：

> **直接修改外面的 `iq_ref`。**

所以：

```
Current_Limit(&iq_ref, 3.0f);
```

逻辑非常合理。

类似以后还可能有：

```
Voltage_Limit(&vd, &vq);
```

或者：

```
Current_Calibration(&ia, &ib, &ic);
```

甚至：

```
Clarke_Run(&abc, &alpha_beta);
```

不同工程师写法会不同，但这种思想非常常见：

> 如果函数需要修改某个外部变量，指针就是一种常见手段。

------

# 十六、数组作为函数参数

再看：

```
void Print_ABC_Current(float current[3])
```

调用：

```
Print_ABC_Current(current_abc);
```

函数里面：

```
current[0]
current[1]
current[2]
```

就是访问：

```
Ia
Ib
Ic
```

这里你暂时不用深究：

> 数组参数和指针本质上的关系。

这个我们现在不需要。

第一周只要知道：

```
void Test(float data[3])
```

就是：

> 这个函数接收一个 float 数组。

够用了。

------

# 十七、Day 2 第一个修改实验：改单个相电流

把：

```
float current_abc[3] =
{
    1.20f,
   -0.50f,
   -0.70f
};
```

改成：

```
float current_abc[3] =
{
    2.00f,
   -1.00f,
   -1.00f
};
```

先预测打印结果：

```
Ia = ?

Ib = ?

Ic = ?
```

然后运行。

这个非常简单，但重点是让你习惯：

```
array[index]
```

这种形式。

------

# 十八、第二个实验：自己修改数组元素

在：

```
Print_ABC_Current(current_abc);
```

前面加：

```
current_abc[0] = 3.0f;
```

那么：

```
Ia
```

会变成：

```
3.0
```

但是：

```
Ib
Ic
```

不会变。

------

# 十九、第三个实验：看懂指针修改变量

原来：

```
float iq_ref = 5.0f;
```

执行：

```
Set_Value(&iq_ref, 4.0f);
```

你先不要运行。

先自己判断：

```
执行前：

iq_ref = ?


执行后：

iq_ref = ?
```

应该是：

```
5 → 4
```

------

# 二十、第四个实验：把 `&` 删除

故意写：

```
Set_Value(iq_ref, 4.0f);
```

看看编译器怎么报错。

它为什么报错？

因为函数要求：

```
float *value
```

也就是：

> 一个地址。

但是你传进去：

```
iq_ref
```

是：

> 一个 `float` 数据。

这两种东西类型不匹配。

这类错误以后非常常见。

所以不要怕编译错误。

它其实是在告诉你：

> “兄弟，这里我要地址，你给了我一个数。”

------

# 二十一、第五个实验：改限流值

原来：

```
iq_ref = 8.0f;

Current_Limit(&iq_ref, 3.0f);
```

所以：

```
8 → 3
```

现在改成：

```
iq_ref = 2.0f;

Current_Limit(&iq_ref, 3.0f);
```

预测：

```
iq_ref = ?
```

因为：

```
2 < 3
```

所以没有触发限幅。

最终还是：

```
2 A
```

------

# 二十二、第六个实验：测试负电流限幅

写：

```
iq_ref = -8.0f;

Current_Limit(&iq_ref, 3.0f);
```

应该变成：

```
-3 A
```

这个实验非常贴合真实电机控制。

因为：

```
Iq > 0
```

与：

```
Iq < 0
```

往往对应不同的转矩方向。

所以限流通常要考虑：

```
+Imax

和

-Imax
```

------

# 二十三、再给你一个以后会经常见到的写法

例如：

```
void Read_Phase_Current(float *ia,
                        float *ib,
                        float *ic)
{
    *ia = 1.0f;
    *ib = -0.4f;
    *ic = -0.6f;
}
```

调用：

```
float ia;
float ib;
float ic;

Read_Phase_Current(&ia, &ib, &ic);
```

执行以后：

```
ia = 1.0

ib = -0.4

ic = -0.6
```

为什么一个函数能“返回三个值”？

严格来说它不是：

```
return 三个值
```

而是：

> 通过三个指针直接修改三个外部变量。

这在嵌入式代码中非常常见。

------

# 二十四、这和普通 `return` 有什么区别？

例如：

```
float ADC_To_Current(uint16_t adc_raw)
{
    float current;

    current = ...;

    return current;
}
```

这个函数：

> 只需要得到一个结果。

所以直接：

```
return current;
```

非常合适。

但是如果我要同时得到：

```
Ia
Ib
Ic
```

可能就写：

```
void Read_Current(float *ia,
                  float *ib,
                  float *ic);
```

所以以后你可以大致这样判断：

### 一个简单结果

可能：

```
float Calculate_Speed(...);
```

------

### 需要修改多个数据

经常：

```
void Clarke_Run(...);
```

配合指针或者结构体输出。

当然实际项目写法很多，不是绝对规则。

------

# 二十五、今天你必须彻底区分这四个东西

假设：

```
float iq = 2.5f;

float *p = &iq;
```

### `iq`

表示：

```
2.5
```

------

### `&iq`

表示：

```
iq 的地址
```

------

### `p`

表示：

```
iq 的地址
```

------

### `*p`

表示：

```
这个地址里面的数据

= iq

= 2.5
```

这四个如果今天搞明白，指针最重要的第一关就已经过了。

------

# 二十六、一个以后你非常容易碰到的问题

看到：

```
float *p;
```

不要理解为：

> “*p 是一个 float。”

更准确一点是：

> `p` 是一个指向 `float` 的指针。

而当你写：

```
*p
```

的时候，才表示：

> p 指向的那个 float 数据。

这两个 `*` 出现的位置不同，理解也稍微不同。

### 定义变量：

```
float *p;
```

意思：

> p 是 float 指针。

### 使用变量：

```
*p = 10.0f;
```

意思：

> 修改 p 指向的数据。

这一点以后很重要。

------

# 二十七、Day 2 你不需要背内存地址

你完全没必要今天去背：

```
Stack
Heap
RAM
ROM
.data
.bss
```

这些以后遇到再学。

今天只需要脑子里面有：

```
变量
 ↓
存储在某个内存位置

&
 ↓
取得这个位置

指针
 ↓
保存这个位置

*
 ↓
通过这个位置访问数据
```

够了。

------

# 二十八、今天再提前给你看一眼未来 FOC 代码

你以后可能看到：

```
void Clarke_Run(float ia,
                float ib,
                float *i_alpha,
                float *i_beta)
{
    *i_alpha = ia;

    *i_beta = 0.57735f * (ia + 2.0f * ib);
}
```

调用：

```
float ia = 1.0f;
float ib = -0.5f;

float i_alpha;
float i_beta;

Clarke_Run(ia,
           ib,
           &i_alpha,
           &i_beta);
```

你现在完全不用学习 Clarke 公式。

只观察 C 语言：

```
ia
ib
 ↓
普通输入


&i_alpha
&i_beta
 ↓
把输出变量地址交给函数


*i_alpha
*i_beta
 ↓
函数修改真正的输出变量
```

这就是今天学指针最直接的意义。

等 Week 5 真正写 Clarke/Park 时，你不会再被语法卡住。

------

# 二十九、今天不建议你学“指针运算”

比如：

```
p++;
```

以及：

```
*(p + 1)
```

虽然和数组关系非常大，但是暂时没必要。

因为你现在真正需要的嵌入式能力主要是：

```
float *value
&variable
*value
```

这三个先熟。

------

# 三十、Day 2 推荐执行顺序

今天大概 **2～2.5 小时**。

## 第 1 阶段：30 分钟

运行我上面的完整程序。

先看数组：

```
current_abc
```

搞懂：

```
[0]
[1]
[2]
```

------

## 第 2 阶段：40 分钟

专门理解：

```
float iq_ref;

&iq_ref

float *p;

*p
```

不要急着往下。

如果这一块没懂，停在这里反复改程序。

------

## 第 3 阶段：40 分钟

研究：

```
Set_Value()
```

重点理解为什么：

```
Set_Value(&iq_ref, 4.0f);
```

能够直接改变：

```
iq_ref
```

------

## 第 4 阶段：30 分钟

研究：

```
Current_Limit()
```

测试：

```
Iq_ref = 8

Iq_ref = 2

Iq_ref = -8
```

------

## 第 5 阶段：20～30 分钟

做我上面说的几个修改。

尤其故意把：

```
&iq_ref
```

写错一次。

观察编译器。

------

# 三十一、Day 2 的验收题

今天结束以后，你自己不用查资料回答：

### 1

```
float current_abc[3];
```

可以存几个 `float`？

------

### 2

为什么：

```
current_abc[3]
```

不能访问第四个元素？

------

### 3

```
float iq = 2.0f;
```

这里：

```
iq
```

代表什么？

------

### 4

```
&iq
```

代表什么？

------

### 5

```
float *p = &iq;
```

`p` 保存的是什么？

------

### 6

```
*p
```

表示什么？

------

### 7

为什么：

```
void Set_Value(float value)
```

通常不能直接修改外面的变量？

------

### 8

为什么：

```
void Set_Value(float *value)
```

可以配合：

```
Set_Value(&iq);
```

修改外部变量？

------

### 9

看到：

```
PI_Run(&pi_d, ...);
```

即使你现在不知道 PI 函数内部怎么写，你是否已经大概知道：

```
&pi_d
```

是什么意思？

如果这 9 个你能答出 **7～8 个**，Day 2 就完全可以过。

------

# Day 2 最值得你记住的一张图

```
普通变量：

float iq = 2.5f;

      iq
      ↓
┌──────────┐
│   2.5    │
└──────────┘
      ↑
      │
    &iq
      │
      │
      p
┌──────────┐
│  地址    │
└──────────┘


p      → 地址

*p     → 地址里的数据

&iq    → iq的地址
```

以后看到指针，先回到这张关系图。

------

## Day 2 最终只要求做到一句话

你今天不需要“掌握 C 语言指针”。

只需要做到：

> **看到 `float \*p`、`&iq`、`\*p` 时不害怕，而且能知道函数为什么通过指针修改电机控制变量。**

这就足够。