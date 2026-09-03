# STM32G431 学习笔记 01

## 从新建工程到 GPIO 实机运行 + USART3 初步配置

## 1. 本阶段完成的内容

本阶段完成了 STM32G431 项目的第一次完整开发流程：

- 在 STM32CubeIDE 中创建 STM32G431 工程
- 选择正确 MCU 型号
- 配置 STM32Cube Firmware Package
- 进入 `.ioc` 配置界面
- 配置系统时钟
- 将系统主频配置到 170 MHz
- 配置 SWD 调试接口
- 配置 GPIO Output
- 编译工程
- 通过 ST-LINK 下载程序
- LED 实机点亮和闪烁
- 排查并解决 GPIO 引脚配置错误
- 理解 CubeMX 重新生成代码时的 USER CODE 保护机制
- 开始配置 USART3
- USART3 配置为 115200 8N1 异步串口

完整开发链路：

```text
STM32CubeIDE
↓
选择 MCU
↓
创建工程
↓
.ioc 配置
↓
Clock / GPIO / USART
↓
Generate Code
↓
Build
↓
ST-LINK
↓
Download
↓
Run / Debug
↓
开发板实机运行
```

---

# 2. 当前 MCU

当前开发板 MCU：

```text
STM32G431CBU6
```

创建 STM32 工程时不能只选择：

```text
STM32G431
```

而应该尽量确认完整型号。

因为完整型号会影响：

- MCU 封装
- 引脚数量
- Flash 容量
- RAM 容量
- GPIO 分布
- 外设资源

因此创建工程时应该：

```text
查看原理图 / 芯片丝印 / 开发板资料
↓
确认完整 MCU 型号
↓
CubeIDE 中选择完全对应的型号
```

不能凭感觉选择“差不多的芯片”。

---

# 3. STM32CubeIDE 是什么

STM32CubeIDE 是 ST 官方 STM32 集成开发环境。

它主要负责：

```text
编写代码
+
编译 Build
+
下载 Download
+
在线 Debug
```

可以简单理解为：

> STM32 固件开发的主要工作环境。

---

# 4. CubeMX 是什么

CubeMX 主要负责 MCU 外设配置。

例如：

```text
Clock
GPIO
UART
Timer
PWM
ADC
DMA
NVIC
```

现在 CubeMX 已经集成在 STM32CubeIDE 中。

通常通过：

```text
xxx.ioc
```

文件进入配置界面。

例如当前：

```text
PMSM_FOC_G431.ioc
```

---

# 5. `.ioc` 文件的作用

`.ioc` 可以理解成：

> STM32 外设和引脚配置文件。

例如在 `.ioc` 中设置：

```text
PC4 → GPIO_Output

USART3 → Asynchronous

TIM4 → PWM

ADC1 → ADC_IN1
```

CubeMX 会根据这些配置自动生成对应初始化代码。

例如：

```c
MX_GPIO_Init();
MX_USART3_UART_Init();
MX_TIM4_Init();
MX_ADC1_Init();
```

因此：

```text
.ioc
↓
硬件外设配置

.c / .h
↓
程序逻辑
```

---

# 6. STM32 主程序基本结构

STM32 的 `main()` 通常类似：

```c
int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    while (1)
    {

    }
}
```

可以按下面理解。

---

## HAL_Init()

```c
HAL_Init();
```

作用：

> 初始化 STM32 HAL 基础运行环境。

目前不需要深入内部实现。

---

## SystemClock_Config()

```c
SystemClock_Config();
```

作用：

> 配置 MCU 系统时钟。

例如当前配置到：

```text
170 MHz
```

---

## MX_xxx_Init()

例如：

```c
MX_GPIO_Init();
MX_USART3_UART_Init();
```

作用：

> 初始化 CubeMX 中已经配置好的外设。

---

## while(1)

```c
while (1)
{
}
```

表示：

> MCU 主循环。

STM32 通常不是运行完程序就退出。

而是：

```text
上电
↓
初始化
↓
while(1)
↓
持续运行
↓
直到掉电或 Reset
```

