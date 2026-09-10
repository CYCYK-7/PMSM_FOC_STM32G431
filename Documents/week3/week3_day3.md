# STM32 学习笔记：Clarke、Park 与 Inverse Park 坐标变换

## 一、当前学习位置

前一阶段已经完成：

```text
PWM同步电流采样
↓
ADC Offset补偿
↓
Iu / Iv真实电流
```

本阶段开始正式进入 FOC 数学部分：

```text
Iu / Iv
↓
Clarke
↓
Iα / Iβ
↓
Park
↓
Id / Iq
```

并建立控制输出侧的：

```text
Vd / Vq
↓
Inverse Park
↓
Vα / Vβ
```

最终为后面的：

```text
Current PI
↓
Inverse Park
↓
SVPWM
↓
TIM1 CCR
```

做准备。

---

# 二、为什么需要坐标变换

三相 PMSM 中实际存在：

```text
Iu
Iv
Iw
```

三个相电流。

但三相无中性线系统满足：

$$
i_u+i_v+i_w=0
$$

所以：

$$
i_w=-i_u-i_v
$$

三个电流实际上只有两个独立自由度。

FOC 不直接在三相坐标系中控制，而是通过：

```text
uvw
↓ Clarke
αβ
↓ Park
dq
```

把三相交流电流最终转换为旋转坐标系下近似直流的：

```text
Id
Iq
```

这样就可以使用 PI 控制器进行电流控制。

---

# 三、Clarke Transform

## 作用

Clarke Transform：

```text
三相静止坐标系
uvw
↓
二维静止坐标系
αβ
```

当前只测量：

```text
Iu
Iv
```

因此使用两电流形式的 Clarke。

---

## 当前采用的公式

采用：

$$
\boxed{i_\alpha=i_u}
$$

$$
\boxed{
i_\beta=
\frac{i_u+2i_v}{\sqrt3}
}
$$

其中：

$$
\frac{1}{\sqrt3}
\approx0.577350269
$$

所以：

```c
i_alpha = iu;

i_beta =
    0.577350269f * (iu + 2.0f * iv);
```

---

## Clarke 的物理意义

Clarke 并没有把交流量变成直流量。

它只是把：

```text
三相静止坐标系
```

转换为：

```text
二维静止 αβ 坐标系
```

因此：

```text
Iα
Iβ
```

仍然会随着电机旋转而变化。

真正把这些旋转交流量变成近似直流量的是后面的：

> Park Transform。

---

# 四、建立 FOC 数学模块

从 Clarke 开始，坐标变换不继续直接堆在 `main.c` 中。

建立：

```text
Core/Inc/foc_math.h
Core/Src/foc_math.c
```

以后：

```text
Clarke
Park
InvPark
```

统一放在该模块中。

---

## AlphaBeta 数据结构

```c
typedef struct
{
    float alpha;
    float beta;

} AlphaBeta_t;
```

---

## Clarke 函数声明

```c
void Clarke_Run(float iu,
                float iv,
                AlphaBeta_t *out);
```

---

## Clarke 函数实现

```c
#define ONE_BY_SQRT3  0.577350269f

void Clarke_Run(float iu,
                float iv,
                AlphaBeta_t *out)
{
    out->alpha = iu;

    out->beta =
        ONE_BY_SQRT3 * (iu + 2.0f * iv);
}
```

---

# 五、Clarke 接入实时控制链

ADC 电流换算之前使用：

```text
mA
```

FOC 内部统一转换为：

```text
A
```

代码：

```c
iu_a = (float)iu_ma * 0.001f;
iv_a = (float)iv_ma * 0.001f;
```

然后：

```c
Clarke_Run(iu_a,
           iv_a,
           &i_ab);
```

于是实时链变为：

```text
ADC同步采样
↓
Iu / Iv
↓
Offset
↓
Current
↓
Clarke
↓
Iα / Iβ
```

由于：

$$
i_\alpha=i_u
$$

所以 Debug 时：

```text
i_ab.alpha
```

应当和：

```text
iu_a
```

基本完全一致。

这是检查 Clarke 是否正确的一个简单方法。

---

# 六、STM32 printf 浮点数支持

调试 Clarke 时开始通过 UART 打印：

```text
Iu
Iv
Ialpha
Ibeta
```

例如：

```c
snprintf(tx_buffer,
         sizeof(tx_buffer),
         "Iu=%.3f Iv=%.3f | Ialpha=%.3f Ibeta=%.3f\r\n",
         iu_a,
         iv_a,
         i_ab.alpha,
         i_ab.beta);
```

