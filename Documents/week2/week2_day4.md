# STM32 学习笔记：ADC 与 DMA 基础

## 1. ADC 的作用

ADC 全称：

**Analog-to-Digital Converter，模数转换器**

作用：

> 将真实世界中的模拟电压转换成 MCU 可以处理的数字量。

当前使用：

- ADC：ADC1
- 分辨率：12 bit
- 输入通道：ADC1_IN1
- GPIO：PA0

12 bit 可以表示：

$$
2^{12}=4096
$$

因此 ADC 的数字量范围为：

$$
0\sim4095
$$

如果 ADC 参考电压近似为 3.3 V，则：

$$
V_{ADC}=\frac{ADC_{raw}}{4095}\times3.3
$$

例如：

$$
ADC_{raw}=2048
$$

则：

$$
V_{ADC}\approx1.65\text{ V}
$$

---

## 2. 当前 ADC 采样链路

当前开发板使用：

```text
VDC
↓
电阻分压
↓
ADC1_IN1
↓
ADC1
↓
数字量
```

因此 MCU 并不是直接测量较高的 VDC，而是先通过硬件电阻网络将电压降低到 ADC 可以接受的范围，再进行采样。

当前 ADC1_IN1 对应：

```text
ADC1_IN1 → PA0
```

---

## 3. ADC Polling 模式

最开始使用的是 Polling 方式。

核心代码：

```c
HAL_ADC_Start(&hadc1);

HAL_ADC_PollForConversion(&hadc1, 10);

adc_raw = HAL_ADC_GetValue(&hadc1);

HAL_ADC_Stop(&hadc1);
```

执行过程：

```text
CPU 启动 ADC
↓
ADC 开始转换
↓
CPU 等待转换完成
↓
CPU 读取 ADC 数据
↓
ADC 停止
```

Polling 的特点：

- 实现简单
- 容易理解
- 适合初期测试
- CPU 需要参与采样过程
- 不适合高速连续采样

因此 Polling 更适合用来验证：

> ADC 本身是否能够正常工作。

---

## 4. ADC 校准

ADC 正式工作前进行了校准：

```c
HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
```

可以简单理解为：

> 修正 ADC 内部自身存在的转换偏差，提高采样准确性。

基本流程：

```text
ADC 初始化
↓
ADC Calibration
↓
开始 ADC 采样
```

ADC 校准通常在初始化阶段执行一次即可，不需要每次采样都重新校准。

---

## 5. ADC 原始值转换成电压

当前 ADC 为 12 bit，因此：

$$
ADC_{raw}\in[0,4095]
$$

参考电压近似为：

$$
V_{ref}=3.3\text{ V}
$$

所以：

$$
V_{ADC}=\frac{ADC_{raw}}{4095}\times V_{ref}
$$

程序中使用 mV 进行计算：

```c
adc_mv = adc_raw * 3300UL / 4095UL;
```

即：

$$
V_{ADC}(mV)=\frac{ADC_{raw}\times3300}{4095}
$$

例如：

$$
ADC_{raw}=83
$$

则：

$$
V_{ADC}\approx
\frac{83\times3300}{4095}
\approx66.9\text{ mV}
$$

由于代码采用整数运算，因此实际显示可能为：

```text
66 mV
```

---

# 6. DMA 是什么

DMA 全称：

**Direct Memory Access，直接存储器访问**

DMA 的主要作用：

> 在外设和内存之间自动搬运数据，减少 CPU 对数据搬运过程的参与。

没有 DMA 时：

```text
ADC
↓
CPU
↓
RAM变量
```

使用 DMA 后：

```text
ADC
↓
DMA
↓
RAM变量
```

可以简单记成：

> **DMA = 帮 CPU 搬数据。**

---

## 7. 为什么 ADC 要配合 DMA

Polling 时 CPU 需要：

```text
启动 ADC
↓
等待 ADC
↓
读取 ADC
```

如果以后电机控制中需要高速采集：

- 相电流
- 母线电压
- 温度
- 其他模拟信号

让 CPU 每次都亲自读取 ADC 会浪费大量计算时间。

使用 DMA 后：

```text
ADC 转换完成
↓
DMA 自动读取 ADC 数据
↓
DMA 自动写入 RAM
↓
CPU 需要时直接使用 RAM 中的数据
```

因此：

> DMA 可以明显减少 CPU 在数据搬运上的工作量。

---

# 8. 当前 ADC + DMA 配置

当前 ADC：

```text
Continuous Conversion Mode = Enabled
DMA Continuous Requests    = Enabled
```

当前 DMA：

