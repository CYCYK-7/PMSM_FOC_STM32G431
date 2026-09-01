# STM32G431 学习笔记 01

## 从新建工程到 GPIO 实机运行 + USART3 初步配置

------

## 一、今天实际完成了什么

今天完整走通了第一条真正的 STM32 开发链：

```text
选择 MCU
   ↓
创建 STM32CubeIDE 工程
   ↓
.ioc 配置
   ↓
系统时钟配置
   ↓
GPIO 配置
   ↓
CubeMX 自动生成代码
   ↓
Build
   ↓
ST-LINK
   ↓
Download
   ↓
MCU 运行
   ↓
LED 实机验证成功
```

这条流程非常重要。

以后无论你做的是：

```text
LED
UART
PWM
ADC
Encoder
FOC
电流环
速度环
Sensorless
```

底层开发流程其实都不会脱离：

```text
配置 → 生成代码 → 写业务代码 → Build → Download → Debug
```

这也是原半年计划要求你真正建立的 STM32 开发闭环。

你今天还额外开始配置了：

```text
USART3
115200
8N1
Asynchronous
```

但 USART3 **还没有完成电脑端串口实机验证**，所以这个功能目前属于“已配置、待验证”。

------

# 二、以后重新建立 STM32G431 工程怎么做

## 1. 创建工程

STM32CubeIDE：

```text
File
→ New
→ STM32 Project
```

进入 `Target Selection`。

选择：

```text
MCU/MPU Selector
```

而不是 Board Selector。

原因是你的电机控制板不是 ST 官方 NUCLEO 开发板，而是以具体 MCU 为核心设计的板子。

搜索：

```text
STM32G431CBU6
```

你这块板使用的 MCU 就是：

```text
STM32G431CBU6
```

这里有一个非常重要的工程习惯：

> **芯片型号不要靠“差不多”选择，一定查原理图、芯片丝印或开发板资料。**

例如：

```text
STM32G431CBU6
STM32G431CBT6
STM32G431RBT6
```

看起来很像，但封装、Flash、引脚数量等可能不同。

以后工作中选错 MCU 型号，很可能直接导致：

```text
CubeMX 引脚不一致
外设找不到
烧录异常
链接脚本错误
Flash/RAM 配置错误
```

------

# 三、工程创建页面怎么选

今天使用的是：

| 选项                  | 设置            |
| --------------------- | --------------- |
| Project Name          | `PMSM_FOC_G431` |
| Targeted Language     | `C`             |
| Targeted Binary Type  | `Executable`    |
| Targeted Project Type | `STM32Cube`     |

### 为什么选 C？

嵌入式电机控制里 C 仍然非常重要。

以后你看到的：

```c
HAL_ADC_Start()
HAL_TIM_PWM_Start()
HAL_UART_Transmit()
FOC_Run()
PI_Run()
SVPWM_Run()
```

绝大多数都是 C 接口。

你的目标不是成为纯软件开发程序员，而是：

> 能读、能改、能组织、能 Debug 电机控制 C 程序。

所以 C 足够，而且非常符合岗位需求。

### Executable 是什么？

表示最终生成：

> 可以烧录到 STM32 Flash 中执行的程序。

`Static Library` 是生成静态库，现在不用。

### 为什么选择 STM32Cube？

因为这样 CubeMX 才能帮助你配置：

```text
Clock
GPIO
TIM
ADC
UART
DMA
NVIC
...
```

并自动产生 HAL 初始化代码。

------

# 四、Firmware Package 页面怎么选

今天选择：

```text
STM32Cube FW_G4
```

因为 STM32G431 属于：

```text
STM32G4 Series
```

Code Generator 选择：

```text
Copy only the necessary library files
```

意思是：

> 把当前工程真正需要的 HAL/CMSIS 文件复制进工程。

对于你现阶段非常合适。

------

# 五、`.ioc` 到底是什么

这是今天必须开始真正理解的东西。

例如：

```text
PMSM_FOC_G431.ioc
```

它不是你的主要 C 程序，而是：

> **CubeMX 的硬件配置文件。**

你在 `.ioc` 里面做：

```text
PC4 → GPIO Output
USART3 → Asynchronous
TIM1 → PWM
ADC1 → Analog Input
Clock → 170 MHz
```

CubeMX 再根据这些配置自动生成：

```c
MX_GPIO_Init();
MX_USART3_UART_Init();
MX_TIM1_Init();
MX_ADC1_Init();
```

