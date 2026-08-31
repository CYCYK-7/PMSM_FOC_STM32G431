对。**从第二周开始，代码的放置方式要和第一周不一样。**

第一周那些 C 语言代码本质上是练习，可以按 `Day01/Day02/...` 分文件夹；但第二周开始已经进入真正的 STM32G431 固件开发，**不建议每天复制一个完整 STM32 工程**。更合适的是从 Day 1 建立一个真正的固件工程，然后 Day 2 GPIO、Day 3 UART、Day 4 Timer……都在这个工程上继续演进，每完成一个稳定功能就 Commit。

这也最符合你半年计划中“一路从 GPIO/UART → Timer → PWM → ADC → FOC 形成真实开发历史”的思路。

# 一、从第二周开始，代码放在哪里

你最开始建立的仓库是：

```text
PMSM_FOC_STM32G431/
│
├── Firmware/
├── Matlab/
├── Simulink/
├── Documents/
├── Debug_Log/
├── Test/
└── README.md
```

这正是原计划规定的项目骨架。

我建议现在整理成这样：

```text
PMSM_FOC_STM32G431/
│
├── Firmware/
│   │
│   ├── practice/
│   │   └── Week01_C/
│   │       ├── Day01_BasicTypes/
│   │       ├── Day02_ArrayPointer/
│   │       ├── Day03_StructTypedef/
│   │       ├── Day04_EmbeddedKeywords/
│   │       ├── Day05_Module/
│   │       ├── Day06_CurrentLoop/
│   │       └── Day07_Review/
│   │
│   └── PMSM_FOC_G431/
│       └── ← 从 Week 2 Day 1 开始，真正STM32工程放这里
│
├── Matlab/
├── Simulink/
│
├── Documents/
│   └── Week02/
│
├── Debug_Log/
│   └── Week02_Debug.md
│
├── Test/
│
└── README.md
```

其中最重要的是：

```text
Firmware/PMSM_FOC_G431/
```

**从今天开始，这个就是你的主固件工程。**

------

# 二、以后不要建立 Day02_GPIO、Day03_UART 的完整工程副本

比如我们今天建立：

```text
Firmware/
└── PMSM_FOC_G431/
```

Day 2 做 GPIO 后，还是这个：

```text
Firmware/
└── PMSM_FOC_G431/
```

只是代码增加 GPIO。

Day 3 做 UART：

```text
Firmware/
└── PMSM_FOC_G431/
```

继续增加 UART。

Day 4 Timer：

```text
Firmware/
└── PMSM_FOC_G431/
```

继续增加 Timer。

一直到以后：

```text
GPIO
 ↓
UART
 ↓
Timer
 ↓
PWM
 ↓
ADC
 ↓
Clarke/Park
 ↓
PI
 ↓
SVPWM
 ↓
Encoder
 ↓
FOC
```

都让同一个工程逐渐长起来。

这样 Git 才真正有意义。

------

# 三、Git 就负责保存“每天的版本”

不用：

```text
Day02_GPIO工程
Day03_UART工程
Day04_Timer工程
```

来保存历史。

而是：

```text
PMSM_FOC_G431
    │
    ├── Commit：Initial STM32G431 firmware project
    │
    ├── Commit：Add GPIO LED test
    │
    ├── Commit：Add UART communication
    │
    ├── Commit：Add timer interrupt
    │
    ├── Commit：Implement TIM1 PWM
    │
    └── ...
```

这才是你的半年计划真正想训练的工程开发方式。

你的计划本来也明确要求 Week 2 开始形成：

> **一个功能对应一个清晰 Commit。**

------

# 四、Documents 和 Debug_Log 怎么用

以后配置过程中有一些**值得保留但不是代码**的东西，不要塞进固件源码。

例如今天可以建立：

```text
Documents/
└── Week02/
    └── Day01_STM32_Project_Setup.md
```

以后简单记录：

```markdown
# Week 2 Day 1

## MCU
STM32G431xxx

## IDE
STM32CubeIDE

## Debugger
ST-LINK

## 工程位置
Firmware/PMSM_FOC_G431/

## 今天完成
- 创建STM32工程
- Build成功
- ST-LINK连接成功
- Download成功
- Debug成功
```

如果今天遇到问题，比如：

```text
ST-LINK找不到芯片
编译失败
下载失败
时钟配置错误
```

