# STM32G431 学习笔记 02

## USART3 串口通信 + TIM6 1 ms 定时中断

## 1. 本阶段完成的内容

本阶段完成了：

- USART3 异步串口配置
- USB 转 TTL 模块与 STM32 实机连接
- 串口助手成功接收 STM32 数据
- `Hello Motor Control` 字符串发送
- `Counter = xxx` 周期输出
- TIM6 基础定时器配置
- TIM6 产生 1 ms 周期中断
- 使用 `timer_counter` 验证 1 ms 时间基准
- 使用 `uart_send_flag` 将 Timer 中断和 UART 主循环任务连接起来
- 实现 UART 每 1 s 输出一次数据，同时不再依赖 `HAL_Delay(1000)`

整体流程：

```text
USART3
↓
STM32 TX / RX
↓
USB-TTL
↓
电脑 COM 口
↓
串口助手
```

以及：

```text
170 MHz Timer Clock
↓
TIM6
↓
PSC
↓
ARR
↓
每 1 ms 产生一次中断
↓
timer_counter++
↓
1000 次中断
↓
uart_send_flag = 1
↓
while(1) 执行 UART 发送
```

---

# 2. UART 是什么

UART 全称：

**Universal Asynchronous Receiver/Transmitter**

中文：

**通用异步收发器**

UART 是 MCU 中非常常用的一种串行通信方式。

最基本的 UART 通信通常只需要：

```text
TX
RX
GND
```

其中：

- TX：Transmit，发送
- RX：Receive，接收
- GND：共同的电压参考地

---

# 3. UART 的连接原则

两个 UART 设备连接时：

```text
STM32 TX  →  USB-TTL RX

STM32 RX  ←  USB-TTL TX

STM32 GND ↔  USB-TTL GND
```

需要记住：

> **TX 接对方 RX，RX 接对方 TX，并且必须共地。**

不能：

```text
TX → TX
RX → RX
```

否则无法正常通信。

---

# 4. 为什么一定要共地

UART 的高低电平必须有共同的电压参考。

例如 STM32 输出：

```text
HIGH ≈ 3.3 V
LOW  ≈ 0 V
```

USB-TTL 必须知道：

> 这个 3.3 V 和 0 V 是相对于哪个参考点而言。

所以必须：

```text
STM32 GND
↕
USB-TTL GND
```

如果没有共地，两个设备对高低电平的参考可能不同，容易导致通信失败。

---

# 5. 当前 USART3 配置

当前使用：

```text
USART3
Mode = Asynchronous
```

参数：

```text
Baud Rate   = 115200
Word Length = 8 Bits
Parity      = None
Stop Bits   = 1
Direction   = TX + RX
Flow Control = Disable
```

也就是常见的：

```text
115200 8N1
```

---

# 6. 115200 8N1 的含义

## 115200

表示：

**Baud Rate，波特率**

当前：

```text
115200 bit/s
```

可以简单理解为：

> 串口每秒传输的符号速度。

目前阶段看到：

```text
9600
115200
```

应该知道它们是在描述 UART 通信速度。

---

## 8

表示：

```text
8 Data Bits
```

即一次 UART 数据帧中包含 8 位有效数据。

---

## N

表示：

```text
No Parity
```

即：

> 不使用奇偶校验位。

---

## 1

表示：

```text
1 Stop Bit
```

即：

> 一帧数据结束后使用 1 个停止位。

---

因此：

```text
115200 8N1
```

表示：

```text
Baud Rate = 115200
Data Bits = 8
Parity    = None
Stop Bits = 1
```

---

# 7. UART 和 USART 的区别

UART：

**Universal Asynchronous Receiver/Transmitter**

主要支持：

```text
Asynchronous
```

即异步通信。

USART：

**Universal Synchronous/Asynchronous Receiver/Transmitter**

可以支持：

```text
Synchronous
+
Asynchronous
```

当前使用：

```text
USART3
↓
Asynchronous
```

所以当前实际上就是把 USART3 当普通 UART 使用。

---

# 8. 当前 USART3 引脚