---

# 7. 为什么 MCU 需要 while(1)

PC 上普通程序可能：

```c
int main(void)
{
    ...
    return 0;
}
```

运行结束后程序退出。

但 MCU 通常需要持续工作，例如：

- 读取传感器
- 控制 PWM
- 接收 UART
- 控制电机
- 检测故障

所以：

```c
while (1)
{
}
```

表示：

> 主程序持续运行。

以后真正的 FOC 高速控制一般不会全部放进 `while(1)`。

更典型的是：

```text
main()
↓
初始化
↓
while(1)
↓
通信 / 状态管理 / 低速任务
```

高速实时控制则可能运行在：

```text
Timer / ADC Interrupt
↓
Control ISR
↓
FOC
```

---

# 8. STM32 时钟为什么重要

MCU 中很多功能都依赖时钟。

例如：

```text
CPU
Timer
PWM
UART
ADC
SPI
I2C
```

时钟可以理解成：

> MCU 和外设工作的节拍。

以后做电机控制时：

```text
Clock
↓
Timer
↓
PWM
↓
ADC Trigger
↓
Control ISR
↓
FOC
```

这些都会联系在一起。

---

# 9. HSI 和 HSE

## HSI

HSI：

**High Speed Internal**

表示：

> MCU 内部高速时钟。

当前 STM32G431 使用：

```text
HSI = 16 MHz
```

优点：

- 不需要外部晶振
- 使用简单
- 启动方便

---

## HSE

HSE：

**High Speed External**

表示：

> 外部高速时钟。

通常来自：

- 外部晶振
- 外部时钟源

目前需要记住：

```text
HSI → 内部时钟
HSE → 外部时钟
```

---

# 10. PLL

PLL：

**Phase Locked Loop，锁相环**

目前在 STM32 中最重要的作用可以理解成：

> 对已有时钟进行分频、倍频，得到需要的系统时钟。

当前：

```text
HSI = 16 MHz
```

经过：

```text
16 MHz
↓
PLLM / 4
↓
4 MHz
↓
PLLN × 85
↓
340 MHz
↓
PLLR / 2
↓
170 MHz
```

因此：

$$
f_{SYSCLK}
=
\frac{16\text{ MHz}}{4}
\times85
\div2
$$

得到：

$$
\boxed{f_{SYSCLK}=170\text{ MHz}}
$$

---

# 11. SYSCLK、HCLK、PCLK

STM32 Clock Tree 中经常看到：

```text
SYSCLK
HCLK
PCLK1
PCLK2
```

目前可以先这样理解。

## SYSCLK

**System Clock**

系统主时钟。

当前：

```text
SYSCLK = 170 MHz
```

---

## HCLK

主要与：

```text
CPU
AHB Bus
Memory
DMA
```

等相关。

---

## PCLK

PCLK：

**Peripheral Clock**

外设时钟。

例如：

- UART
- Timer
- SPI
- I2C

很多外设时钟最终来自 PCLK。

目前先建立：

```text
Clock Source
↓
PLL
↓
SYSCLK
↓
HCLK
↓
PCLK
↓
Peripheral
```

这个基本认识。

---

# 12. GPIO 是什么

GPIO：

**General Purpose Input/Output**

中文：

**通用输入输出**

GPIO 最基本分为：

```text
Input
↓
MCU读取外部电平
```

以及：

```text
Output
↓
MCU输出高低电平
```

典型应用：

- LED
- 按键
- Enable
- Fault
- 普通数字控制

---

# 13. GPIO Pin Multiplexing

STM32 一个物理 Pin 往往可以有多个功能。

例如一个 Pin 可能支持：

```text
GPIO
UART
Timer
PWM
SPI
I2C
ADC
```

这叫：

**Pin Multiplexing，引脚复用**

因此：

> 一个 Pin 不是只能干一种事情。

配置时必须结合：

```text
开发板原理图
+
MCU 引脚复用功能
+
CubeMX 配置
```

一起确认。

---

# 14. GPIO 配置必须和原理图一致

本阶段 LED 一开始不能正常工作。