```text
Direction               = Peripheral To Memory
Mode                    = Circular
Peripheral Increment    = Disable
Memory Increment        = Disable
Peripheral Data Width   = Half Word
Memory Data Width       = Half Word
```

DMA 使用：

```text
DMA1 Channel 1
```

---

## 9. 启动 ADC + DMA

当前代码：

```c
volatile uint16_t adc_dma_value = 0;
```

启动：

```c
HAL_ADC_Start_DMA(&hadc1,
                  (uint32_t *)&adc_dma_value,
                  1);
```

最后的：

```c
1
```

表示 DMA 数据长度为 1。

因此工作过程为：

```text
ADC 新数据
↓
DMA
↓
adc_dma_value
↓
下一次 ADC 新数据再次覆盖
```

也就是说：

> `adc_dma_value` 中保存的是当前最新的 ADC 转换结果。

---

# 10. 为什么使用 uint16_t

当前 ADC 是 12 bit。

最大 ADC 值：

$$
4095
$$

8 bit 最大只能表示：

$$
2^8-1=255
$$

不够使用。

16 bit 最大可以表示：

$$
2^{16}-1=65535
$$

因此：

```c
uint16_t adc_dma_value;
```

完全可以保存 12 bit ADC 数据。

DMA 中：

```text
Half Word = 16 bit
```

所以当前设置：

```text
Peripheral Data Width = Half Word
Memory Data Width     = Half Word
```

是合适的。

---

# 11. Peripheral To Memory

当前 DMA Direction：

```text
Peripheral To Memory
```

含义：

```text
ADC 外设
↓
DMA
↓
RAM 内存
```

ADC 属于 Peripheral。

`adc_dma_value` 位于 RAM 中，属于 Memory。

因此：

$$
Peripheral\rightarrow Memory
$$

---

# 12. Circular Mode

当前 DMA：

```text
Mode = Circular
```

Circular 表示 DMA 完成一次传输后，会自动重新开始。

过程：

```text
DMA 搬运
↓
完成
↓
重新开始
↓
DMA 搬运
↓
完成
↓
……
```

因此适合：

- ADC 连续采样
- 电机电流采样
- UART 连续接收
- 周期性数据采集

---

# 13. Increment Address

当前设置：

```text
Peripheral Increment = Disable
Memory Increment     = Disable
```

## Peripheral Increment

ADC 数据始终从固定的 ADC Data Register 中读取。

因此 DMA 每次读取的外设地址都一样：

```text
ADC_DR
↓
ADC_DR
↓
ADC_DR
```

所以：

```text
Peripheral Increment = Disable
```

---

## Memory Increment

当前只有一个变量：

```c
adc_dma_value
```

每次 ADC 数据都覆盖同一个变量即可：

```text
ADC数据
↓
adc_dma_value
```

因此：

```text
Memory Increment = Disable
```

如果以后使用：

```c
uint16_t adc_buffer[100];
```

希望 DMA 自动写成：

```text
adc_buffer[0]
adc_buffer[1]
adc_buffer[2]
...
adc_buffer[99]
```

这时才需要：

```text
Memory Increment = Enable
```

---

# 14. UART 每秒输出一次不代表 ADC 每秒采样一次

现在 ADC + DMA 一直连续工作。

例如 ADC 实际上可能不断产生：

```text
83
84
83
82
84
83
...
```

DMA 不断更新：

```c
adc_dma_value
```

但是 UART 仍然通过 TIM6：

```text
每 1 秒
↓
uart_send_flag = 1
↓
读取当前 adc_dma_value
↓
UART 打印
```

所以串口可能显示：

```text
DMA ADC = 83, Vadc = 66 mV
DMA ADC = 84, Vadc = 67 mV
DMA ADC = 82, Vadc = 66 mV
```

这并不代表 ADC 一秒只采了一次。

实际上：

> ADC 和 DMA 在这一秒中已经工作了很多次，只是 UART 每秒读取一次当前最新结果。

---

# 15. 今天遇到的 DMA 中断问题

运行 ADC + DMA 后出现过：

```text
adc_dma_value 正常
uart_send_flag 正常变成 1

但是：

adc_mv = 0
UART 无输出
```

这说明：

```text
ADC   ✅
DMA   ✅
TIM6  ✅
```

但主循环无法正常及时运行。

问题出现在：

```text
Continuous ADC
+
Circular DMA
+
DMA Buffer Length = 1
+
DMA Transfer Interrupt
```

ADC 转换速度很快。

由于 DMA 长度只有 1，每搬运一个 ADC 数据就完成一次 DMA 传输。

于是可能不断产生：