所以可以这样理解：

```text
.ioc
=
硬件配置说明书
```

而：

```text
main.c
*.c/*.h
=
真正运行的软件
```

------

# 六、今天非常重要的知识：Pin Multiplexing

STM32 一个引脚通常不只是一个功能。

例如某个引脚可能能够作为：

```text
普通 GPIO
UART_TX
TIM_CHx
ADC
SPI
I2C
...
```

这叫：

> **Pin Multiplexing，引脚复用。**

所以 CubeMX 里点击一个 Pin 时，会看到多个功能。

这也是你今天 LED 出问题的根本原因：

> **实际硬件 LED 接在哪个 GPIO，必须根据原理图确定，而不能自己随便选一个 GPIO。**

这是非常典型的嵌入式 Debug 思维。

以后电机不转，你第一反应不能只是：

> “是不是代码算法错了？”

而要逐层确认：

```text
原理图
↓
Pin
↓
CubeMX 配置
↓
初始化代码
↓
HAL调用
↓
真实引脚电平
↓
外围硬件
```

你今天因为“GPIO 引脚选错导致 LED 不工作”，其实就是一次非常标准的硬件软件联合 Debug。

------

# 七、SYS → Serial Wire 是干什么的

在：

```text
System Core
→ SYS
```

配置：

```text
Debug = Serial Wire
```

之后：

```text
PA13 → SWDIO
PA14 → SWCLK
```

会用于：

```text
PC
 ↓
ST-LINK
 ↓
SWD
 ↓
STM32
```

SWD 全称：

> **Serial Wire Debug**

这是 ARM Cortex-M MCU 非常常见的调试接口。

通常最核心的信号就是：

```text
SWDIO
SWCLK
GND
```

再根据情况加入：

```text
NRST
VTref
```

------

# 八、ST-LINK 是什么

这个以后工作必须能说清楚。

不要只理解成：

> “那个拿来烧程序的小东西。”

更专业的理解是：

> **ST-LINK 是 ST 针对 STM32 提供的下载与在线调试工具，通过 SWD/JTAG 与 MCU 通信。**

它至少负责两件重要事情：

```text
Programming
+
Debugging
```

也就是：

### 烧录

把编译得到的 MCU 程序：

```text
PC
↓
ST-LINK
↓
STM32 Flash
```

### 在线调试

允许 IDE：

```text
暂停 MCU
打断点
单步运行
观察变量
查看内存
继续运行
```

以后真正做电机控制时，这个非常重要。

------

# 九、Build 和 Download 必须严格区分

这是面试也可能问到的基础概念。

## Build

```text
.c / .h
 ↓
Compiler
 ↓
.o
 ↓
Linker
 ↓
最终程序
```

可以简单理解成：

> **把你的 C 代码转换成 MCU 能运行的机器程序。**

所以：

```text
Build Success
```

只能说明：

> 软件能够成功编译和链接。

它绝对不意味着：

```text
GPIO正确
硬件正确
LED正确
电机能转
FOC正确
```

你今天亲自验证了这一点：

> Build 完全没有报错，但 GPIO 引脚配错以后 LED 仍然没有动作。

所以一定记住：

Build成功≠功能正确\boxed{\text{Build成功}\neq\text{功能正确}}

这是一条很有价值的工程意识。你原来的学习计划也专门强调了这一点。

## Download

则是：

```text
已经 Build 好的程序
↓
ST-LINK
↓
烧入 STM32 Flash
```

------

# 十、Run 和 Debug 有什么区别

### Run

目标主要是：

> 让 MCU 正常执行程序。

### Debug

则允许你控制 MCU：

```text
Breakpoint
Step Over
Step Into
Resume
Watch
Expressions
Call Stack
```

以后电机控制岗位很看重 Debug 能力。

例如：

```text
电机突然停转
```

一个真正有工程能力的人不会只盯着代码猜。

可能会：

```text
打断点
↓
检查状态机
↓
看 fault_flag
↓
看 ADC current
↓
看 speed
↓
看 PWM duty
↓
找到异常产生的位置
```

所以 Debugger 不是附加功能，而是你的工作工具。

------

# 十一、STM32 时钟系统——今天最值得认真理解的专业知识之一

今天把 STM32G431 配到了：

```text
170 MHz
```

使用：

```text
HSI = 16 MHz
```

然后：

```text
HSI
 ↓
PLLM /4
 ↓
4 MHz
 ↓
PLLN ×85
 ↓
340 MHz
 ↓
PLLR /2
 ↓
170 MHz
```