当时：

```text
Build 成功
ST-LINK 正常
Download 正常
```

但：

```text
LED 没反应
```

最终发现：

> GPIO 引脚配置错误。

这说明：

```text
Build 成功
≠
功能正确
```

嵌入式开发必须检查真实硬件。

正确 Debug 思路：

```text
原理图上器件接哪个 Pin？
↓
CubeMX 配的是不是这个 Pin？
↓
Pin 当前是什么功能？
↓
GPIO 初始化是否正确？
↓
程序操作的是不是正确 GPIO？
↓
真实电平是否发生变化？
```

---

# 15. Push-Pull

GPIO Output 常用：

```text
Output Push Pull
```

Push-Pull：

**推挽输出**

可以主动输出：

```text
HIGH
LOW
```

即：

```text
GPIO = 1
→ 主动拉高

GPIO = 0
→ 主动拉低
```

适合：

- LED
- Enable
- 普通数字输出

---

# 16. Open-Drain

Open-Drain：

**开漏输出**

通常可以主动：

```text
拉低
```

但不能主动产生高电平。

高电平一般依靠：

```text
Pull-Up Resistor
```

产生。

典型应用：

```text
I2C
```

目前最重要的区别：

```text
Push-Pull
→ 可以主动输出 HIGH 和 LOW

Open-Drain
→ 主要负责主动拉 LOW
```

---

# 17. Pull-Up 和 Pull-Down

GPIO Input 如果没有明确连接高低电平，可能处于：

```text
Floating
```

即：

> 悬空。

悬空容易受到噪声影响。

因此可以使用：

## Pull-Up

```text
默认保持 HIGH
```

## Pull-Down

```text
默认保持 LOW
```

如果是 Push-Pull Output，通常：

```text
No Pull
```

即可。

---

# 18. Active High 和 Active Low

数字器件不一定：

```text
1 = 开
0 = 关
```

有些硬件是：

```text
0 = 有效
1 = 无效
```

这种叫：

**Active Low，低电平有效**

例如部分 LED 可能：

```text
GPIO = LOW
↓
LED ON
```

以后常见：

```text
RESET_N
FAULT_N
CS_N
ENABLE_N
```

其中 `_N` 经常表示：

> 低电平有效。

因此控制 GPIO 时要结合原理图，而不是只靠变量名字猜电平。

---

# 19. HAL 是什么

HAL：

**Hardware Abstraction Layer**

中文：

**硬件抽象层**

ST 把大量寄存器操作封装成 HAL 函数。

例如：

```c
HAL_GPIO_WritePin();
HAL_GPIO_TogglePin();
HAL_UART_Transmit();
HAL_TIM_PWM_Start();
HAL_ADC_Start();
```

目前学习顺序应该是：

```text
先会使用 HAL
↓
理解外设工作原理
↓
以后需要时再深入 LL / Register
```

而不是一开始就背所有寄存器。

---

# 20. LED Blink 实验

基本代码：

```c
HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4);

HAL_Delay(500);
```

其中：

```c
HAL_GPIO_TogglePin()
```

作用：

> 将 GPIO 当前输出状态翻转。

例如：

```text
HIGH → LOW

LOW → HIGH
```

---

## HAL_Delay()

```c
HAL_Delay(500);
```

表示：

> 延时约 500 ms。

所以：

```text
GPIO翻转
↓
等待500ms
↓
GPIO再次翻转
↓
等待500ms
```

最终形成 LED 闪烁。

---

# 21. Build 是什么

Build 可以简单理解为：

> 将 C 代码转换成 MCU 可以运行的程序。

大概过程：

```text
C Source
↓
Compiler
↓
Object File
↓
Linker
↓
MCU Program
```

Build 成功：

```text
0 Errors
```

只说明：

> 代码能够成功编译和链接。

不代表：

- GPIO 一定正确
- 硬件一定正确
- PWM 一定正确
- 电机一定能转

所以需要牢记：

> **Build Success ≠ Function Correct**

---

# 22. Download 是什么

Download：