当前开发板使用：

```text
PB10 → USART3_TX
PB11 → USART3_RX
```

与 USB-TTL 连接：

```text
PB10 / USART3_TX → USB-TTL RX

PB11 / USART3_RX ← USB-TTL TX

GND              ↔ USB-TTL GND
```

---

# 9. 第一次 UART 发送

最开始进行最简单的字符串发送：

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

说明以下链路正常：

```text
STM32 USART3
↓
GPIO TX
↓
USB-TTL
↓
USB
↓
Windows COM
↓
串口助手
```

---

# 10. HAL_UART_Transmit()

基本形式：

```c
HAL_UART_Transmit(&huart3,
                  data,
                  size,
                  timeout);
```

当前最重要的是理解四个参数。

---

## `&huart3`

表示：

> 使用 USART3。

---

## `data`

表示：

> 要发送的数据。

例如：

```c
message
```

或者：

```c
(uint8_t *)tx_buffer
```

---

## `size`

表示：

> 本次发送多少个字节。

例如：

```c
sizeof(message) - 1
```

---

## `timeout`

例如：

```c
HAL_MAX_DELAY
```

表示：

> 等待 UART 发送完成时允许的最大等待时间。

---

# 11. 为什么字符串末尾使用 `\r\n`

例如：

```c
"Hello Motor Control\r\n"
```

其中：

```text
\r
```

表示：

**Carriage Return**

即：

> 回到当前行开头。

而：

```text
\n
```

表示：

**New Line**

即：

> 换到下一行。

串口输出中通常组合使用：

```c
"\r\n"
```

这样串口助手显示时会一行一行排列。

---

# 12. Counter 串口实验

为了验证 UART 能持续发送动态数据，使用：

```c
uint32_t counter = 0;
char tx_buffer[50];
```

主循环中：

```c
int len = snprintf(tx_buffer,
                   sizeof(tx_buffer),
                   "Counter = %lu\r\n",
                   counter);

HAL_UART_Transmit(&huart3,
                  (uint8_t *)tx_buffer,
                  len,
                  HAL_MAX_DELAY);

counter++;

HAL_Delay(1000);
```

串口显示：

```text
Counter = 0
Counter = 1
Counter = 2
Counter = 3
...
```

---

# 13. snprintf() 的作用

例如：

```c
snprintf(tx_buffer,
         sizeof(tx_buffer),
         "Counter = %lu\r\n",
         counter);
```

作用：

> 把变量转换成字符串，并写入 `tx_buffer`。

例如：

```text
counter = 15
```

经过 `snprintf()` 后：

```text
tx_buffer
↓
"Counter = 15\r\n"
```

然后 UART 才能把这段字符发送到电脑。

---

# 14. 为什么需要 tx_buffer

UART 最终发送的是：

> 一个字节一个字节的数据。

而：

```c
counter
```

本身是整数。

不能直接把：

```text
15
```

当成字符串：

```text
"15"
```

发送。

所以先：

```text
数值
↓
snprintf()
↓
字符缓冲区
↓
UART
```

例如：

```c
char tx_buffer[50];
```

就是一个用于临时保存待发送字符串的缓冲区。

---

# 15. 为什么 `(uint8_t *)tx_buffer`

`tx_buffer` 定义：

```c
char tx_buffer[50];
```

而：

```c
HAL_UART_Transmit()
```

需要的数据类型类似：

```c
uint8_t *
```

因此使用：

```c
(uint8_t *)tx_buffer
```

表示：

> 将这块字符数据按字节数据交给 UART 发送。

当前阶段知道这个用途即可，不需要继续深入类型转换底层细节。

---

# 16. 为什么不能长期依赖 HAL_Delay()

最开始使用：

```c
HAL_Delay(1000);
```

可以实现：

```text
发送
↓
等待 1 秒
↓
发送
↓
等待 1 秒
```

这种方法非常适合基础测试。

但存在问题：

```text
HAL_Delay()
↓
CPU 等待
```

如果以后做电机控制，例如：

```text
20 kHz FOC
```

一个控制周期只有：

