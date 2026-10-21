# STM32G431 PMSM Hall-FOC 项目阶段开发记录

**记录时间：** 2026-10-21  
**本次范围：** 上一轮速度闭环阶段记录之后，至手册第十章 TIM1 Break + FOC 故障锁存验证完成  
**项目工程：** `PMSM_FOC_G431`（仓库 `PMSM_FOC_STM32G431`）  
**参考资料：**《STM32G4 Simulink FOC 开发套件用户手册 V13.1》、商家第六章 UART 例程、`11.DAC_OUT` 例程  
**笔记类型：** 实际开发记录 / Debug Log，**不是**用于系统复习的手写原理笔记

> **状态说明：** 本文中“通过”仅指用户实际报告通过的实验；没有执行的实验或需要进一步核实的保护能力明确标为“未验证”。未提供最近一次真实 Git commit ID，因此本记录不虚构提交哈希。

---

## 1. 本阶段主要成果

| 模块                                       | 完成的内容                                                   | 验证状态                   |
| ------------------------------------------ | ------------------------------------------------------------ | -------------------------- |
| Hall-FOC 动态调速                          | `100 → 150 → 100 rpm` 的三阶段速度给定与跟踪                 | **实机通过**               |
| USART3 DMA / VOFA+（手册第六章 6A）        | JustFloat 实时波形；目标转速、反馈转速、`Iq_ref`、`Iq`、母线电压 | **实机通过**               |
| USART3 中断接收（第六章 6B）               | 发送字符 `P`，接收中断计数，并由 VOFA+ CH5 显示              | **实机通过**               |
| OPAMP3 + DAC3 + COMP1（第十章 10A/10B）    | 第三路运放、内部 DAC 参考电压、比较器初始化/启动             | **寄存器验证通过**         |
| COMP1 → TIM1 Break（10C）                  | 受控降低 DAC 阈值，COMP 翻转、TIM1 Break 置位、MOE 清零      | **非带载触发通过**         |
| Break → FOC 故障锁存（10D）                | 新增 `FOC_FAULT_HW_BREAK`，故障状态进入 `FOC_STATE_FAULT`    | **调试器验证通过**         |
| 真实电流硬件保护标定                       | 根据 OPAMP3 电流标定设置最终 DAC 门限，并验证真实过流保护    | **未进行，不可宣称已完成** |
| 主动再生制动 / 反向启动 / 实际机械负载抗扰 | 已做方案评估；未完成本阶段相应实机验证                       | **未完成，不计入本次验收** |

## 2. 阶段起点：原有 Hall-FOC 双闭环

本阶段沿用已实际运行的 STM32G431 Hall-FOC 程序，保留：

- TIM1 三相互补 PWM / SVPWM；ADC 注入同步采样与 OPAMP1/OPAMP2 电流测量。
- Hall 状态检测、电角度估算与标定偏置；d/q 电流 PI 和速度 PI。
- SW3 长按启动、Bootstrap、对齐、RUN/READY/FAULT 状态机、超时停止。
- **电流控制中断 20 kHz，速度外环 100 Hz**（本次不重调 PI）。
- 已有测试限幅：`SPEED_LOOP_IQ_MAX_A=0.08f`、`CURRENT_TEST_V_LIMIT_V=1.5f`、软件过流阈值 `CURRENT_OC_TRIP_A=0.25f`。这些是此前试验参数，并非电机额定参数。

### 2.1 已有动态调速结果

三段速度给定为 `100 → 150 → 100 rpm`。此前记录的相应后段平均实际转速约为：

| 阶段 | 目标转速 | 后段平均实际转速 |
| ---- | -------: | ---------------: |
| 1    |  100 rpm |        96.86 rpm |
| 2    |  150 rpm |       151.03 rpm |
| 3    |  100 rpm |        99.57 rpm |

