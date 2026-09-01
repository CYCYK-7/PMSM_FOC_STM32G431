# STM32 当前阶段必须熟记的核心知识

## 1. GPIO

GPIO：

> **General Purpose Input/Output，通用输入输出。**

最基本就两种：

```text
Input
→ MCU读取外部高低电平

Output
→ MCU向外输出高低电平
```

例如：

```text
LED
按键
使能信号
故障信号
```

都经常通过 GPIO 处理。

### Push-Pull

推挽输出。

特点：

```text
可以主动输出高电平
也可以主动输出低电平
```

普通 LED、Enable 等数字输出经常用它。

### Pull-Up / Pull-Down

用于避免输入引脚悬空。

```text
Pull-Up
→ 默认保持高电平

Pull-Down
→ 默认保持低电平
```

### 最重要的一点

> **GPIO 配置必须和原理图上的真实引脚一致。**

程序不报错，不代表 GPIO 就配置正确。

------

# 2. 引脚复用

STM32 一个引脚通常可以有多个功能，例如：

```text
GPIO
UART
TIM
SPI
I2C
ADC
```

但同一时间只能选择其中一个功能。

这叫：

> **Pin Multiplexing，引脚复用。**

所以以后配置外设时首先要知道：

```text
原理图这个信号
↓
接 MCU 哪个引脚
↓
这个引脚配置成什么外设功能
```

这件事必须形成习惯。

------

# 3. 时钟 Clock

STM32 里面几乎所有东西都需要时钟。

例如：

```text
CPU运行
Timer计数
PWM
UART
ADC
SPI
```

都离不开 Clock。

可以把时钟理解成：

> **MCU 各个模块工作的节拍。**

时钟越快，同样时间里 MCU 能完成的操作通常越多。

------

# 4. HSI 和 HSE

### HSI

High Speed Internal：

> **MCU 内部高速时钟。**

不需要外部晶振。

例如当前 STM32G431 使用：

```text
HSI = 16 MHz
```

### HSE

High Speed External：

> **外部高速时钟。**

一般来自外部晶振或外部时钟源。

目前先记：

```text
HSI = 内部
HSE = 外部
```

就够了。

------

# 5. PLL

PLL：

> **Phase Locked Loop，锁相环。**

在 STM32 里现阶段最重要的理解就是：

> **用来把已有时钟经过分频、倍频，得到需要的系统时钟。**

例如当前配置：

```text
HSI 16 MHz
↓
PLL
↓
SYSCLK 170 MHz
```

不用背 PLL 内部工作原理，但要知道：

```text
低频时钟
↓
分频 / 倍频
↓
得到目标高频时钟
```

------

# 6. SYSCLK、HCLK、PCLK

不用现在背完整时钟树，但这三个名字最好认识。

### SYSCLK

> 系统主时钟。

例如当前：

```text
SYSCLK = 170 MHz
```

### HCLK

主要供：

```text
CPU
AHB总线
Memory
```

### PCLK

Peripheral Clock：

> 外设时钟。

UART、Timer、SPI 等外设都需要对应的 PCLK。

当前阶段记住：

```text
SYSCLK
↓
系统主时钟

HCLK
↓
CPU / AHB

PCLK
↓
外设
```

即可。

------

# 7. UART / USART

UART：

> **Universal Asynchronous Receiver/Transmitter**

最常见的串口通信。

最基本三根线：

```text
TX
RX
GND
```

连接时：

```text
STM32 TX → 对方 RX
STM32 RX ← 对方 TX
GND      ↔ GND
```

最重要的连接原则：

> **TX 接 RX，RX 接 TX，并且必须共地。**

------

# 8. 115200 8N1

这个以后看到最好马上能懂。

例如：

```text
115200 8N1
```

代表：

```text
115200
→ 波特率

8
→ 8个数据位

N
→ 无奇偶校验

1
→ 1个停止位
```

这是非常常见的串口参数。

------

# 9. UART 和 USART 的区别

UART：

> 只支持异步通信。

USART：

> 支持同步 + 异步。

但是如果 STM32 中：

```text
USART3
→ Asynchronous
```

那实际就是按普通 UART 使用。