其中：

```c
%.3f
```

表示输出浮点数并保留三位小数。

STM32CubeIDE 默认可能没有开启浮点格式化支持。

需要在工程设置中开启：

```text
-u _printf_float
```

否则会提示：

```text
Float formatting support is not enabled
```

由于串口字符串变长，调试缓冲区也由较小长度增加，例如：

```c
char tx_buffer[100];
```

---

# 七、Park Transform

## Park 的作用

Clarke 得到：

```text
Iα
Iβ
```

但 αβ 坐标系仍然固定在定子上。

Park Transform 将其转换到一个：

> 跟随转子磁场一起旋转的 dq 坐标系。

即：

```text
Iα / Iβ
↓
Park
↓
Id / Iq
```

理想稳态时：

```text
三相正弦交流电流
↓
Clarke
↓
αβ旋转电流矢量
↓
Park
↓
Id / Iq近似直流
```

因此：

> FOC 可以使用普通 PI 控制器调节 Id 和 Iq。

---

# 八、机械角与电角度

Park Transform 必须知道：

$$
\theta_e
$$

即：

> 转子电角度。

机械角：

$$
\theta_m
$$

电角：

$$
\boxed{\theta_e=p\theta_m}
$$

其中：

```text
p = 极对数
```

例如：

```text
4极电机
→ 2极对
→ p = 2
```

机械转：

$$
180^\circ
$$

则电角度变化：

$$
360^\circ
$$

---

## 以后真实系统中的角度来源

后面编码器阶段会形成：

```text
Encoder
↓
机械角 θm
↓
× 极对数 p
↓
电角度零点补偿
↓
θe
↓
Park
```

当前还没有接入编码器，因此使用：

```text
theta_e_test
```

作为临时测试电角度。

它不是实际转子位置。

---

# 九、当前采用的 Park 公式

固定采用：

$$
\boxed{
i_d=
i_\alpha\cos\theta_e+
i_\beta\sin\theta_e
}
$$

$$
\boxed{
i_q=
-i_\alpha\sin\theta_e+
i_\beta\cos\theta_e
}
$$

---

## DQ 数据结构

```c
typedef struct
{
    float d;
    float q;

} DQ_t;
```

---

## Park 函数

```c
void Park_Run(float alpha,
              float beta,
              float theta_e,
              DQ_t *out)
{
    float sin_theta = sinf(theta_e);
    float cos_theta = cosf(theta_e);

    out->d =
        alpha * cos_theta
        + beta * sin_theta;

    out->q =
        -alpha * sin_theta
        + beta * cos_theta;
}
```

注意：

> `sinf()` 和 `cosf()` 输入的是弧度 rad，不是角度 degree。

---

# 十、Park 数学验证

进行了两组简单测试。

## Test 1

输入：

```text
alpha = 1
beta  = 0
theta = 0
```

因为：

$$
\cos0=1
$$

$$
\sin0=0
$$

得到：

$$
d=1
$$

$$
q=0
$$

实际测试正确。

---

## Test 2

输入：

```text
alpha = 1
beta  = 0
theta = π/2
```

得到：

$$
d=0
$$

$$
q=-1
$$

实际测试正确。

说明当前 Park 公式与代码实现正确。

---

# 十一、人工电角度发生器

为了暂时没有编码器时让 Park 在实时链中运行，建立人工旋转电角度。

设置测试电角频率：

$$
f_e=1Hz
$$

意思是：

> 电角度每秒完整旋转一圈。

也就是每秒增加：

$$
2\pi
$$

当前控制频率：

$$
f_s=20kHz
$$

因此每个控制周期角度增加：

$$
\Delta\theta_e
=
\frac{2\pi f_e}{f_s}
$$

代入：

$$
\Delta\theta_e
=
\frac{2\pi}{20000}
$$

得到：

$$
\boxed{
\Delta\theta_e
\approx0.000314159
}
$$

代码：

```c
theta_e_step =
    6.283185307f / 20000.0f;
```

在每次高速控制周期：

```c
theta_e_test += theta_e_step;
```

---

# 十二、电角度归一化

电角度不需要无限增加。

因此：

```c
if (theta_e_test >= 6.283185307f)
{
    theta_e_test -= 6.283185307f;
}
```

使：

$$
\boxed{
0\leq\theta_e<2\pi
}
$$

形成：

```text
0
↓
π/2
↓
π
↓
3π/2
↓
2π
↓
0
```

不断循环。

---

# 十三、20 kHz、1 Hz 与 UART 1 Hz 的区别