> 把 Build 后产生的程序写入 STM32 Flash。

流程：

```text
PC
↓
ST-LINK
↓
STM32 Flash
```

程序写入 Flash 后，即使掉电，固件通常仍然保存在 MCU 中。

---

# 23. Flash 和 RAM

## Flash

主要用于保存：

- 程序代码
- 固件
- 常量

特点：

> 掉电后数据仍然存在。

---

## RAM

主要用于保存：

- 变量
- 数组
- Stack
- Heap
- 运行状态

特点：

> 掉电后数据丢失。

可以简单记：

```text
Flash
→ 程序放在哪里

RAM
→ 程序运行时数据放在哪里
```

---

# 24. ST-LINK

ST-LINK 是 STM32 常用的：

> 下载 + 在线调试工具。

连接关系：

```text
PC
↓
USB
↓
ST-LINK
↓
SWD
↓
STM32
```

主要作用：

```text
Programming
+
Debugging
```

可以完成：

- 烧录程序
- 打断点
- 单步执行
- 暂停 MCU
- 查看变量
- 查看内存

---

# 25. SWD

SWD：

**Serial Wire Debug**

是 Cortex-M MCU 常见调试接口。

主要信号：

```text
SWDIO
SWCLK
GND
```

实际工程还可能使用：

```text
NRST
VTref
```

CubeMX 中通常配置：

```text
System Core
↓
SYS
↓
Debug
↓
Serial Wire
```

这样保留 SWD 调试功能。

---

# 26. Run 和 Debug

## Run

主要作用：

> 让程序正常执行。

---

## Debug

Debug 可以：

- Breakpoint
- Step Over
- Step Into
- Resume
- Watch
- Expressions

因此 Debug 不只是“运行程序”。

而是：

> 控制 MCU 的执行过程，并观察 MCU 内部状态。

以后电机控制出现问题时，Debugger 是非常重要的工具。

---

# 27. CubeMX 为什么会覆盖代码

修改 `.ioc` 后：

```text
Ctrl + S
↓
Generate Code
```

CubeMX 会重新生成它负责的代码。

如果自己的代码随便写在自动生成区域，就可能被覆盖。

因此自己的代码应该优先放在：

```c
/* USER CODE BEGIN xxx */

/* USER CODE END xxx */
```

中。

例如：

```c
/* USER CODE BEGIN 3 */

HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4);
HAL_Delay(500);

/* USER CODE END 3 */
```

这样重新 Generate Code 时一般会保留。

---

# 28. 常见 USER CODE 区域

## Includes

```c
/* USER CODE BEGIN Includes */

#include <stdio.h>

/* USER CODE END Includes */
```

用于添加自己的头文件。

---

## Private Variables

```c
/* USER CODE BEGIN PV */

uint32_t counter = 0;

/* USER CODE END PV */
```

用于添加自己的全局变量。

---

## USER CODE BEGIN 2

```c
/* USER CODE BEGIN 2 */

HAL_TIM_PWM_Start(...);

/* USER CODE END 2 */
```

常用于：

> 外设初始化完成以后只执行一次的代码。

---

## USER CODE BEGIN 3

```c
/* USER CODE BEGIN 3 */

...

/* USER CODE END 3 */
```

常用于：

> `while(1)` 主循环中的用户代码。

---

## USER CODE BEGIN 4

```c
/* USER CODE BEGIN 4 */

void My_Function(void)
{
}

/* USER CODE END 4 */
```

可以用于添加自己的函数。

---

# 29. 为什么以后不能把所有代码都放 main.c

以后项目会逐渐加入：

```text
ADC
PWM
Encoder
Current Sampling
PI
Clarke
Park
SVPWM
FOC
Fault
State Machine
```

如果全部放在：

```text
main.c
```

文件会越来越难维护。

以后应该逐渐拆成模块：

```text
Control/
├── pi.c
├── transform.c
├── svpwm.c
└── foc.c

Driver/
├── adc.c
├── pwm.c
└── encoder.c
```

`main.c` 更适合负责：