结论：无载动态速度跟踪获得实机验证，但这**不等于**完成了实际机械负载抗扰或主动再生制动测试。裸轴没有合适的外部加载装置，暂不进行不受控摩擦加载。

## 3. 手册第六章 6A：USART3 DMA 与 VOFA+ 实时波形

### 3.1 开发目标

把“运行结束后串口打印 CSV”改进为“FOC 运行过程中持续显示波形”，并且不能在 20 kHz ADC 中断中阻塞等待 UART 发送。

串口通信基本配置：

| 项目     | 实际配置                                                     |
| -------- | ------------------------------------------------------------ |
| UART     | USART3；原有 TX/RX 引脚 PB10/PB11                            |
| 串口参数 | 115200，8 数据位，无校验，1 停止位                           |
| TX       | `USART3_TX` 使用 `DMA1_Channel3`                             |
| NVIC     | `DMA1_Channel3_IRQn = 3`，`USART3_IRQn = 3`；ADC1/ADC2 中断优先级保留为 0 |
| VOFA+    | `JustFloat`，小端 IEEE-754 `float32` 数据帧                  |
| 发送周期 | 主循环约每 10 ms 尝试发送一次；DMA 忙则丢弃本帧，不阻塞控制  |

**注意 DMA 资源：** 原 FOC 工程的 ADC DMA 已使用 `DMA1_Channel1/2`，不直接复制商家 UART 例程中可能与 ADC 冲突的 DMA 通道配置。

### 3.2 重要函数和代码位置

- `Core/Src/vofa_stream.c`：构建 JustFloat 帧，在前台调用 `HAL_UART_Transmit_DMA()`；DMA 缓冲区必须保持有效直至传输完成。
- `Core/Inc/vofa_stream.h`：声明 `VOFA_Stream_TrySend(...)` 参数列表。
- `Core/Src/main.c`：主循环按 10 ms 节拍调用 VOFA 发送；`#include "vofa_stream.h"` **不可省略**。
- 暂时不在同一 USART3 数据流中混发 `St=...` 等 ASCII 文本和 JustFloat 二进制帧；原来的 `FOC_CurrentTrace_DumpUART()` 文字输出调用已在 VOFA 阶段禁用。

初始 **5 个通道**：

| 通道 | MCU 变量             | 单位 |
| ---- | -------------------- | ---- |
| CH0  | `speed_ref_rpm`      | rpm  |
| CH1  | `speed_feedback_rpm` | rpm  |
| CH2  | `iq_ref`             | A    |
| CH3  | `i_dq.q`             | A    |
| CH4  | `vdc_bus`            | V    |

5 路 × 4 字节 + 4 字节 `00 00 80 7F` 帧尾 = **24 字节**。

### 3.3 本次最有价值的 Bug：漏掉头文件导致浮点参数错传

**现象：** VOFA+ 正确配置为 JustFloat，但先后出现 `100 rpm` 被解码为约 `3.390625`、电流与母线电压明显异常等问题；固定 24 字节测试帧却能够正确显示 `10、20、0.01、0.02、24`。

**根因定位：** 最新实际烧录的 `main.c` 调用了 `VOFA_Stream_TrySend(...)`，却没有包含它的声明头文件 `vofa_stream.h`。在允许隐式函数声明的编译配置中，浮点实参可能发生默认提升 `float → double`，使调用端与 `vofa_stream.c` 中 `float` 形参的约定不匹配，属于未定义行为。

**修复：** 在 `main.c` 的 `USER CODE BEGIN Includes` 添加：

```c
#include "vofa_stream.h"
```

**实际结果：** 修复后 VOFA+ 能正确显示五路实时量；某次截图中目标速度为 `100 rpm`，反馈约 `101.62 rpm`，母线电压约 `24.30 V`。据此确认真实变量传输恢复正常。

**开发经验：** `.c` 文件存在函数定义，不代表另一个 `.c` 自动知道函数原型；跨模块调用必须包含对应 `.h`。Build 没有 Error 不等于没有 Warning，需关注 `implicit declaration of function` 等警告。