$$
T=\frac{1}{20000}
$$

即：

$$
T=50\mu s
$$

这时显然不能在控制程序中随便：

```c
HAL_Delay(1000);
```

所以后面使用硬件 Timer 建立周期任务。

---

# 17. Timer 是什么

Timer：

**定时器**

可以简单理解为：

> 根据输入时钟不断自动计数的硬件外设。

例如：

```text
0
1
2
3
4
...
```

当计数达到设定值以后，Timer 可以：

- 产生中断
- 产生 PWM
- 触发 ADC
- 测量信号周期
- 测量脉宽

Timer 是以后电机控制非常重要的外设。

---

# 18. 为什么选择 TIM6

当前使用：

```text
TIM6
```

做 1 ms 基础定时中断。

TIM6 属于基础定时器，非常适合：

> 提供周期性的时间基准。

同时不需要占用 GPIO。

这样可以把更重要的：

```text
TIM1
```

留给以后做：

```text
三相 PWM
互补 PWM
Dead Time
FOC
```

---

# 19. TIM6 当前配置

当前参数：

```text
Timer Clock = 170 MHz

Prescaler = 169

Counter Period = 999
```

也就是：

```text
PSC = 169
ARR = 999
```

---

# 20. Prescaler 的作用

Prescaler：

**预分频器**

实际分频系数：

$$
PSC+1
$$

当前：

$$
PSC=169
$$

所以实际分频：

$$
169+1=170
$$

Timer 输入时钟：

$$
170\text{ MHz}
$$

经过分频：

$$
f_{CNT}
=
\frac{170\text{ MHz}}{170}
$$

得到：

$$
f_{CNT}=1\text{ MHz}
$$

因此 Counter 每计数一次需要：

$$
T_{CNT}=\frac{1}{1\text{ MHz}}
$$

即：

$$
T_{CNT}=1\mu s
$$

所以：

> TIM6 每 1 μs 加 1。

---

# 21. ARR：Counter Period

当前：

```text
ARR = 999
```

Timer 会：

```text
0
1
2
...
999
```

一共计数：

$$
999+1=1000
$$

次。

每次：

$$
1\mu s
$$

因此总时间：

$$
1000\times1\mu s
$$

得到：

$$
\boxed{1\text{ ms}}
$$

所以 TIM6 每 1 ms 产生一次 Update Event。

---

# 22. Timer 周期计算公式

需要开始熟悉：

$$
T
=
\frac{(PSC+1)(ARR+1)}
{f_{TIM}}
$$

当前：

$$
PSC=169
$$

$$
ARR=999
$$

$$
f_{TIM}=170\times10^6
$$

代入：

$$
T
=
\frac{(169+1)(999+1)}
{170\times10^6}
$$

得到：

$$
T=0.001\text{ s}
$$

即：

$$
\boxed{T=1\text{ ms}}
$$

对应频率：

$$
f=1000\text{ Hz}
$$

---

# 23. Timer Init 和 Start 的区别

CubeMX 自动生成：

```c
MX_TIM6_Init();
```

只是：

> 配置 TIM6。

例如配置：

- PSC
- ARR
- Counter Mode

但 Timer 不会因此自动开始工作。

还需要：

```c
HAL_TIM_Base_Start_IT(&htim6);
```

才能：

> 启动 TIM6，并使用中断方式运行。

因此继续记住：

```text
MX_TIM6_Init()
↓
配置

HAL_TIM_Base_Start_IT()
↓
启动
```

也就是：

> **Init ≠ Start**

---

# 24. Interrupt 是什么

Interrupt：

**中断**

可以简单理解为：

> 某个重要事件发生后，硬件通知 CPU 暂时处理这个事件。

例如 TIM6：

```text
Timer计数
↓
达到ARR
↓
产生Update Event
↓
产生Interrupt
↓
CPU处理Timer事件
↓
处理完成
↓
继续原来的程序
```

---

# 25. NVIC 是什么

NVIC 全称：

**Nested Vectored Interrupt Controller**

可以理解成：

