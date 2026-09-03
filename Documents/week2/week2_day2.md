可以。今天这一天的内容比昨天更“嵌入式”一些，因为已经从单纯 GPIO 进入了：

> **串口通信 → 硬件定时器 → 中断 → ISR → 主循环与中断协作**

而且继续在同一个 `PMSM_FOC_G431` 主工程上增加功能，不另外复制新工程，这正符合之前确定的工程演进方式。

下面按昨天笔记的风格整理。

# STM32G431 学习笔记 02

## USART3 串口通信 + TIM6 1 ms 定时中断

------

# 一、今天完成了什么

今天在昨天已经完成的：

```text
STM32G431 工程
+
170 MHz 系统时钟
+
GPIO
+
LED
+
ST-LINK
```

基础上继续完成了：

```text
USART3 配置
↓
USB-TTL 硬件连接
↓
电脑识别 COM 口
↓
串口助手接收数据
↓
Hello Motor Control
↓
周期输出 Counter
↓
TIM6 配置
↓
1 ms 定时中断
↓
Debug 观察 timer_counter
↓
Timer + UART 联动
```

现在已经有一条比较完整的数据链：

```text
STM32G431
   ↓
USART3
   ↓
USB-TTL
   ↓
电脑 COM 口
   ↓
串口调试助手
```

以及一条时间控制链：

```text
170 MHz Clock
   ↓
TIM6
   ↓
Prescaler
   ↓
Counter
   ↓
ARR
   ↓
1 ms Update Event
   ↓
Interrupt
   ↓
ISR / Callback
```

------

# 二、USART3 的配置

在 `.ioc` 中进入：

```text
Connectivity
→ USART3
```

设置：

```text
Mode              Asynchronous

Baud Rate         115200 Bits/s
Word Length       8 Bits
Parity            None
Stop Bits         1
Data Direction    Receive and Transmit
Flow Control      Disable
```

也就是常说的：

> **115200 8N1**

当前 CubeMX 分配：

```text
PB10 → USART3_TX
PB11 → USART3_RX
```

------

# 三、115200 8N1 必须看懂

这是串口通信中非常常见的表达。

```text
115200
→ 波特率

8
→ 8 个数据位

N
→ No Parity，无奇偶校验

1
→ 1 个停止位
```

所以看到：

```text
115200 8N1
```

以后应该能够直接知道：

> 串口速率 115200，8 位数据，无校验，1 位停止位。

------

# 四、UART / USART 的区别

UART：

> Universal Asynchronous Receiver/Transmitter

即：

> 通用异步收发器。

USART：

> Universal Synchronous/Asynchronous Receiver/Transmitter

比 UART 多支持同步模式。

但是本次：

```text
USART3
→ Asynchronous
```

因此实际上就是把 USART3 当普通 UART 使用。

------

# 五、USB-TTL 是干什么的

电脑的 USB 接口不能直接当 STM32 的 UART 使用。

因此中间使用：

```text
USB-TTL
```

完成：

```text
电脑 USB
↕
USB-TTL转换
↕
TTL UART
↕
STM32
```

也就是说：

> USB-TTL 是电脑 USB 与 MCU TTL 电平串口之间的转换器。

这个工具以后调试 MCU 非常常见。

------

# 六、UART 最重要的接线规则

本次只需要：

```text
TX
RX
GND
```

连接规则：

```text
STM32 TX ─────→ USB-TTL RX

STM32 RX ←───── USB-TTL TX

STM32 GND ───── USB-TTL GND
```

必须记住：

> **TX 接 RX，RX 接 TX。**

不能：

```text
TX → TX
RX → RX
```

另外：

> **必须共地。**

因为 UART 的 HIGH / LOW 电压必须有共同参考点。

------

# 七、为什么这次没有连接 USB-TTL 的 3.3V

如果开发板已经自己正常供电：

```text
USB-TTL
```

只负责通信即可。

所以：

```text
TX
RX
GND
```

就够了。

没有必要再通过 USB-TTL 给整块电机控制板供电。

对于以后的硬件调试，要形成一个习惯：