```text
系统初始化
↓
模块初始化
↓
任务调度
↓
程序入口
```

---

# 30. USART3 初步配置

本阶段最后开始配置 USART3。

CubeMX：

```text
Connectivity
↓
USART3
↓
Asynchronous
```

配置：

```text
Baud Rate = 115200

Word Length = 8 Bits

Parity = None

Stop Bits = 1

Data Direction = TX + RX

Hardware Flow Control = Disable
```

即：

```text
115200 8N1
```

---

# 31. USART3 当前引脚

当前：

```text
PB10 → USART3_TX
PB11 → USART3_RX
```

后续通过 USB-TTL 连接电脑：

```text
STM32 TX → USB-TTL RX

STM32 RX ← USB-TTL TX

GND ↔ GND
```

本阶段只完成 USART3 基础配置。

真正的：

```text
Hello Motor Control
Counter
Timer + UART
```

属于下一阶段。

---

# 32. 本阶段最重要的 Debug 经验

本阶段最重要的问题：

```text
Build成功
↓
Download成功
↓
LED没有反应
```

最终发现：

```text
GPIO Pin配置错误
```

这个问题说明以后 Debug 不能直接认为：

> “代码一定有问题。”

应该逐层排查：

```text
供电
↓
原理图
↓
Pin
↓
CubeMX配置
↓
初始化
↓
程序执行
↓
真实硬件输出
```

以后做电机控制同样如此。

例如：

```text
电机不转
```

可能是：

- PWM 没输出
- Driver 没 Enable
- ADC 错误
- Encoder 错误
- Fault 触发
- GPIO 配错
- FOC 算法错误

不能一上来就怀疑 PI 或 FOC。

---

# 33. 本阶段最需要记住的知识

1. STM32 程序基本运行结构：

   ```text
   Reset
   ↓
   main()
   ↓
   初始化
   ↓
   while(1)
   ```

2. GPIO：

   > General Purpose Input/Output

3. STM32 Pin 支持 Alternate Function。

4. GPIO 配置必须结合真实原理图。

5. Push-Pull：

   > 可以主动输出 HIGH 和 LOW。

6. Pull-Up：

   > 默认拉高。

7. Pull-Down：

   > 默认拉低。

8. HSI：

   > MCU 内部高速时钟。

9. HSE：

   > 外部高速时钟。

10. PLL：

    > 通过分频、倍频产生目标时钟。

11. 当前：

    $$
    SYSCLK=170\text{ MHz}
    $$

12. HAL：

    > ST 提供的硬件抽象层接口。

13. Build：

    > 编译 + 链接程序。

14. Build Success：

    > 不代表硬件功能正确。

15. ST-LINK：

    > 下载 + Debug。

16. SWD：

    > STM32 常用在线调试接口。

17. 自己的代码优先写在：

    ```c
    /* USER CODE BEGIN */
    /* USER CODE END */
    ```

18. 修改 `.ioc` 后重新生成代码时，要注意自己的代码是否位于保护区域。

19. 大型工程不能把所有代码长期堆在 `main.c`。

20. USART3 当前基础参数：

    ```text
    115200 8N1
    ```

---

# 34. 本阶段开发流程

需要逐渐形成固定开发习惯：

```text
查看原理图
↓
CubeMX配置
↓
Generate Code
↓
添加USER CODE
↓
Build
↓
Download
↓
Run / Debug
↓
实机验证
↓
发现问题
↓
逐层排查
```

最重要的是：

> **嵌入式开发必须同时关注软件配置和真实硬件。**

---

# 35. 与后续学习的关系

本阶段建立的是最基础的开发环境和硬件控制能力：

```text
Project Setup
↓
Clock
↓
GPIO
↓
ST-LINK / SWD
↓
UART基础
```

后续继续发展为：

```text
UART
↓
Timer
↓
Interrupt
↓
PWM
↓
ADC
↓
DMA
↓
Current Sampling
↓
FOC
```

这一阶段真正需要形成的是：

> **能够独立完成 STM32 工程创建、基础外设配置、Build、Download、Debug 和实机验证。**