即：

fSYSCLK=16 MHz4×85÷2=170 MHzf_{\text{SYSCLK}} = \frac{16\,MHz}{4}\times85\div2 = 170\,MHz

------

# 十二、HSI / HSE 是什么

## HSI

High Speed Internal。

就是：

> MCU 内部自带的高速 RC 时钟源。

优点：

```text
不需要外部晶振
启动方便
配置简单
```

缺点是：

> 精度通常不如高质量外部晶振。

## HSE

High Speed External。

就是：

> 外部高速时钟/晶振。

来自 MCU 外面的硬件。

以后做不同产品时，具体选择要根据：

```text
精度
成本
通信要求
硬件设计
EMC
```

综合决定。

------

# 十三、PLL 是什么

PLL：

> **Phase Locked Loop，锁相环。**

你现在阶段可以先把它理解成：

> 把较低的输入时钟经过分频、倍频，再得到我们需要的高频系统时钟。

你的例子：

```text
16 MHz
↓
PLL
↓
170 MHz
```

以后看到：

```text
PLLM
PLLN
PLLR
PLLQ
PLLP
```

不要怕。

核心思想就是：

```text
输入
↓
分频
↓
倍频
↓
再次分频
↓
输出不同用途的时钟
```

------

# 十四、SYSCLK、HCLK、PCLK 是什么

这是工作中非常值得掌握的。

可以先建立这样的框架：

```text
Clock Source
     ↓
    PLL
     ↓
  SYSCLK
     ↓
AHB Prescaler
     ↓
   HCLK
     ↓
 ┌───────────┐
 ↓           ↓
APB1        APB2
 ↓           ↓
PCLK1       PCLK2
```

### SYSCLK

System Clock。

可以理解成：

> 整个 MCU 系统最核心的系统时钟来源。

### HCLK

主要进入：

```text
CPU Core
AHB Bus
Memory
DMA
部分外设
```

### PCLK

Peripheral Clock。

主要给挂在 APB 总线上的外设。

例如：

```text
UART
Timer
SPI
I2C
```

会根据芯片结构挂在 APB1 或 APB2。

------

# 十五、为什么电机控制工程师必须懂时钟

因为以后所有实时控制最终都要落到：

> **时间。**

比如你以后可能设置：

```text
PWM = 20 kHz
```

意味着：

TPWM=120000=50μsT_{PWM}=\frac1{20000}=50\mu s

如果 FOC 每个 PWM 周期执行一次，那么你只有大约：

```text
50 μs
```

完成：

```text
ADC采样
↓
Clarke
↓
Park
↓
PI
↓
InvPark
↓
SVPWM
↓
更新PWM
```

所以：

```text
Clock
→ Timer
→ PWM
→ ADC Trigger
→ Control ISR
→ FOC
```

其实是一条完整的实时控制链。

这也是为什么我们以后学 PWM 之前一定要先把 Clock 搞懂。

------

# 十六、GPIO 是什么

GPIO：

> **General Purpose Input/Output**

通用输入输出。

简单来说：

```text
Input
→ MCU读取外部高低电平

Output
→ MCU向外输出高低电平
```

例如：

```text
LED
继电器
Enable
Fault
按键
```

很多都会使用 GPIO。

------

# 十七、GPIO Output Push-Pull 是什么

你今天 LED 使用的是：

```text
Output Push Pull
```

这是非常常见的输出方式。

Push-Pull 可以主动输出：

```text
HIGH
或者
LOW
```

可以简单理解成 MCU 内部有：

```text
上拉输出管
+
下拉输出管
```

所以：

```text
输出 HIGH
→ 主动拉高

输出 LOW
→ 主动拉低
```

特别适合：

```text
LED
普通数字控制
Chip Enable
```

------

# 十八、Push-Pull 和 Open-Drain 要能区分

以后面试很可能碰到。

### Push-Pull

可以主动输出：

```text
High
Low
```

### Open-Drain

通常：

```text
可以主动拉低
但不能主动拉高
```

需要：

```text
Pull-up resistor
```

才能获得高电平。

典型应用：

```text
I2C
```

所以如果以后面试问：

> “为什么 I2C 通常使用开漏输出？”

至少应该知道：

> 因为多个设备可以共享总线，设备通过拉低总线表示低电平，而高电平由公共上拉电阻产生，可以避免多个设备主动输出相反电平造成冲突。