> **不要看到模块上的 3.3V / 5V 就随便连接，先确认谁给谁供电。**

------

# 八、第一个 UART 发送程序

本次第一次验证 UART 时使用：

```c
uint8_t message[] = "Hello Motor Control\r\n";

HAL_UART_Transmit(&huart3,
                  message,
                  sizeof(message) - 1,
                  HAL_MAX_DELAY);
```

串口助手成功收到：

```text
Hello Motor Control
```

这一步证明：

```text
USART3配置       ✅
TX引脚           ✅
USB-TTL连接      ✅
电脑COM口         ✅
波特率            ✅
UART发送          ✅
```

真正的硬件通信链已经跑通。

------

# 九、HAL_UART_Transmit() 怎么理解

函数：

```c
HAL_UART_Transmit(...)
```

当前阶段需要知道四个主要参数。

例如：

```c
HAL_UART_Transmit(&huart3,
                  data,
                  length,
                  HAL_MAX_DELAY);
```

### `&huart3`

表示：

> 使用 USART3。

### `data`

表示：

> 从哪里拿准备发送的数据。

### `length`

表示：

> 发送多少个字节。

### `HAL_MAX_DELAY`

表示：

> 如果发送还没完成，就允许一直等待。

因此这是一个比较简单的：

> **Blocking / 阻塞式发送。**

学习和普通调试非常方便。

------

# 十、周期输出 Counter

随后将一次发送升级成：

```text
Counter = 0
Counter = 1
Counter = 2
Counter = 3
...
```

主要使用：

```c
snprintf()
```

将数字转换成字符串。

例如：

```c
snprintf(tx_buffer,
         sizeof(tx_buffer),
         "Counter = %lu\r\n",
         counter);
```

如果：

```text
counter = 15
```

则产生：

```text
Counter = 15
```

然后再使用：

```c
HAL_UART_Transmit()
```

发送。

------

# 十一、`\r\n` 是什么意思

串口输出中经常看到：

```c
"\r\n"
```

其中：

```text
\r
→ 回到当前行开头

\n
→ 换到下一行
```

组合使用后：

```text
Counter = 1
Counter = 2
Counter = 3
```

可以每条数据显示在新的一行。

------

# 十二、为什么后来不再使用 HAL_Delay(1000)

最开始可以这样：

```c
发送串口
HAL_Delay(1000);
```

即：

```text
发送
↓
CPU等待1秒
↓
发送
↓
CPU再等待1秒
```

它的优点是：

> 简单，非常适合初学验证。

但缺点也很明显：

> CPU 在等待期间无法高效处理其他工作。

因此后面开始改成：

```text
硬件 Timer 负责计时
```

而不是靠：

```c
HAL_Delay()
```

完成所有周期任务。

------

# 十三、TIM6 为什么适合这次实验

本次使用：

```text
TIM6
```

而没有使用：

```text
TIM1
```

原因：

TIM6 是：

> **Basic Timer，基本定时器。**

非常适合：

```text
周期计时
产生中断
建立时间基准
```

而：

```text
TIM1
```

属于功能很强的高级定时器。

以后准备留给：

```text
三相PWM
互补输出
死区
PMSM FOC
```

所以目前不用 TIM1 做普通 1 ms 定时。

------

# 十四、TIM6 的 1 ms 是怎么算出来的

当前：

```text
TIM6 Clock = 170 MHz
```

设置：

```text
Prescaler = 169

Counter Period = 999
```

------

## 第一级：Prescaler

定时器计数频率：

fcounter=fTIMPSC+1f_{counter} = \frac{f_{TIM}}{PSC+1}

所以：

fcounter=170MHz169+1=1MHzf_{counter} = \frac{170MHz}{169+1} = 1MHz

也就是：

> Counter 每 `1 μs` 加 1。

------

## 第二级：ARR

Counter：

```text
0
1
2
...
999
```

总共：

```text
1000 次
```

所以：

1000×1μs=1ms1000\times1\mu s = 1ms

最终 TIM6：

> **每 1 ms 产生一次 Update Event。**

------

# 十五、Timer 最重要的公式