## 4. 手册第六章 6B：UART 接收中断

### 4.1 实现方法

同一 USART3 实现**TX 使用 DMA、RX 使用中断**，互不冲突；保持 JustFloat TX 二进制协议不变。

- 初始化完成后，调用 `HAL_UART_Receive_IT(&huart3, &uart_rx6b_byte, 1U)` 启动第一个字节接收。
- 在 `HAL_UART_RxCpltCallback()` 中记录收到的字节；收到 ASCII 大写 `P` 时增加计数器。
- 回调结束前重新调用 `HAL_UART_Receive_IT()` 接收下一个字节。
- 不在回调中 `printf` 或发送文本，避免串口帧混用及中断内阻塞。
- 通过 VOFA+ 额外显示 **CH5：`UART_P_Count`**。6 路 × 4 字节 + 4 字节帧尾，共 **28 字节**。

### 4.2 实际验收

不按 SW3，不启动电机，在 VOFA+ 发送区连续三次发送大写 `P`：

```text
初始 CH5 = 0
发送第 1 次 P -> CH5 = 1
发送第 2 次 P -> CH5 = 2
发送第 3 次 P -> CH5 = 3
```

**结果：通过。** 验证 USART3 单字节接收中断、回调处理和接收再使能逻辑。该字符目前**不直接控制电机启停或 PI 参数**，真正指令下发留到手册后续章节。

## 5. 手册第十章：OPAMP3、DAC3、COMP1 与 TIM1 Break

### 5.1 参考例程与目的

- 一开始拿到的 `10.TIM1_OPAMP_ADC_DAC.zip` 实质主要是 TIM1/OPAMP/ADC，缺少当前目标的 COMP+DAC Break 完整演示。
- 后来检查 `11.DAC_OUT.zip`，确认其 `.ioc` 和源码包含 OPAMP3、DAC3、COMP1 和 TIM1 Break 的参考配置（另含第十一章 DAC1 波形输出）。
- **不覆盖现有 FOC 工程。** 仅从商家例程学习连接方式，再在自己的 `.ioc` 中增加所需配置。

目的：实现独立于 CPU 电流环的硬件保护路径：

```text
第三路电流采样信号 -> OPAMP3 -> PB1 / COMP1 (+)
                                  COMP1 (-) <- DAC3_CH1 比较阈值
                                       |
                                  COMP1 输出
                                       |
                                  TIM1 Break
                                       |
                               TIM1 MOE 清零 / PWM 关闭
```

### 5.2 10A：CubeMX 外设配置

| 模块    | 关键配置                                                     |
| ------- | ------------------------------------------------------------ |
| OPAMP3  | Standalone；PB0=VINP、PB2=VINM、PB1=VOUT                     |
| DAC3    | OUT1 connected to on-chip peripherals only；Normal Mode；Output Buffer Disable；Trigger None |
| COMP1   | `Input [+]` 使用 PB1（与 `OPAMP3_VOUT` 共享模拟引脚）；`Input [-]` 选择 `DAC3_OUT1`；非反相、无消隐、无迟滞（本次演示） |
| TIM1    | Break State Enable；Break Filter=2；Break Source COMP1 Enable；Automatic Output Disable |
| 原 TIM1 | PWM Period=`4249`，Dead Time=`85`，原互补 PWM 和 CH4 ADC 触发均保留 |

本次 `.ioc` 中确认出现：

```ini
TIM1.BreakState=TIM_BREAK_ENABLE
TIM1.BreakFilter=2
TIM1.SourceBRKCOMP1=TIM_BREAKINPUTSOURCE_ENABLE
```

### 5.3 10B：新增外设初始化和启动

CubeMX 重新生成代码后，`main.c` 中已有：

```c
MX_OPAMP3_Init();
MX_DAC3_Init();
MX_COMP1_Init();
```