> Cortex-M MCU 中负责管理中断的控制器。

它负责：

- 哪些中断允许执行
- 中断优先级
- 多个中断同时发生时先处理谁

当前 TIM6 需要开启：

```text
TIM6 Global Interrupt
```

这样 Timer 产生中断以后 CPU 才能够响应。

---

# 26. ISR 是什么

ISR 全称：

**Interrupt Service Routine**

中文：

**中断服务程序**

可以简单理解成：

> 中断发生后 CPU 执行的代码。

当前最终使用 HAL 的 Timer 回调：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        timer_counter++;
    }
}
```

表示：

```text
TIM6周期到
↓
HAL处理Timer中断
↓
调用PeriodElapsedCallback
↓
timer_counter++
```

---

# 27. timer_counter 验证实验

定义：

```c
volatile uint32_t timer_counter = 0;
```

Timer 回调：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        timer_counter++;
    }
}
```

因为 TIM6：

```text
1 ms 中断一次
```

所以：

```text
1 ms     → timer_counter ≈ 1

100 ms   → timer_counter ≈ 100

1000 ms  → timer_counter ≈ 1000

5 s      → timer_counter ≈ 5000
```

可以通过 Debugger 的 Expressions 查看：

```text
timer_counter
```

从而验证 Timer 确实按照约 1 ms 周期运行。

---

# 28. 为什么 timer_counter 使用 volatile

定义：

```c
volatile uint32_t timer_counter;
```

原因：

> `timer_counter` 会在中断中被异步修改。

主程序并不能通过普通代码流程判断它什么时候变化。

所以使用：

```c
volatile
```

告诉编译器：

> 这个变量可能在当前代码之外发生变化，不要随意优化掉对它的读取。

以后常见使用场景包括：

- ISR 与主循环共享变量
- DMA 更新变量
- 硬件寄存器
- 某些异步状态量

---

# 29. 为什么 ISR 应该尽量短

一种看似简单的写法是：

```c
void HAL_TIM_PeriodElapsedCallback(...)
{
    HAL_UART_Transmit(...);
}
```

但这并不推荐。

因为 UART 发送需要时间。

如果 ISR 中做大量工作：

```text
进入中断
↓
UART慢慢发送
↓
CPU长时间停留在ISR
↓
其他实时任务等待
```

以后可能影响：

- ADC
- PWM
- 电流采样
- FOC
- 其他中断

因此非常重要的工程原则是：

> **ISR 尽量短、尽量快。**

---

# 30. 使用 Flag 将 ISR 和主循环分离

最终使用：

```c
volatile uint8_t uart_send_flag = 0;
```

TIM6 回调：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        timer_counter++;

        if (timer_counter >= 1000)
        {
            timer_counter = 0;
            uart_send_flag = 1;
        }
    }
}
```

这里：

```text
TIM6
↓
每 1 ms 中断一次
↓
timer_counter++
↓
达到 1000
↓
约 1 秒
↓
uart_send_flag = 1
```

---

# 31. 主循环处理 UART

在：

```c
while (1)
```

中：

```c
if (uart_send_flag)
{
    uart_send_flag = 0;

    seconds_counter++;

    int len = snprintf(tx_buffer,
                       sizeof(tx_buffer),
                       "Seconds = %lu\r\n",
                       seconds_counter);

    HAL_UART_Transmit(&huart3,
                      (uint8_t *)tx_buffer,
                      len,
                      HAL_MAX_DELAY);
}
```

最终结构：

```text
TIM6 ISR
↓
只计时 + 设置 Flag
↓
立即退出中断

while(1)
↓
发现 Flag
↓
执行 UART 发送
```

这比直接在中断里面发送 UART 更合理。

---

# 32. Flag 模式的意义

这种程序结构可以理解成：

```text
Interrupt
↓
只通知“有事情发生”
↓
Flag = 1

Main Loop
↓
检查 Flag
↓
执行较慢的任务
```

因此：

```text
中断
→ 负责实时触发