这个以后应该逐渐记熟：

T=(PSC+1)(ARR+1)fTIM\boxed{ T= \frac{(PSC+1)(ARR+1)} {f_{TIM}} }

其中：

```text
PSC
→ Prescaler

ARR
→ Auto Reload Register

f_TIM
→ Timer输入时钟
```

本次：

T=(169+1)(999+1)170×106=0.001sT= \frac{(169+1)(999+1)} {170\times10^6} = 0.001s

即：

T=1msT=1ms

------

# 十六、Prescaler 是什么

Prescaler：

> **预分频器。**

作用：

> 把进入 Timer 的高速时钟降低。

本次：

```text
170 MHz
↓
÷170
↓
1 MHz
```

所以才容易按照：

```text
1 μs
```

为单位计数。

需要特别记住：

> STM32 Timer 的实际分频通常是 `PSC + 1`。

所以：

```text
PSC = 169
```

不是：

```text
÷169
```

而是：

```text
÷170
```

------

# 十七、ARR 是什么

ARR：

> **Auto Reload Register**

可以简单理解成：

> Timer 计数计到哪里以后重新开始。

例如：

```text
ARR = 999
```

Counter：

```text
0 → 1 → 2 → ... → 999
```

随后产生 Update Event，然后重新开始。

因此实际有：

```text
999 + 1 = 1000
```

次计数。

这也是公式里为什么是：

ARR+1ARR+1

而不是单纯 ARR。

------

# 十八、Interrupt 是什么

Interrupt：

> **中断。**

可以理解成：

> 某个重要硬件事件发生时，CPU 暂时去处理这个事件，然后再继续原来的程序。

本次：

```text
TIM6计满
↓
Update Event
↓
产生Interrupt
↓
CPU响应
↓
执行定时器中断代码
↓
返回原程序
```

这比：

```text
while(1)里不停问Timer到没到
```

更加适合实时系统。

------

# 十九、NVIC 是什么

本次在：

```text
TIM6
→ NVIC Settings
```

打开：

```text
TIM6 global interrupt
```

NVIC：

> **Nested Vectored Interrupt Controller**

可以简单理解为：

> Cortex-M 内核里的中断管理器。

它负责：

```text
哪个中断允许进入
中断优先级
多个中断同时到来时谁先执行
```

现阶段先知道：

> Timer 配好以后，如果想让 CPU 真正响应中断，还需要使能对应的 NVIC Interrupt。

------

# 二十、初始化 ≠ 启动

CubeMX 生成：

```c
MX_TIM6_Init();
```

只表示：

> 按照 `.ioc` 参数配置 TIM6。

并不表示：

> TIM6 已经开始计数。

真正启动：

```c
HAL_TIM_Base_Start_IT(&htim6);
```

这里：

```text
Start
→ 启动

IT
→ Interrupt
```

所以这句就是：

> **启动 TIM6，并以中断方式工作。**

这个区别很重要。

以后：

```text
Timer
PWM
ADC
DMA
```

很多外设都会存在：

```text
初始化
≠
启动
```

------

# 二十一、第一次 Timer 回调函数