```text
ADC转换
↓
DMA搬运
↓
DMA完成中断
↓
CPU响应
↓
ADC转换
↓
DMA搬运
↓
DMA完成中断
↓
CPU响应
↓
……
```

导致 CPU 大量时间消耗在 DMA 中断中。

---

# 16. DMA 不一定需要中断

这是本次实验非常重要的知识点：

> **使用 DMA 不等于一定需要 DMA Interrupt。**

DMA 完全可以自己完成：

```text
ADC
↓
DMA
↓
RAM
```

而不通知 CPU。

只有当 CPU 确实需要知道：

> “一批数据已经传输完成。”

才需要 DMA 中断。

例如：

```c
uint16_t adc_buffer[100];
```

当 DMA 采满 100 个数据以后：

```text
DMA Transfer Complete
↓
通知 CPU
↓
CPU 开始处理这 100 个数据
```

这时 DMA 中断才有明显价值。

---

# 17. HT 与 TC

最终使用：

```c
__HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT);
__HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_TC);
```

其中：

## HT

HT：

**Half Transfer**

表示：

> DMA 已经完成一半的数据传输。

---

## TC

TC：

**Transfer Complete**

表示：

> DMA 已经完成全部的数据传输。

当前：

```text
DMA Length = 1
```

只希望 DMA 不断覆盖：

```c
adc_dma_value
```

因此不需要 HT 和 TC 中断通知 CPU。

关闭以后：

```text
ADC继续工作
↓
DMA继续搬运
↓
adc_dma_value继续更新
↓
CPU正常执行主循环
↓
UART正常输出
```

---

# 18. Polling 与 DMA 的区别

## Polling

```text
CPU
↓
启动 ADC
↓
等待 ADC
↓
读取 ADC
↓
处理数据
```

CPU 直接参与 ADC 数据读取。

---

## DMA

```text
ADC
↓
DMA
↓
RAM
```

CPU：

```text
需要数据
↓
读取 RAM
```

因此 DMA 的核心价值是：

> **减少 CPU 在数据搬运上的参与。**

---

# 19. 当前完整 ADC + DMA 链路

当前已经实现：

```text
真实模拟电压
↓
ADC1_IN1
↓
12-bit ADC
↓
DMA
↓
adc_dma_value
↓
软件换算
↓
UART
↓
电脑显示
```

相比 Polling：

```text
ADC
↓
CPU
↓
RAM
```

现在已经变成：

```text
ADC
↓
DMA
↓
RAM
```

---

# 20. 与以后 PMSM FOC 的关系

以后会逐渐形成：

```text
TIM1 PWM
↓
指定时刻触发 ADC
↓
ADC 采集 Ia / Ib
↓
DMA
↓
电流数据进入 RAM
↓
电流换算
↓
Clarke
↓
Park
↓
Id / Iq
↓
PI
↓
SVPWM
↓
CCR1 / CCR2 / CCR3
↓
三相 PWM
```

因此：

> ADC + DMA 是以后 FOC 电流采样链路的重要基础。

---

# 21. 本阶段需要重点记住

1. ADC 的作用：

   > 模拟量 → 数字量

2. 12 bit ADC：

   $$
   0\sim4095
   $$

3. ADC 电压换算：

   $$
   V_{ADC}=\frac{ADC_{raw}}{4095}\times V_{ref}
   $$

4. DMA：

   > 帮 CPU 搬数据。

5. ADC + DMA：

   ```text
   ADC
   ↓
   DMA
   ↓
   RAM
   ```

6. ADC DMA 常用方向：

   ```text
   Peripheral To Memory
   ```

7. Circular：

   > DMA 循环工作。

8. Half Word：

   ```text
   16 bit
   ```

9. 单变量 DMA：

   ```text
   Memory Increment = Disable
   ```

10. DMA 不一定需要中断。

11. DMA 中断过于频繁会增加 CPU 负担。

12. DMA 的核心价值：

   > **减少 CPU 对数据搬运的参与。**

---

# 22. 当前学习进度

- [x] GPIO / LED
- [x] USART3
- [x] TIM6 1 ms Interrupt
- [x] Timer + UART Flag
- [x] 普通 PWM
- [x] 20 kHz PWM
- [x] 25% / 50% / 75% Duty
- [x] ADC Polling
- [x] ADC 电压换算
- [x] ADC + DMA
- [x] DMA 中断问题排查
- [ ] 电流采样
- [ ] CubeMonitor
- [ ] Clarke / Park
- [ ] PI
- [ ] SVPWM
- [ ] FOC

下一阶段：

**开发板电流采样电路：采样电阻 → 运放 → ADC → 实际相电流。**