目前理解到这里即可。

------

# 10. Timer 定时器

这个马上就会学，是非常重要的外设。

Timer 可以理解成：

> **根据时钟不断计数的硬件模块。**

例如：

```text
0
1
2
3
4
...
```

计到指定值以后可以：

```text
产生中断
产生PWM
触发ADC
测量输入信号
```

以后电机控制里 Timer 非常核心。

------

# 11. Prescaler

Prescaler：

> **预分频器。**

作用就是：

> 把 Timer 输入时钟先降下来。

例如：

```text
Timer Clock = 170 MHz
```

Prescaler 设成：

```text
169
```

那么计数器时钟大致变成：

170MHz169+1=1MHz\frac{170MHz}{169+1} = 1MHz

也就是：

```text
每1 μs计一次数
```

这个以后算 Timer、PWM 会经常用。

------

# 12. Counter / ARR

Timer 里面还要重点认识两个东西：

### Counter

当前计数值。

例如：

```text
0 → 1 → 2 → 3 → ...
```

### ARR

Auto Reload Register。

可以简单理解成：

> **Timer 计到多少重新开始。**

例如：

```text
计数频率 = 1 MHz
ARR = 999
```

那么：

```text
1000次计数
×
1 μs
=
1 ms
```

所以 Timer 可以每 1 ms 产生一次事件。

------

# 13. Interrupt 中断

中断可以理解成：

> **发生某个重要事件时，CPU 暂时停下当前任务，先处理这个事件。**

例如：

```text
Timer计满
↓
产生Interrupt
↓
CPU进入ISR
↓
执行代码
↓
返回原程序
```

以后电机控制里：

```text
Timer
ADC
UART
Fault
```

都会大量用中断。

------

# 14. ISR

ISR：

> **Interrupt Service Routine，中断服务程序。**

就是：

> 中断发生以后执行的函数。

以后例如：

```text
PWM周期到
↓
ADC采样完成
↓
进入ISR
↓
运行FOC
```

所以真正的高速电机控制一般不是简单写在：

```c
while(1)
```

里面。

------

# 15. PWM

PWM：

> **Pulse Width Modulation，脉宽调制。**

核心就两个概念：

```text
Frequency
→ PWM频率

Duty Cycle
→ 占空比
```

例如：

```text
PWM频率 = 20 kHz
Duty = 50%
```

表示每：

50μs50\mu s

一个周期，其中一半时间为高电平。

以后 PMSM 控制中的：

```text
SVPWM
```

最终就是转换成三相 PWM 占空比。

------

# 16. ADC

ADC：

> **Analog-to-Digital Converter，模数转换器。**

作用：

> 把真实世界中的模拟电压转换成 MCU 可以处理的数字量。

例如电机控制需要采：

```text
相电流
母线电压
温度
```

通常都是：

```text
传感器电压
↓
ADC
↓
数字量
↓
程序计算
```

以后做 FOC：

> **ADC 是获取电流反馈的核心。**

------

# 17. DMA

DMA：

> **Direct Memory Access，直接存储器访问。**

最核心的理解：

> **由 DMA 帮 CPU 搬数据，不需要 CPU 一次次自己搬。**

例如：

```text
ADC
↓
DMA
↓
RAM数组
```

CPU 可以同时执行别的任务。

以后做高速 ADC 采样时非常常用。

------

# 18. 当前阶段最值得背熟的词

目前建议真正记到脑子里的只有这些：

```text
GPIO
Input / Output
Push-Pull
Pull-Up / Pull-Down
Pin Multiplexing

HSI
HSE
PLL
SYSCLK
HCLK
PCLK

UART
USART
TX
RX
Baud Rate
115200 8N1

Timer
Prescaler
Counter
ARR
Interrupt
ISR

PWM
Frequency
Duty Cycle

ADC
DMA
```

如果这些以后别人随便问一个，你都能用一两句话解释清楚，那你目前的 STM32 基础就已经在往正确方向走了。

其中优先级最高的是：

> **GPIO → Clock → UART → Timer → Interrupt → PWM → ADC → DMA**

这也正好就是我们接下来实际开发的学习顺序。