记录在：

```text
Debug_Log/
└── Week02_Debug.md
```

以后这些反而很适合面试讲。

------

# 五、所以第二周最终目录会逐渐变成

Week 2 结束大概：

```text
PMSM_FOC_STM32G431/
│
├── Firmware/
│   ├── practice/
│   │   └── Week01_C/
│   │
│   └── PMSM_FOC_G431/
│       ├── Core/
│       │   ├── Inc/
│       │   └── Src/
│       ├── Drivers/
│       ├── Debug/
│       ├── ...
│       └── PMSM_FOC_G431.ioc
│
├── Documents/
│   └── Week02/
│
├── Debug_Log/
│   └── Week02_Debug.md
│
└── ...
```

其中 `Core/Drivers/...` 这些目录**不用你自己手动创建**，STM32CubeIDE/CubeMX 创建工程后会帮你生成。

------

# Week 2 · Day 1

今天正式开始：

> **建立 STM32G431 主工程 + 搞明白 STM32 开发流程 + Build / Download / Debug 第一次跑通**

你的 Week 2 原计划要求建立这样一条完整流程：

```text
CubeMX / CubeIDE
        ↓
配置工程
        ↓
生成代码
        ↓
Build
        ↓
ST-LINK
        ↓
Download
        ↓
Debug
```



今天就只完成这条链。

**暂时不做 GPIO，不做 UART，不做 Timer。**

------

# 六、Day 1 最终目标

今天结束的时候，达到：

```text
PC
 ↓
STM32CubeIDE
 ↓
建立 STM32G431 工程
 ↓
生成代码
 ↓
Build 0 Errors
 ↓
ST-LINK连接
 ↓
Download 到 MCU
 ↓
进入 Debug
 ↓
在 main() 设置断点
 ↓
MCU成功停在断点
```

做到这里，Day 1 就结束。

不要继续往 GPIO 冲。

------

# 七、今天先理解 5 个东西

今天不要学一大堆 STM32 理论。

只搞懂：

### ① STM32CubeIDE

可以先理解成：

> STM32 官方开发环境。

它帮你完成：

```text
编辑代码
+
编译
+
下载
+
Debug
```

------

### ② CubeMX

负责：

```text
选择MCU
 ↓
配置Pin
 ↓
配置Clock
 ↓
配置GPIO/UART/Timer/ADC
 ↓
自动生成初始化代码
```

现在 CubeMX 功能已经集成在 CubeIDE 里，所以后面我们很多时候直接在 `.ioc` 配置界面操作即可。

------

### ③ Build

比如你有：

```c
main.c
```

以及大量：

```text
HAL
CMSIS
启动文件
```

Build 会经过大概：

```text
C源码
 ↓
Compiler
 ↓
目标文件
 ↓
Linker
 ↓
最终程序
```

今天不用研究编译器细节。

只记：

> **Build = 把人写的代码变成 MCU 能运行的程序。**

------

### ④ ST-LINK

你以后非常经常看到：

```text
PC
  │
 USB
  ↓
ST-LINK
  │
 SWD
  ↓
STM32G431
```

它主要负责：

```text
烧录
+
在线Debug
```

------

### ⑤ Debug

Debug 不是普通“运行”。

它允许你：

```text
暂停程序
看变量
打断点
单步执行
继续运行
```

以后电机不转的时候，这东西非常重要。

------

# 八、今天创建 STM32 工程

建议工程名称直接：

```text
PMSM_FOC_G431
```

这样就和我们之前确定的：

```text
Firmware/PMSM_FOC_G431/
```

一致。

不要取：

```text
test
test1
123
demo
aaa
```

以后这个工程会一路变成你的求职项目。

------

# 九、工程放置位置

路径选择你仓库里的：

```text
PMSM_FOC_STM32G431/
└── Firmware/
```

最终应该让 CubeIDE 工程位于：

```text
PMSM_FOC_STM32G431/
└── Firmware/
    └── PMSM_FOC_G431/
```

不要让 CubeIDE 默认把它创建到：

```text
C:\Users\xxx\STM32CubeIDE\workspace...
```

然后和 Git 仓库完全分离。

这一点今天非常重要。

------

# 十、第一步：确认你到底是哪一颗 G431

你的开发板虽然确定是：

```text
STM32G431
```