这个以后我们学 I2C 时再深入。

------

# 十九、Pull-Up / Pull-Down 是什么

GPIO Input 如果什么都没有连接，输入可能处于：

> Floating，悬空。

也就是说：

```text
不是稳定 0
也不是稳定 1
```

可能受到干扰。

于是可以：

```text
Pull-Up
→ 默认拉成 High

Pull-Down
→ 默认拉成 Low
```

你今天 LED 输出：

```text
No Pull
```

通常没有问题，因为 Push-Pull 自己就在主动输出电平。

------

# 二十、为什么 LED 有时候 LOW 才亮

这个非常常见。

如果硬件：

```text
3.3V
 │
电阻
 │
LED
 │
GPIO
```

那么：

```text
GPIO = LOW
```

电流：

```text
3.3V
↓
电阻
↓
LED
↓
GPIO
↓
GND
```

于是 LED 亮。

这种称为：

> **Active Low，低电平有效。**

以后你经常看到：

```text
LED_ON = 0
ENABLE_N
RESET_N
FAULT_N
CS_N
```

名字最后带：

```text
_N
```

很多时候就在表示：

> Low Active。

这是值得形成职业习惯的东西。

------

# 二十一、HAL 是什么

例如你今天使用：

```c
HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4);
```

HAL：

> **Hardware Abstraction Layer**

硬件抽象层。

ST 帮你封装了很多底层寄存器操作。

例如：

```c
HAL_GPIO_WritePin()
HAL_GPIO_TogglePin()
HAL_UART_Transmit()
HAL_ADC_Start()
HAL_TIM_PWM_Start()
```

这样你不用每次直接操作寄存器。

现在你的路线应该是：

```text
先熟练 HAL
↓
理解外设工作原理
↓
有需要再看寄存器/LL
```

而不是一上来背寄存器。

这也符合你目前“能看懂、会用、最终能做工程”的目标。

------

# 二十二、`main()` 的基本结构必须看懂

以后基本都会看到：

```c
int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART3_UART_Init();

    while (1)
    {

    }
}
```

可以理解成：

```text
main
 ↓
初始化 HAL
 ↓
初始化 Clock
 ↓
初始化外设
 ↓
进入 while(1)
 ↓
一直运行
```

而 MCU 和 PC 程序最大的区别之一就是：

> MCU 通常不会正常“运行结束”。

而是：

```text
Power On
↓
Initialization
↓
while(1)
↓
一直工作
↓
Power Off / Reset
```

你之前学习计划里也专门强调了这个 MCU 主循环模型。

------

# 二十三、今天碰到的第二个非常重要的坑：CubeMX 重生成代码

你发现：

> 修改 `.ioc` 后，原来 `main.c` 里的代码消失。

这件事必须牢记。

CubeMX 会管理它生成的文件。

因此你自己的代码优先写在：

```c
/* USER CODE BEGIN xxx */

/* USER CODE END xxx */
```

之间。

例如：

```c
/* USER CODE BEGIN 3 */

HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4);
HAL_Delay(500);

/* USER CODE END 3 */
```

CubeMX 重新生成代码时，通常会保护这些区域。原计划也明确提醒，自己写的程序应该优先放在这些 USER CODE 区域，否则重新 Generate Code 时可能被覆盖。

------

# 二十四、几个常见 USER CODE 区域

### 自己的头文件

```c
/* USER CODE BEGIN Includes */

#include "foc.h"
#include "pi.h"

/* USER CODE END Includes */
```

### 自己的变量

```c
/* USER CODE BEGIN PV */

float iq_ref = 0.0f;

/* USER CODE END PV */
```

### 外设初始化后的代码

```c
/* USER CODE BEGIN 2 */

HAL_TIM_PWM_Start(...);

/* USER CODE END 2 */
```

### while 主循环

```c
/* USER CODE BEGIN 3 */

...

/* USER CODE END 3 */
```

------

# 二十五、以后真正的大工程不要全部写 main.c

这是今天从“初学”走向“工程”的一个非常重要认知。

以后千万不要：

```text
main.c
  3000行
```

里面全都是：

```text
ADC
PWM
PI
Clarke
Park
SVPWM
Encoder
FOC
State Machine
Fault
UART
```

而应该逐渐变成：

```text
main.c
│
├── 初始化
│
├── 系统入口
│
└── 调用功能模块

Control/
├── pi.c
├── transform.c
├── svpwm.c
└── foc.c

Driver/
├── pwm.c
├── adc.c
└── encoder.c
```

