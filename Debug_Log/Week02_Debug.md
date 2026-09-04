##  8.31

点亮LED灯一直失败，后面发现是将PC4引脚错误的配成了PA4.。。。

## 9.3

今天在完成好ADC基础采样之后开始进行学习DMA，然后将DMA相关配置好并修改好mian.c代码之后，运行发现串口一直没有反应，后续发现原因并不是代码错误，而是因为我开启了DMA中断，因为我只是让 DMA 永远覆盖一个“最新值”。这种情况下完全没必要每采一个ADC就通知CPU，否则就会导致DMA中断把CPU几乎给淹没了，导致CPU基本并没有机会回到while（1）执行这里。

解决办法很简单只需要关闭DMA的中断配置就行了，但是![image-20260903132747768](C:\Users\CYC.YK\AppData\Roaming\Typora\typora-user-images\image-20260903132747768.png)

CubeMX 把 DMA1 Channel1 的 NVIC 中断当成 DMA 配置的一部分强制打开了

所以我们只能用另一种方法，

`在：HAL_ADC_Start_DMA(&hadc1,`
                  `(uint32_t *)&adc_dma_value,`
                  `1);`

`后加：`

`HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT);`
`HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_TC);`

DMA到CPU可以理解为要通过两道门，现在CubeMX 强制把第二道门开着，我们现在把第一道门关掉：

DMA中断源
   ↓
第一道门：DMA_IT_TC / DMA_IT_HT
   ↓
第二道门：NVIC DMA1_Channel1_IRQn
   ↓
CPU

## 9.4

opamp 和ADC一样初始化后也要start，要不然不起作用，设置完之后就忘记了

然后同时开启ADC1的IN1 和 IN3 时候，ADC配置中，要记得将rank2中的channal选择为channal3 要不然系统会继续默认一直使用channal1 导致后面通道一直沿用采集的前面通道ADC的值



### DMA 数据偶尔不一致

### 问题现象

串口偶尔出现：

```text
U=2065/2066, Iu=0 mA
```

按理：

$$
2065-2066=-1
$$

应该得到约：

$$
I_u\approx-21\text{ mA}
$$

---

### 原因

ADC + DMA 连续工作时，DMA 会不断更新：

```c
adc1_dma_buffer[1]
```

可能出现：

```text
CPU计算时：Raw = 2066
DMA随后更新：Raw = 2065
UART打印时：显示2065
```

所以同一行里的 `Raw` 和 `Iu` 可能不是同一时刻的数据。

---

### 解决方法

先把 DMA 数据复制成快照：

```c
uint16_t u_raw = adc1_dma_buffer[1];
uint16_t v_raw = adc2_dma_value;
```

后面的计算和打印全部使用：

```c
u_raw
v_raw
```

不要反复直接读取 DMA Buffer。

---

## 结论

> DMA 会异步修改内存数据。连续采样时，先复制一份局部快照，再进行计算和打印，可以避免数据不一致。