但芯片完整型号一般还会类似：

```text
STM32G431CBT6
STM32G431RBT6
...
```

这里**不要凭记忆乱选**。

你可以看：

- 芯片表面的完整丝印；
- 开发板资料；
- 原理图；
- 之前买板子的资料。

最终在 CubeIDE 创建工程时选择完全对应的 MCU。

如果你的开发板有对应官方/厂商 `.ioc` 工程，后面也可能直接基于它，但今天先理解这个原则。

------

# 十一、创建工程以后你会看到很多文件

初学者第一次打开非常容易懵：

```text
Core
Drivers
Middlewares
Debug
...
```

今天不要全点。

你只关注：

```text
Core/
├── Inc/
└── Src/
```

尤其：

```text
Core/Src/main.c
```

------

# 十二、main.c 就是今天最重要的入口

打开以后你大概率会看到：

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

实际生成内容会随工程配置不同而不同。

今天先按人话理解：

```text
int main(void)
↓
程序主要入口


HAL_Init();
↓
初始化HAL基础环境


SystemClock_Config();
↓
配置系统时钟


MX_xxx_Init();
↓
初始化你配置的各种外设


while(1)
↓
程序进入无限循环
```

------

# 十三、为什么 STM32 里面要有 while(1)

你第一周 PC 程序经常：

```c
int main(void)
{
    ...
    return 0;
}
```

执行完程序就结束。

但 MCU 不一样。

STM32 上电以后通常需要一直工作：

```text
上电
 ↓
初始化
 ↓
while(1)
 ↓
一直运行
 ↓
直到掉电/复位
```

所以：

```c
while (1)
{
}
```

可以理解成：

> **MCU 的主循环。**

这是今天非常重要的认知。

------

# 十四、以后 FOC 是不是就全部写在 while(1)？

不是。

以后你会逐渐形成：

```text
main()
 ↓
初始化
 ↓
while(1)
 ↓
低速任务 / 状态管理 / 通信
```

而真正高速电流环更可能：

```text
Timer / ADC Interrupt
          ↓
      Control ISR
          ↓
          FOC
```

也就是说：

```text
高速实时控制
≠
简单塞在 while(1)
```

现在先有这个意识。

------

# 十五、今天暂时不要修改 CubeMX 生成的大量代码

STM32 自动生成代码里，你会看到类似：

```c
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
```

以及：

```c
/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
```

这种区域。

这非常重要。

以后我们自己代码优先放：

```text
USER CODE BEGIN
        ↓
自己的代码
        ↓
USER CODE END
```

因为重新生成 CubeMX 代码的时候，这些区域通常会被保留。

而你如果随便修改自动生成区：

> 后面重新 Generate Code 时，自己的修改可能被覆盖。

今天就要开始形成这个习惯。

------

# 十六、今天如果想放一个最简单的测试变量

可以在合适 USER CODE 区域放：

```c
volatile uint32_t debug_counter = 0;
```

然后主循环：

```c
while (1)
{
    debug_counter++;
}
```

今天不需要 LED。

这个变量只是为了等会 Debug。

为什么我写：

```c
volatile
```

你第一周已经知道了。

不过这里即使不用 `volatile`，普通逻辑也能运行；我们今天用它主要是方便 Debug 观察，并继续熟悉嵌入式写法。

------

# 十七、完整思想非常简单

代码类似：

```c
#include "main.h"

/* USER CODE BEGIN PV */

volatile uint32_t debug_counter = 0;

/* USER CODE END PV */


int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    /* USER CODE BEGIN 2 */

    debug_counter = 0;

    /* USER CODE END 2 */


    while (1)
    {
        /* USER CODE BEGIN WHILE */

        debug_counter++;

        /* USER CODE END WHILE */
    }
}
```

**注意：不要把这整份覆盖到你 CubeIDE 自动生成的 `main.c` 上。**

你只需要把：

```c
volatile uint32_t debug_counter = 0;
```

以及：

```c
debug_counter++;
```

放进对应 USER CODE 区域。

STM32 工程从今天开始要养成：

> **基于自动生成工程修改，而不是拿我一整份 main.c 强行覆盖。**

这个区别很重要。

------

# 十八、然后第一次 Build

执行：

```text
Build Project
```

最终最重要的是看：

```text
0 Errors
```