这一部分容易混淆。

当前存在三个不同的频率：

```text
控制更新频率      = 20 kHz
测试电角频率      = 1 Hz
UART打印频率      ≈ 1 Hz
```

---

## 控制频率 20 kHz

代表每秒执行：

```text
20000次
```

高速控制计算。

每次电角度只增加：

$$
0.000314159rad
$$

---

## 电角频率 1 Hz

代表一秒钟累计：

$$
20000\times0.000314159
\approx2\pi
$$

所以：

> 电角度每秒实际上完整旋转一圈。

---

## UART 1 Hz

UART 只是：

> 每秒观察一次当前变量。

它并不负责更新角度。

由于：

```c
theta_e_test
```

每到：

$$
2\pi
$$

就重新减去：

$$
2\pi
$$

所以串口看到的始终是：

```text
0 ~ 6.283
```

范围内的当前相位。

不会看到：

```text
6.283
12.566
18.849
...
```

---

## 为什么串口可能显示缓慢增加

实际观察曾出现：

```text
5.957
5.959
5.962
5.967
...
```

这并不代表：

> 电角度每秒只增加约 0.002 rad。

实际上每秒已经完整增加了约：

$$
2\pi
$$

只是完成一圈以后进行了取模。

PWM 控制时基与 UART 打印时基之间不是绝对同步，因此每次 UART 采到的相位会存在轻微漂移。

---

# 十四、Park 接入实时控制链

当前：

```c
Clarke_Run(iu_a,
           iv_a,
           &i_ab);

theta_e_test += theta_e_step;

if (theta_e_test >= 6.283185307f)
{
    theta_e_test -= 6.283185307f;
}

Park_Run(i_ab.alpha,
         i_ab.beta,
         theta_e_test,
         &i_dq);
```

形成：

```text
TIM1 20kHz
↓
ADC同步采样
↓
Iu / Iv
↓
Clarke
↓
Iα / Iβ
↓
Park
↓
Id / Iq
```

当前电机没有运行，所以：

```text
Iu / Iv ≈ 0
Iα / Iβ ≈ 0
Id / Iq ≈ 0
```

存在少量几十 mA 级波动属于正常采样噪声。

---

# 十五、Inverse Park Transform

Inverse Park 是 Park 的逆变换。

Park：

```text
αβ
↓
dq
```

Inverse Park：

```text
dq
↓
αβ
```

但实际 FOC 中两者处理的物理量不同。

反馈侧：

```text
Iα / Iβ
↓
Park
↓
Id / Iq
```

控制输出侧：

```text
Vd / Vq
↓
InvPark
↓
Vα / Vβ
```

---

# 十六、为什么 InvPark 输入是 Vd/Vq

真正电流闭环中：

```text
Id_ref - Id
↓
d轴 PI
↓
Vd

Iq_ref - Iq
↓
q轴 PI
↓
Vq
```

所以 InvPark 接收的是：

```text
Vd
Vq
```

而不是把刚刚计算出来的：

```text
Id
Iq
```

直接送进去。

完整控制结构：

```text
Iu / Iv
↓
Clarke
↓
Park
↓
Id / Iq
↓
Current PI
↓
Vd / Vq
↓
InvPark
↓
Vα / Vβ
↓
SVPWM
```

当前还没有学习 Current PI，因此暂时使用固定测试电压指令：

```c
vd_cmd = 0.0f;
vq_cmd = 1.0f;
```

它们只是为了提前把 InvPark 接口打通。

---

# 十七、当前采用的 InvPark 公式

由于当前 Park 为：

$$
d=
\alpha\cos\theta+
\beta\sin\theta
$$

$$
q=
-\alpha\sin\theta+
\beta\cos\theta
$$

所以对应的逆变换必须为：

$$
\boxed{
v_\alpha=
v_d\cos\theta_e-
v_q\sin\theta_e
}
$$

$$
\boxed{
v_\beta=
v_d\sin\theta_e+
v_q\cos\theta_e
}
$$

Park 和 InvPark 的符号约定必须保持一致。

---

# 十八、InvPark 函数

函数声明：

```c
void InvPark_Run(float d,
                 float q,
                 float theta_e,
                 AlphaBeta_t *out);
```

实现：

```c
void InvPark_Run(float d,
                 float q,
                 float theta_e,
                 AlphaBeta_t *out)
{
    float sin_theta = sinf(theta_e);
    float cos_theta = cosf(theta_e);

    out->alpha =
        d * cos_theta
        - q * sin_theta;

    out->beta =
        d * sin_theta
        + q * cos_theta;
}
```