这样 CubeMX 再怎么修改：

```text
ADC
TIM
GPIO
UART
```

你的：

```text
PI
SVPWM
FOC
```

这些自己创建的源文件都不会被它覆盖。

这也是你第一周学习 `.c/.h` 模块化真正开始发挥价值的地方。

------

# 二十六、USART3 今天配置到了哪里

今天已经配置：

```text
Connectivity
→ USART3
→ Asynchronous
```

参数：

| 参数                  | 当前设置             |
| --------------------- | -------------------- |
| Baud Rate             | 115200               |
| Word Length           | 8 Bits               |
| Parity                | None                 |
| Stop Bits             | 1                    |
| Data Direction        | Receive and Transmit |
| Hardware Flow Control | Disable              |

也就是经常说：

```text
115200 8N1
```

这个表达以后最好能直接看懂。

------

# 二十七、UART 是什么

UART：

> **Universal Asynchronous Receiver/Transmitter**

通用异步收发器。

最简单情况下：

```text
TX
RX
GND
```

即可通信。

### TX

Transmit：

> 发送。

### RX

Receive：

> 接收。

所以两个设备连接：

```text
STM32 TX ─── USB-TTL RX

STM32 RX ─── USB-TTL TX

STM32 GND ── USB-TTL GND
```

一定要：

> **TX 接 RX，RX 接 TX。**

还必须：

> **共地。**

“共地”是以后硬件调试中非常重要的概念。

------

# 二十八、115200 8N1 是什么意思

```text
115200
```

是 Baud Rate。

可以简单理解成通信速率。

```text
8
```

代表 8 Data Bits。

```text
N
```

代表：

```text
No Parity
```

没有奇偶校验。

```text
1
```

代表：

```text
1 Stop Bit
```

所以：

```text
115200 8N1
```

就是非常常见的 UART 设置。

------

# 二十九、USART 和 UART 为什么名字不一样

STM32 里面你经常看到：

```text
UART
USART
```

USART：

> Universal Synchronous/Asynchronous Receiver/Transmitter

比普通 UART 多了：

```text
Synchronous
```

同步通信能力。

但你今天设置：

```text
USART3
→ Asynchronous
```

所以实际就是当普通 UART 在用。

------

# 三十、UART 为什么对电控软件工程师很重要

UART 不一定承担最终产品最核心通信，但它是非常好的：

> **Debug 通道。**

比如以后电机不正常，可以串口打印：

```text
speed = 1000
iq = 1.82
id = 0.03
Vdc = 24.1
state = RUN
fault = 0
```

这比：

> “电机怎么不转，我猜是不是 PI 参数不对”

专业得多。

所以以后你会逐渐学会：

```text
变量
↓
UART
↓
PC
↓
实时观察
```

后面我们还会升级到：

```text
UART Interrupt
UART DMA
printf 重定向
串口协议
```

------

# 三十一、轮询 / Interrupt / DMA 以后要逐渐区分

今天还没开始，但现在先有概念。

例如：

```c
HAL_UART_Transmit()
```

最简单的方式属于：

> Blocking / Polling 思路。

CPU 会参与等待发送完成。

以后还会看到：

```c
HAL_UART_Transmit_IT()
```

Interrupt：

> 通过中断完成通信。

以及：

```c
HAL_UART_Transmit_DMA()
```

DMA：

> 由 DMA 搬运数据，减少 CPU 负担。

在实时电机控制系统里，不能什么东西都：

```text
CPU一直等
```

因为 CPU 还要执行：

```text
ADC
FOC
PWM
Protection
Communication
```

所以未来你会逐渐形成：

```text
简单测试
→ Polling

异步事件
→ Interrupt

大量/持续数据
→ DMA
```

这种工程判断。

------

# 三十二、今天最有价值的 Debug 案例

你今天遇到：

```text
Build 成功
ST-LINK 正常
Download 正常

但是

LED 不亮
```

最终原因：

> **GPIO 引脚配置错误。**

这个案例建议你以后专门记在 Debug Log。

因为它训练的是非常重要的排查方式：

```text
Build是否成功？
        ↓ YES

Download是否成功？
        ↓ YES

程序是否在运行？
        ↓ YES

GPIO配置是否正确？
        ↓

原理图Pin是否对应？
        ↓
找到问题
```

以后电机控制真正 Debug 也一样。

不要一下子：

> “肯定是 FOC 算法问题。”

而要：