Warnings 暂时也最好没有。

如果出现错误，不要开始胡乱改。

以后统一按：

```text
第一条 Error
 ↓
文件
 ↓
行号
 ↓
错误内容
```

来排查。

**不要一次盯十几个后续错误。**

很多时候第一个错误会引发后面十几个连锁错误。

------

# 十九、今天 Build 成功意味着什么

它只能证明：

> **代码语法/链接等基本上能够生成程序。**

不代表：

```text
硬件一定正常
GPIO一定正常
电机一定正常
```

这是以后 Debug 很重要的思维：

```text
Build成功
≠
功能正确
```

------

# 二十、下一步：ST-LINK 连接

你要让：

```text
PC
 ↓
ST-LINK
 ↓
STM32
```

真正连起来。

今天目标不是研究 SWD 协议。

只需要确认：

> CubeIDE 能识别目标芯片。

------

# 二十一、第一次 Download / Run

Build 成功以后：

```text
Run
```

或者 Debug 下载程序。

CubeIDE 会把程序烧入 STM32 Flash。

你可以简单理解：

```text
电脑里的程序文件
        ↓
ST-LINK
        ↓
写入STM32 Flash
        ↓
MCU复位
        ↓
从main开始运行
```

这一步成功非常重要。

因为意味着：

> **你的 PC → 编译器 → ST-LINK → MCU 整条开发链已经打通。**

------

# 二十二、今天最重要的实验：第一次断点

在：

```c
debug_counter++;
```

这一行设置：

> Breakpoint

然后进入：

```text
Debug
```

程序应该运行到这里停下来。

如果成功：

> 你第一次真正控制住 MCU 的执行过程。

------

# 二十三、此时观察 debug_counter

打开：

```text
Variables
```

或者：

```text
Expressions / Watch
```

加入：

```text
debug_counter
```

假设现在：

```text
debug_counter = 0
```

单步一下：

```text
debug_counter = 1
```

再：

```text
2
```

再：

```text
3
```

这就是：

> **Debugger 真的正在读 MCU RAM 里的变量。**

这个感觉一定建立起来。

------

# 二十四、今天顺便认识 Step Over

光标停在：

```c
debug_counter++;
```

点：

```text
Step Over
```

程序执行这一行并停到下一行/下一次可停位置。

今天先会用：

- Breakpoint
- Resume/Continue
- Step Over

就够。

真正 Debugger 专项在 Day 6。

------

# 二十五、今天不要研究寄存器

Debug 页面里可能看到：

```text
Registers
Memory
Disassembly
SFR
```

今天统统不用点。

不要因为：

> “感觉专业”

就开始看寄存器。

你的路线是：

```text
先会用 HAL
↓
先跑通工程
↓
以后需要的时候再下钻
```

不是一上来研究 ARM Cortex-M4 底层。

------

# 二十六、今天最需要真正搞懂的一张图

```text
你写：

main.c
   ↓

CubeIDE Build
   ↓

Compiler + Linker
   ↓

生成 MCU 程序
   ↓

ST-LINK
   ↓

写入 STM32 Flash
   ↓

STM32 Reset
   ↓

main()
   ↓

初始化
   ↓

while(1)
   ↓

不断执行
```

今天如果这条链理解了，就已经很成功。

------

# 二十七、Day 1 的几个修改实验

今天还是保持我们的原则：

> 不默写，重点会改。

### 实验 1

原来：

```c
debug_counter++;
```

改：

```c
debug_counter += 10;
```

再 Debug。

预测：

```text
0
10
20
30
...
```

------

### 实验 2

改：

```c
debug_counter += 100;
```

预测结果。

------

### 实验 3

设置：

```c
volatile uint8_t motor_enable = 0;
```

然后：

```c
motor_enable = 1;
```

在 Debug 里观察：

```text
0 → 1
```

这里没有真实启动电机。

只是开始使用：

> 电机工程命名。

------

### 实验 4

增加：

```c
float iq_ref = 2.0f;
```

在 Watch 里查看：

```text
iq_ref = 2
```

然后改：

```c
iq_ref = 3.0f;
```

重新 Build / Download / Debug。

你会开始体会：

```text
修改代码
↓
Build
↓
Flash
↓
Debug
↓
观察
```

这就是以后半年每天都会重复的开发流程。

------

