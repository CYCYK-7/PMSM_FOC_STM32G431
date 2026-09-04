# STM32 学习笔记：U/V 两相电流采样

## 1. 电流采样基本链路

开发板三相使用：

```text
相电流
↓
5 mΩ 采样电阻
↓
毫伏级压差
↓
OPAMP 放大
↓
ADC
↓
DMA
↓
软件换算成电流
```

采样电阻：

$$
R_{shunt}=5m\Omega=0.005\Omega
$$

---

## 2. 为什么零电流约为 1.65 V

ADC 只能测：

$$
0\sim3.3V
$$

但相电流有正负，因此电路把 0 A 放在中间：

```text
负电流 → < 1.65 V
0 A    → ≈ 1.65 V
正电流 → > 1.65 V
```

所以：

$$
0A\approx1.65V
$$

对应 12 bit ADC：

$$
ADC_{0A}\approx2048
$$

实际硬件存在误差，所以不能固定使用 2048，而要测真实 Offset。

---

## 3. 运放电流采样公式

当前电路运放增益约为：

$$
G=\frac{11k}{1.5k}\approx7.33
$$

因此：

$$
V_{out}=1.65+7.33(V_P-V_N)
$$

又因为：

$$
V_P-V_N=I\times0.005
$$

所以：

$$
V_{out}=1.65+0.0367I
$$

即每 1 A 电流大约使输出变化：

$$
36.7mV
$$

---

## 4. ADC 原始值换算电流

12 bit ADC 每个 Count 对应：

$$
\frac{3300}{4095}\approx0.806mV
$$

因此：

$$
1\text{ ADC count}
\approx
\frac{0.806}{36.7}
\approx21.98mA
$$

代码中：

```c
current_delta = (int32_t)raw - (int32_t)offset;
current_ma = current_delta * 2198L / 100L;
```

---

## 5. 零电流 Offset 校准

实际零点可能是：

```text
U相：2066
V相：1985
```

而不是统一的 2048。

因此启动后在确定无电流时，对多次 ADC 数据取平均：

```text
实际 Raw
-
实际 Offset
=
电流对应的 ADC 差值
```

核心关系：

$$
I\propto ADC_{raw}-ADC_{offset}
$$

---

## 6. ADC1 多通道 + DMA

ADC1 当前：

```text
Rank 1 → ADC1_IN1 → VDC
Rank 2 → ADC1_IN3 → U相电流
```

DMA：

```text
adc1_dma_buffer[0] → VDC
adc1_dma_buffer[1] → U相
```

由于 DMA 要依次写数组：

```text
Memory Increment = Enable
```

注意：

> ADC Rank 顺序决定 DMA Buffer 中的数据顺序。

---

## 7. 今天遇到的 Rank 配置问题

曾出现：

```text
VDC_ADC ≈ Current_ADC
```

原因是代码实际配置成：

```text
Rank1 → ADC_CHANNEL_1
Rank2 → ADC_CHANNEL_1
```

Rank2 缺少：

```c
sConfig.Channel = ADC_CHANNEL_3;
```

正确应为：

```text
Rank1 → ADC_CHANNEL_1
Rank2 → ADC_CHANNEL_3
```

---

## 8. U/V 两相采样结构

U 相：

```text
采样电阻
→ OPAMP1
→ ADC1_IN3
→ ADC1 DMA
```

V 相：

```text
采样电阻
→ OPAMP2
→ ADC2_IN3
→ ADC2 DMA
```

两路分别进行 Offset 校准后，目前：

```text
Iu ≈ 0 A
Iv ≈ 0 A
```

验证正常。

---

## 9. OPAMP 也需要 Start

和 Timer/PWM 一样：

```text
Init ≠ Start
```

配置完成后还需要：

```c
HAL_OPAMP_Start(&hopamp1);
HAL_OPAMP_Start(&hopamp2);
```

---

## 10. DMA 数据一致性问题

DMA 会在后台不断修改 ADC Buffer。

可能出现：

```text
CPU计算时 Raw = 2066
DMA随后更新 Raw = 2065
UART最后打印 Raw = 2065
```

于是出现：

```text
U=2065/2066, Iu=0 mA
```

看似计算错误，实际是计算和打印使用了不同时刻的数据。

解决方法：

```c
uint16_t u_raw = adc1_dma_buffer[1];
uint16_t v_raw = adc2_dma_value;
```

本轮计算和打印全部使用 `u_raw`、`v_raw`。

核心经验：

> DMA 与 CPU 异步访问数据时，先复制局部快照，再进行计算。

---

# 今天最需要记住

- 5 mΩ：电流采样电阻
- OPAMP：放大毫伏级采样信号
- 0 A ≈ 1.65 V ≈ ADC 中点
- 实际使用 Offset，而不是死记 2048
- 1 ADC Count ≈ 21.98 mA
- ADC Rank 决定 DMA Buffer 顺序
- 多通道数组 DMA 要开启 Memory Increment
- OPAMP：Init 后还要 Start
- DMA Buffer 会被异步更新，计算前最好先做数据快照

# 当前进度

- [x] GPIO / UART / Timer
- [x] 普通 PWM
- [x] ADC Polling
- [x] ADC + DMA
- [x] OPAMP 电流采样原理
- [x] U 相电流采样
- [x] V 相电流采样
- [x] U/V Offset 校准
- [x] ADC → mA 换算
- [x] DMA 数据一致性 Debug
- [ ] PWM 同步 ADC 采样
- [ ] CubeMonitor
- [ ] Clarke / Park
- [ ] PI
- [ ] SVPWM / FOC