```text
电源
↓
驱动
↓
Pin
↓
PWM
↓
ADC
↓
Encoder
↓
软件
↓
控制算法
```

逐层定位。

这比会背很多 HAL API 更能体现工程能力。

------

# 三十三、以后每次修改 `.ioc` 的标准操作流程

建议你以后固定形成肌肉记忆：

```text
① 确认当前代码已经保存

② 如果当前功能已经稳定
   → Git Commit

③ 打开 .ioc

④ 修改 GPIO / TIM / ADC / UART 等配置

⑤ Ctrl + S

⑥ CubeMX Generate Code

⑦ 检查自己的 USER CODE 是否还在

⑧ Build

⑨ Download

⑩ 实机验证

⑪ Debug

⑫ 功能确认正常

⑬ Git Commit
```

你的半年计划要求的核心也正是：

> **不要复制很多工程保存版本，而是让一个主工程持续演进，每个功能对应一个清晰 Commit。**

------

# 三十四、你以后找工作最好能够讲出的专业表达

假如面试官问：

> 你平时 STM32 怎么开发？

以后你应该逐渐能够自然回答成：

> 我目前主要使用 STM32CubeIDE 和 CubeMX 完成 STM32G431 的外设配置，基于 HAL 库开发。通过 `.ioc` 配置系统时钟、GPIO、UART、Timer、ADC 等外设，生成初始化代码后，在用户代码区域以及独立功能模块中实现应用逻辑。程序通过 ST-LINK 和 SWD 下载调试，开发过程中会使用断点、Watch 和实机信号验证进行问题定位。

这段话已经比：

> “我用过 STM32，会点灯。”

专业很多。

以后半年我们还会把它升级成：

> 我基于 STM32G431 搭建 PMSM FOC 控制软件，TIM1 产生三相互补 PWM，ADC 由 PWM 定时触发同步采样，相电流经过 Clarke/Park 变换进入 dq 电流 PI，随后通过逆 Park 和 SVPWM 更新 PWM，占空比在控制 ISR 内周期计算……

到了这个阶段，你才能真正拿着这个项目去谈电控软件岗位。

------

# 三十五、今天结束时你真正应该记住的 12 件事

1. **`.ioc` 是 CubeMX 的硬件配置文件。**
2. **Build 是编译链接，不等于实机功能正确。**
3. **Download 是把程序写进 STM32 Flash。**
4. **ST-LINK 既能烧录，也能在线 Debug。**
5. **SWD 主要使用 SWDIO、SWCLK 等信号进行调试。**
6. **GPIO 引脚必须对照原理图，不能凭感觉选。**
7. **GPIO 一个 Pin 可以复用成多种外设功能。**
8. **Push-Pull 可以主动输出 High 和 Low。**
9. **STM32G431 当前通过 HSI + PLL 配置到了 170 MHz。**
10. **自己的代码优先放 `USER CODE BEGIN/END`，否则 CubeMX 重新生成可能覆盖。**
11. **真正的大型 FOC 程序以后要拆成 `.c/.h` 模块，不要全堆在 `main.c`。**
12. **UART 最基本的连接规则是 TX→RX、RX→TX、GND→GND，今天已经完成 USART3 的 115200 8N1 初步配置。**

------

## 当前项目检查点

你现在的进度可以记成：

```text
STM32G431CBU6 Project            ✅
STM32CubeIDE                     ✅
.ioc 基础使用                    ✅
170 MHz Clock                    ✅
SWD / ST-LINK                    ✅
Build                            ✅
Download                         ✅
GPIO Output                      ✅
LED Blink 实机                   ✅
发现并解决 GPIO Pin 配置错误     ✅
理解 USER CODE 防覆盖机制        ✅

USART3 Asynchronous              ✅ 配置
USART3 115200 8N1                ✅ 配置

USB-TTL 实机连接                 ⬜
Hello Motor Control 串口发送     ⬜
周期 Counter 输出                ⬜
Timer Interrupt                  ⬜
```

所以**我们下一次不用重复今天任何东西**。

下一次直接从：

> **USART3 实机通信：STM32 → USB-TTL → 电脑串口助手看到 `Hello Motor Control`**

开始。

跑通 UART 以后，如果你理解得快，我们当天继续 **Timer + 1 ms Interrupt**；如果 Timer 某一步需要消化，就在那里多停一会。以后整个半年计划都按这种节奏推进，而不是为了凑“Day 1～Day 7”人为拖慢或者赶进度。