# 二十八、今天不要碰 GPIO

即使工程里已经自动生成：

```c
MX_GPIO_Init();
```

也不要今天就研究 LED。

Day 2 专门做：

> GPIO + LED Blink

今天只验证：

```text
MCU工程能创建
编译能成功
芯片能连接
程序能烧录
Debugger能停
变量能观察
```

把开发链打通。

------

# 二十九、今天的 Debug Log

建议建立：

```text
Debug_Log/
└── Week02_Debug.md
```

今天可以先写：

```markdown
# Week 2 Debug Log

## Day 1 - STM32G431 Project Setup

### MCU
STM32G431xxxx

### IDE
STM32CubeIDE

### Debugger
ST-LINK

### Result
- Project created
- Build passed
- Download passed
- Debug connected
- Breakpoint passed

### Problems
无 / 实际出现的问题

### Current Stable State
STM32G431 empty base project runs successfully
```

如果今天遇到问题，就真实写，不要为了好看写“无”。

------

# 三十、Day 1 今天的 Git Commit

今天所有东西确认：

```text
Build
✅

Download
✅

Debug
✅
```

以后再 Commit。

先：

```bash
git status
```

检查。

然后：

```bash
git add -A
```

然后：

```bash
git commit -m "Initialize STM32G431 firmware project"
```

最后：

```bash
git push
```

这个 Commit 非常重要。

以后它就是：

> **你的 STM32G431 最初始稳定工程。**

------

# 三十一、为什么这个版本以后非常有用

比如未来 Week 3 PWM 配坏了。

或者 Week 4 ADC 搞出问题。

你永远知道：

```text
Initialize STM32G431 firmware project
```

这个版本至少：

```text
Build正常
ST-LINK正常
MCU正常
Debug正常
```

所以出现问题时，可以判断：

> 是后来外设配置导致的，而不是最基础的环境就有问题。

这就是 Git 真正的工程价值。

------

# 三十二、今天建议投入时间

大约 **2～3 小时**。

### 第 1 阶段：20～30 min

整理目录：

```text
Firmware/
├── practice/Week01_C/
└── PMSM_FOC_G431/
```

------

### 第 2 阶段：30～40 min

创建 STM32G431 工程。

认识：

```text
.ioc
Core
Drivers
main.c
```

------

### 第 3 阶段：20～30 min

第一次：

```text
Build
```

确保：

```text
0 Errors
```

------

### 第 4 阶段：30～40 min

连接 ST-LINK。

第一次：

```text
Download
Debug
```

------

### 第 5 阶段：30 min

使用：

```c
debug_counter
```

训练：

```text
Breakpoint
Watch
Step Over
Continue
```

------

### 第 6 阶段：15 min

写：

```text
Week02_Debug.md
```

然后 Git Commit。

------

# 三十三、Day 1 验收题

今天结束以后，你应该能回答：

1. CubeIDE 是干什么的？
2. CubeMX/ioc 配置主要干什么？
3. Build 和 Download 有什么区别？
4. ST-LINK 是干什么的？
5. STM32 的代码最终存在哪里？
6. `main()` 是什么？
7. 为什么 MCU 常有 `while(1)`？
8. `USER CODE BEGIN/END` 为什么重要？
9. Breakpoint 是什么？
10. Watch/Expressions 是干什么的？
11. 为什么 `Build成功 ≠ 功能一定正确`？
12. 为什么今天确认正常以后要 Commit？

如果能回答 9～10 个：

> **Week 2 Day 1 就通过。**

------

# 三十四、Day 1 真正的成果不是代码量

今天甚至可能只真正手动加：

```c
volatile uint32_t debug_counter = 0;
```

和：

```c
debug_counter++;
```

但这完全没问题。

因为今天真正获得的是：

```text
第一次真正 STM32 工程
+
第一次 Build
+
第一次烧录
+
第一次 Debug
+
第一次观察 MCU 变量
+
第一个稳定固件 Commit
```

这比今天写 200 行代码重要得多。

**Day 2 再正式开始 GPIO：找到你这块开发板实际 LED 对应的 GPIO，引脚配置、Push-Pull、Output Level、`HAL_GPIO_WritePin()` / `HAL_GPIO_TogglePin()`，最后让真实 LED 按 100 ms / 500 ms / 1000 ms 三种周期闪烁。**