while(1)
→ 负责普通低速任务
```

这是嵌入式软件中非常常见的一种思想。

---

# 33. HAL_Delay 和 Timer 中断的区别

最开始：

```c
HAL_UART_Transmit(...);

HAL_Delay(1000);
```

程序结构：

```text
UART发送
↓
CPU等待1000ms
↓
UART发送
```

后来：

```text
TIM6每1ms自动计时
↓
1000次后设置Flag
↓
主循环处理UART
```

主要区别是：

## HAL_Delay

```text
CPU主动等待时间过去
```

## Hardware Timer

```text
Timer硬件自己计时
CPU可以同时做其他事情
```

因此后者更加接近真实嵌入式系统。

---

# 34. UART 在以后电机控制中的作用

UART 不仅仅是：

> 发送 Hello World。

以后可以作为非常重要的 Debug 通道。

例如输出：

```text
speed = 1000 rpm

id = 0.02 A

iq = 1.85 A

Vdc = 24.1 V

state = RUN

fault = 0
```

通过电脑观察 MCU 内部实时变量。

因此 UART 对以后调试：

- ADC
- 电流
- 速度
- PI
- FOC
- Fault

都非常有价值。

---

# 35. Timer 在以后 FOC 中的作用

当前 TIM6 只是：

```text
1 ms Timer
```

但以后 Timer 会承担更关键的工作：

```text
Timer
↓
产生PWM
↓
同步ADC
↓
触发Control ISR
↓
执行FOC
```

最终可能形成不同时间尺度：

```text
20 kHz
↓
FOC / Current Loop

1 kHz
↓
Speed Loop

100 Hz
↓
State Management

10 Hz
↓
UART / Monitoring
```

因此：

> Timer 是实时电机控制系统的核心时间基础之一。

---

# 36. 本阶段最需要记住的知识

1. UART 最基本信号：

   ```text
   TX
   RX
   GND
   ```

2. UART 连接：

   ```text
   TX → RX
   RX → TX
   GND ↔ GND
   ```

3. 当前串口参数：

   ```text
   115200 8N1
   ```

4. `115200 8N1`：

   ```text
   115200 → Baud Rate
   8      → 8 Data Bits
   N      → No Parity
   1      → 1 Stop Bit
   ```

5. `HAL_UART_Transmit()`：

   > UART 阻塞式发送。

6. `\r\n`：

   > 串口换行常用组合。

7. Timer 本质：

   > 根据时钟自动计数的硬件外设。

8. Timer 周期：

   $$
   T
   =
   \frac{(PSC+1)(ARR+1)}
   {f_{TIM}}
   $$

9. 当前 TIM6：

   ```text
   Timer Clock = 170 MHz
   PSC = 169
   ARR = 999
   ```

   得到：

   $$
   T=1\text{ ms}
   $$

10. Interrupt：

    > 硬件事件发生后通知 CPU 处理。

11. ISR：

    > 中断发生后执行的程序。

12. NVIC：

    > MCU 的中断管理器。

13. ISR 原则：

    > **尽量短、尽量快。**

14. `volatile`：

    > 用于可能被中断、DMA、硬件等异步修改的变量。

15. 使用 Flag：

    ```text
    ISR
    ↓
    设置 Flag

    while(1)
    ↓
    执行低速任务
    ```

16. `Init` 和 `Start` 不一样：

    ```text
    MX_TIM6_Init()
    → 配置
    
    HAL_TIM_Base_Start_IT()
    → 启动
    ```

---

# 37. 与后续学习的关系

这一阶段建立了：

```text
UART
↓
电脑通信 / Debug

Timer
↓
系统时间基准

Interrupt
↓
实时事件处理

Flag
↓
ISR和主循环任务分离
```

这些知识以后会继续发展成：

```text
Timer
↓
PWM
↓
ADC Trigger
↓
ADC + DMA
↓
Current Sampling
↓
Control ISR
↓
FOC
```

因此这一阶段最重要的不是记住几个 HAL 函数，而是理解：

> **MCU 可以让硬件 Timer 自动计时，通过 Interrupt 触发实时事件，再利用 Flag 将高速事件和低速任务分开。**