本次使用：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        timer_counter++;
    }
}
```

它的作用：

```text
TIM6每1ms产生一次中断
↓
HAL处理中断
↓
调用HAL_TIM_PeriodElapsedCallback()
↓
判断是不是TIM6
↓
timer_counter++
```

所以理论上：

```text
1 ms     → +1
100 ms   → +100
1000 ms  → +1000
```

实际通过 Debug 的：

```text
Expressions / Watch
```

观察 `timer_counter`，已经验证正常。

------

# 二十二、为什么 timer_counter 使用 volatile

使用：

```c
volatile uint32_t timer_counter;
```

原因是：

> 这个变量会在中断中发生变化。

`volatile` 告诉编译器：

> 不要假设这个变量不会突然改变，每次使用都要真正读取它当前的值。

典型使用场景：

```text
ISR中修改的变量
DMA修改的数据
硬件寄存器
```

以后 STM32 代码里会经常看到 `volatile`。

------

# 二十三、为什么不要在 Timer ISR 里直接发送 UART

一种看起来很简单的写法：

```c
void HAL_TIM_PeriodElapsedCallback(...)
{
    HAL_UART_Transmit(...);
}
```

小程序可能能够运行。

但是不推荐。

因为：

```c
HAL_UART_Transmit()
```

可能需要等待串口发送完成。

如果 ISR 中做很多耗时操作：

```text
进入ISR
↓
发送大量UART数据
↓
长时间占用CPU
↓
其他Interrupt等待
↓
实时性下降
```

以后做 FOC 时尤其危险。

所以要逐渐养成：

> **ISR 尽量短。**

------

# 二十四、今天第一次使用 Flag 思想

最终没有让 ISR 直接负责串口发送。

而是：

```c
volatile uint8_t uart_send_flag = 0;
```

Timer ISR 中：

```text
每1ms
↓
timer_counter++
↓
到1000
↓
uart_send_flag = 1
↓
退出ISR
```

然后：

```text
while(1)
↓
检查uart_send_flag
↓
发现等于1
↓
清零Flag
↓
执行UART发送
```

程序结构变成：

```text
          TIM6
            ↓
        1 ms中断
            ↓
        更新时间
            ↓
        设置Flag
            ↓
          返回


        while(1)
            ↓
         检查Flag
            ↓
         执行UART
```

这比直接：

```text
ISR里面做所有事情
```

更合理。

------

# 二十五、中断和主循环开始分工

这是今天最重要的程序结构认识之一。

现在已经开始形成：

### 中断

负责：

```text
实时
短
快速
确定性较强
```

例如：

```text
计数
采样完成
设置标志
快速控制计算
```

### while(1)

负责：

```text
低速任务
通信
状态管理
后台处理
```

以后 PMSM FOC 很可能进一步发展成：

```text
高速 ADC/PWM ISR
→ FOC电流环

较低频 Timer
→ 速度环

while(1)
→ 通信
→ 状态机
→ 故障管理
```

这个思想比记某一个 HAL 函数更重要。

------

# 二十六、Polling、Interrupt、DMA 目前怎么区分

现在已经开始接触前两种。

### Polling / Blocking

CPU 自己执行并等待。

例如：

```c
HAL_UART_Transmit();
```

优点：

> 简单。

适合：

```text
基础实验
普通调试
低频任务
```

------

### Interrupt

硬件事件发生以后通知 CPU：

```text
事件
↓
Interrupt
↓
CPU执行ISR
```

优点：

> 不需要 CPU 一直轮询等待。

例如今天：

```text
TIM6每1ms产生中断
```

------

### DMA

还没正式学习。

先知道：

> DMA 可以帮助外设和 RAM 搬数据，减少 CPU 参与。

后面 ADC 会重点接触。

------

# 二十七、今天最重要的一条时间链

建议以后能直接画出来：

```text
170 MHz Timer Clock
        ↓
    PSC = 169
        ↓
      1 MHz
        ↓
 Counter每1μs +1
        ↓
    ARR = 999
        ↓
     1000次
        ↓
       1 ms
        ↓
 Update Event
        ↓
 Interrupt
        ↓
      NVIC
        ↓
      CPU
        ↓