在原有 OPAMP1/2 启动完成后的初始化区，新增第三路运放、DAC 阈值与比较器的启动流程。此次验证使用的初始化值：

```c
HAL_OPAMP_Start(&hopamp3);
HAL_DAC_SetValue(&hdac3, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 3000U);
HAL_DAC_Start(&hdac3, DAC_CHANNEL_1);
HAL_COMP_Start(&hcomp1);
```

正式代码包含返回状态错误处理。上面列的是**关键调用流程示意**，并非可直接覆盖的完整工程。`3000U` 按理想 `VREF=3.3 V` 估算约 `2.42 V`，仅为示范比较门限，**不是已标定的真实过流阈值**。

测试阶段暂时注释主循环的 `FOC_CheckStartButton();`，不允许通过 SW3 触发电机运行；`FOC_StateManager();` 保留。

### 5.4 10C：受控触发 TIM1 Break

初次进入 Debugger 时：

| 寄存器表达式                          |            初始值 | 含义             |
| ------------------------------------- | ----------------: | ---------------- |
| `(COMP1->CSR & COMP_CSR_EN) != 0U`    |                 1 | COMP1 已使能     |
| `(COMP1->CSR & COMP_CSR_VALUE) != 0U` |                 0 | 比较器输出低电平 |
| `TIM1->SR & TIM_SR_BIF`               |                 0 | 未检测到 Break   |
| `TIM1->BDTR & TIM_BDTR_MOE`           | 32768（`0x8000`） | MOE=1            |

随后为验证硬件链路，在 READY 且系统启动 2 秒以后，临时把 DAC3 值从 `3000U` 降至 `1000U`（约 `0.81 V`），使 COMP1 输出翻转：

| 寄存器表达式                          |        触发后结果 | 含义                |
| ------------------------------------- | ----------------: | ------------------- |
| `(COMP1->CSR & COMP_CSR_EN) != 0U`    |                 1 | 比较器运行中        |
| `(COMP1->CSR & COMP_CSR_VALUE) != 0U` |             **1** | 比较器被触发        |
| `TIM1->SR & TIM_SR_BIF`               | **128（`0x80`）** | TIM1 Break 发生     |
| `TIM1->BDTR & TIM_BDTR_MOE`           |             **0** | TIM1 主输出使能清零 |

**结果：通过。** 这是**不驱动电机的内部 Break 事件及 MOE 关断验证**，并非实机相电流过流实验，也未通过示波器确认功率输出引脚电平。

### 5.5 10D：把 Break 事件接入现有故障状态机

在 `FOC_Fault_t` 中新增：

```c
FOC_FAULT_HW_BREAK
```

在 `FOC_StateManager()` 的最前面（原有 `Global fault request` 处理之前），增加：

```c
if ((foc_state != FOC_STATE_FAULT) &&
    ((TIM1->SR & TIM_SR_BIF) != 0U))
{
    FOC_TriggerFault(FOC_FAULT_HW_BREAK);
}
```

本次用户报告实测达到：

```text
foc_state       = FOC_STATE_FAULT
foc_fault_code  = FOC_FAULT_HW_BREAK
TIM1 BIF        = 128
TIM1 MOE        = 0
```

**结果：通过。** 定时器硬件检测事件后，原有软件状态机能识别、锁存故障，进入 FAULT。

### 5.6 实验完成后的清理与保留

**应删除：** `while(1)` 中 `/* 10C: TIM1 Break 临时验证 */` 整段，包含 `break_test_done` 和 `HAL_DAC_SetValue(...,1000U)`；避免上电后 2 秒自动制造故障。

**应保留：**

- 初始化时 `HAL_DAC_SetValue(...,3000U)`（仅为演示阈值，尚不能视为有效保护参数）。
- OPAMP3 / DAC3 / COMP1 的初始化、启动，以及 TIM1 Break CubeMX 配置。
- `FOC_FAULT_HW_BREAK` 枚举和 `FOC_StateManager()` 内 BIF→故障锁存代码。
- 原有软件过流功能和其他保护逻辑。