---

# 十九、InvPark 接入实时链

当前暂时使用：

```c
vd_cmd = 0.0f;
vq_cmd = 1.0f;
```

实时调用：

```c
InvPark_Run(vd_cmd,
            vq_cmd,
            theta_e_test,
            &v_ab);
```

因为：

$$
v_d=0
$$

$$
v_q=1
$$

所以：

$$
v_\alpha=-\sin\theta_e
$$

$$
v_\beta=\cos\theta_e
$$

因此随着：

```text
theta_e_test
```

不断旋转：

```text
Valpha ≈ -1 ~ +1
Vbeta  ≈ -1 ~ +1
```

周期变化。

---

# 二十、目前已经形成的 FOC 数学链

反馈侧：

```text
Iu / Iv
↓
Clarke
↓
Iα / Iβ
↓
Park
↓
Id / Iq
```

控制输出侧目前为：

```text
临时 Vd / Vq
↓
InvPark
↓
Vα / Vβ
```

以后将连接成：

```text
Iu / Iv
↓
Clarke
↓
Park
↓
Id / Iq
↓
Current PI
↓
Vd / Vq
↓
InvPark
↓
Vα / Vβ
↓
SVPWM
↓
CCR1 / CCR2 / CCR3
↓
TIM1 PWM
```

---

# 二十一、当前哪些量是真实的，哪些是测试量

## 来自硬件的真实反馈量

```text
Iu
Iv
```

由 ADC 同步采样得到。

---

## FOC 数学计算量

```text
Ialpha
Ibeta
Id
Iq
Valpha
Vbeta
```

---

## 当前临时测试量

```text
theta_e_test
vd_cmd
vq_cmd
```

其中：

```text
theta_e_test
```

以后由：

```text
Encoder → Electrical Angle
```

替代。

而：

```text
vd_cmd
vq_cmd
```

以后由：

```text
Current PI
```

输出。

---

# 二十二、本阶段核心公式

## Clarke

$$
\boxed{
i_\alpha=i_u
}
$$

$$
\boxed{
i_\beta=
\frac{i_u+2i_v}{\sqrt3}
}
$$

---

## Park

$$
\boxed{
i_d=
i_\alpha\cos\theta_e+
i_\beta\sin\theta_e
}
$$

$$
\boxed{
i_q=
-i_\alpha\sin\theta_e+
i_\beta\cos\theta_e
}
$$

---

## Inverse Park

$$
\boxed{
v_\alpha=
v_d\cos\theta_e-
v_q\sin\theta_e
}
$$

$$
\boxed{
v_\beta=
v_d\sin\theta_e+
v_q\cos\theta_e
}
$$

---

## 电角度

$$
\boxed{
\theta_e=p\theta_m
}
$$

---

## 人工电角度步长

$$
\boxed{
\Delta\theta_e=
\frac{2\pi f_e}{f_s}
}
$$

当前：

$$
f_e=1Hz
$$

$$
f_s=20kHz
$$

因此：

$$
\Delta\theta_e
\approx0.000314159
$$

---

# 二十三、本阶段需要真正理解的内容

- Clarke 将三相静止坐标转换为二维静止 αβ 坐标。
- Clarke 后的电流仍然是交流变化量。
- Park 将 αβ 坐标转换到跟随转子旋转的 dq 坐标。
- 稳态时 Id/Iq 可以近似看作直流量，因此适合 PI 控制。
- Park 必须知道转子电角度。
- 电角度不是机械角，二者相差极对数。
- `sinf()` 和 `cosf()` 使用弧度。
- 当前 `theta_e_test` 只是测试信号，不是真实转子角度。
- 20 kHz 是控制计算频率。
- 1 Hz 是当前人工电角旋转频率。
- UART 1 Hz 只是观察频率。
- InvPark 的实际输入是 PI 输出的 Vd/Vq。
- Park 与 InvPark 必须使用一致的符号约定。
- 坐标变换已经放入高速实时控制链，而不是 `while(1)` 后台任务。

---

# 二十四、当前学习进度

已完成：

- [x] Clarke Transform
- [x] Clarke 实时接入
- [x] STM32 UART 浮点输出
- [x] Park Transform
- [x] Park 数学验证
- [x] 人工电角度发生器
- [x] 电角度归一化
- [x] Park 实时接入
- [x] Inverse Park Transform
- [x] InvPark 实时接入

下一阶段：

> **Current PI：利用 Id/Iq 与参考值之间的误差计算 Vd/Vq，开始建立真正的 FOC 电流闭环控制器。**