Callback / ISR
```

如果这个链条真正理解了：

> Timer 的基础原理基本就建立起来了。

------

# 二十八、今天已经开始和以后 FOC 接轨

今天虽然只是：

```text
1 ms Timer
+
UART
```

但以后非常类似。

例如：

```text
PWM Timer
↓
20 kHz
↓
每50 μs一次
↓
触发ADC
↓
ADC完成中断
↓
执行FOC
↓
更新PWM Duty
```

所以今天学的：

```text
Clock
Timer
PSC
ARR
Interrupt
NVIC
ISR
```

不是无关紧要的 MCU 基础。

这些以后都会直接进入 PMSM 电机控制。

------

# 二十九、今天必须记住的专业知识

不用背所有代码，但下面这些最好逐渐做到能直接解释。

### UART

```text
TX / RX / GND
TX接RX
RX接TX
必须共地
```

### 115200 8N1

```text
115200波特率
8数据位
无校验
1停止位
```

### Timer

> 根据时钟不断计数的硬件外设。

### Prescaler

> 对 Timer 输入时钟进行预分频。

### ARR

> 决定 Counter 计到多少重新开始。

### Timer 周期公式

T=(PSC+1)(ARR+1)fTIMT= \frac{(PSC+1)(ARR+1)} {f_{TIM}}

### Interrupt

> 硬件事件到来时让 CPU 暂时处理事件。

### NVIC

> Cortex-M 的中断管理器。

### ISR

> Interrupt Service Routine，中断服务程序。

### volatile

> 变量可能被中断、硬件等异步修改，避免编译器错误优化。

### ISR 原则

> **尽量短、快，不做不必要的阻塞操作。**

------

# 三十、今天的实际成果检查

目前工程已经完成：

```text
GPIO LED                    ✅

USART3                      ✅
115200 8N1                  ✅

USB-TTL                     ✅
TX/RX/GND连接               ✅

电脑COM口                    ✅
串口助手                     ✅

Hello Motor Control         ✅

Counter周期发送              ✅

TIM6                        ✅

TIM6 Clock                  170 MHz

Prescaler                   169

ARR                         999

Timer周期                   1 ms

NVIC Interrupt              ✅

HAL_TIM_Base_Start_IT       ✅

Timer Callback              ✅

timer_counter Debug验证      ✅

Timer + UART Flag机制        ✅
```

------

# 三十一、目前工程结构已经发生了什么变化

最开始只是：

```text
MCU
↓
GPIO
↓
LED
```

现在已经变成：

```text
                 STM32G431
                     │
          ┌──────────┴──────────┐
          │                     │
        GPIO                  USART3
          │                     │
         LED                  USB-TTL
                                │
                                PC

                     │
                    TIM6
                     │
                  1 ms Tick
                     │
                  Interrupt
                     │
                   Flag
                     │
                  while(1)
                     │
                   UART
```

这已经开始有真正嵌入式应用程序的样子了。

------

# 三十二、今天最重要的 10 个结论

如果以后忘了具体代码，至少把这些记住：

1. **UART 基本连线是 TX→RX、RX→TX，并且必须共地。**
2. **115200 8N1 是常见串口配置。**
3. **USB-TTL 用于电脑 USB 与 MCU TTL 串口之间转换。**
4. **Timer 本质上是靠时钟计数。**
5. **Prescaler 用来降低 Timer 的计数频率。**
6. **ARR 决定计数周期。**
7. **Timer 周期公式是 (PSC+1)(ARR+1)/fTIM(PSC+1)(ARR+1)/f_{TIM}。**
8. **配置 Timer 不等于启动 Timer。**
9. **Interrupt 发生后 CPU 会执行 ISR / Callback。**
10. **ISR 应尽量短，耗时低速任务可以通过 Flag 交给主循环。**

------

## Day 2 最终验收

现在如果能够回答下面这些问题，今天的内容就基本掌握了：

> USART3 的 TX 和 RX 为什么要交叉连接？

> 为什么 UART 一定要共地？

> 115200 8N1 分别是什么意思？

> TIM6 的 `PSC=169、ARR=999` 为什么得到 1 ms？

> 为什么实际分频是 `PSC+1`？

> ARR 为什么也要 `+1`？

> NVIC 是干什么的？

> `HAL_TIM_Base_Start_IT()` 和 `MX_TIM6_Init()` 有什么区别？

> `volatile` 为什么常用于 ISR 共享变量？

> 为什么不建议在 1 ms Timer ISR 里直接大量发送 UART？

如果这些能够自己讲明白，今天的 **UART + Timer + Interrupt** 就算真正学到了，而不仅仅是“程序跑出来了”。

下一阶段可以正式进入 **PWM**：从 Timer 产生固定频率和占空比的 PWM 开始，然后逐渐走向 TIM1、三相 PWM、互补输出和 Dead Time。这将是从普通 STM32 学习真正进入 PMSM 电机控制硬件基础的关键一步。