**暂不恢复：** `FOC_CheckStartButton();` 的正常启动调用。因为尚未完成真实硬件过流阈值、比较器极性/电流对应关系和安全恢复检查；当前代码作为**硬件 Break 演示/锁定版本**保存。**Git 提交前确认临时 DAC=1000 代码确实已经删掉，Build 无报错。**

## 6. 本次开发中需要牢记的工程边界

1. **软件过流 ≠ 硬件 Break：** 前者需要 MCU 周期采样与执行判断，后者可由 COMP→TIM1 硬件路径触发。
2. **BIF=1、MOE=0 是内部链路成功的证据，但不代表已测得实际保护响应时间或三相 PWM 引脚关断波形。**
3. **单个 COMP1 + 单一路 OPAMP3 比较不能替代完整三相、双向电流保护。** 要结合真实采样电路方向、运放零点和增益决定阈值。
4. **`DAC=3000U` 与 `0.25A` 软件阈值不是同一意义的量。** 后续如果需要正式保护，必须把 DAC 阈值映射到 OPAMP3 输出电压和实际电流；不可凭比例猜。
5. **进入 FAULT 和恢复 FAULT 是两件事。** BIF 标志清理、Break 输入解除、MOE 恢复时序都尚需后续确定，不能在未完成前恢复 SW3 带载运行。
6. **CubeMX 重新生成代码要保护手写内容。** 确保修改位于 `USER CODE BEGIN/END` 合适区域，并检查 Git diff。

## 7. 后续学习路线（保持商家用户手册主线）

- **下一章：第十一章 DAC 波形输出。** 已收到商家 `11.DAC_OUT`，后续学习 DAC 输出波形的原理和可观测结果，不重复 TIM1/ADC 已完成部分。
- **之后：第十二章 CAN/CANFD**，继续建立完整电机控制通信能力。
- **另列未完成专题：** 真正工程化硬件过流阈值、Break 复位策略，以及主动制动/正反转/实际机械负载抗扰。**这些不能写成已通过的功能。**
- 系统手写笔记以后统一整理：本阶段需收录 UART 全双工、DMA TX vs IRQ RX、ISR 中避免阻塞、函数声明/浮点 ABI、DAC/COMP/TIM1 Break 硬件机制。**本 Markdown 是开发日志，不替代手写笔记。**

## 8. Git 提交（在本轮清理完成并 Build 通过之后执行）

在仓库根目录执行：

```bash
git status
git diff --check
git add -A
git diff --cached --stat
git diff --cached --check
git commit -m "Complete UART VOFA RX and TIM1 COMP break demo"
git push
```

- 提交前先检查 `git diff --cached --stat`，确认没有误提交 `Debug/`、`Release/`、临时构建产物和不应提交的巨大商家压缩包；发现多余文件要先取消暂存。
- 应跟踪的是实际 FOC 源码、`.ioc`、VOFA 通信模块、必要外设/IRQ 初始化改动，以及**这份开发笔记**。
- 建议把本文保存到 `Documents/STM32G431_Chapter06_Chapter10_Progress_Log_2026-10-21.md`（按你的现有项目目录微调）。
- `git push` 只推送到**当前已配置的 upstream**；如果终端提示没有 upstream，按 Git 输出配置对应远端，不要随意新建第二个仓库。
- 如果在提交前还有未解决的 Build 错误，请不要把提交说明写成“完整保护已完成”；此版本的准确性质是 **COMP→TIM1 Break demo**。

---

**阶段结论：** 已从原有可运行的 Hall-FOC 双闭环，扩展到 VOFA+ 实时可视化、USART3 异步接收以及 DAC3/COMP1/TIM1 Break 的内部触发与软件故障锁存。当前主程序仍是实验性固件，真实硬件电流门限和带载保护尚未确认。下一步按手册第十一章继续。 