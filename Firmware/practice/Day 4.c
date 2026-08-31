#include <stdio.h>
#include <stdint.h>


/* =========================
 * 宏定义
 * ========================= */

#define PWM_FREQ_HZ        20000U
#define CONTROL_FREQ_HZ    20000U

#define MOTOR_ENABLE       1U
#define MOTOR_DISABLE      0U

#define CURRENT_LIMIT_A    5.0f

#define TWO_PI             6.2831853f


/* =========================
 * 常量 ：一般这个变量定义之后，不希望再被修改。是一种对代码的保护和约束
 * ========================= */

const float control_ts = 1.0f / CONTROL_FREQ_HZ;


/* =========================
 * 模拟硬件/中断共享变量  //volatile的作用是告诉编译器，这个变量可能会被其他线程或中断程序修改，所以每次访问这个变量时都要从内存中重新读取，而不是使用寄存器中的缓存值。这样可以确保程序在多线程或中断环境下能够正确地读取和写入这个变量的值。
 * ========================= */

volatile uint16_t adc_current_raw = 2048U;

volatile uint8_t adc_ready = 0U;

volatile uint32_t control_isr_count = 0U;


/* =========================
 * 普通全局变量
 * ========================= */

float phase_current = 0.0f;

float iq_ref = 0.0f;

uint8_t motor_enable = MOTOR_DISABLE;


/* =========================
 * 模拟ADC中断
 * ========================= */

void ADC_Interrupt_Handler(void)
{
    adc_current_raw = 2148U;

    adc_ready = 1U;
}


/* =========================
 * ADC转换电流
 * ========================= */

float ADC_To_Current(uint16_t adc_raw)
{
    const float adc_offset = 2048.0f;

    const float current_scale = 0.01f;

    float current;

    current =
        ((float)adc_raw - adc_offset)
        * current_scale;

    return current;
}


/* =========================
 * 控制中断
 * ========================= */

void Motor_Control_ISR(void)
{
    static float last_current = 0.0f;  //static的作用是,使你定义的这个变量只要定义一次,就会一直存在,不会随着函数的调用而消失,
                                      //也不会随着函数的调用而重新定义（比如后面再调用这个函数，这个变量的值不是0.0f，而是上一次运行后的值）,它的值会一直保留着,直到程序结束. 也就是说,static变量的生命周期是整个程序运行期间.
                                      //static很方便用来保留历史状态
    control_isr_count++;


    if (adc_ready == 1U)
    {
        phase_current =
            ADC_To_Current(adc_current_raw);

        last_current = phase_current;

        adc_ready = 0U;
    }


    printf("Current     = %.2f A\n",
           phase_current);

    printf("Last Current= %.2f A\n",
           last_current);

    printf("ISR Count   = %lu\n",
           (unsigned long)control_isr_count);
}


/* =========================
 * 电流限幅
 * ========================= */

void Current_Limit(void)
{
    if (iq_ref > CURRENT_LIMIT_A)
    {
        iq_ref = CURRENT_LIMIT_A;
    }

    if (iq_ref < -CURRENT_LIMIT_A)
    {
        iq_ref = -CURRENT_LIMIT_A;
    }
}

void Static_Test(void)
{
     uint32_t count = 0U;  // 看看加与不加static的区别

    count++;

    printf("Static Count = %lu\n",
           (unsigned long)count);
}

/* =========================
 * 主程序
 * ========================= */

int main(void)
{
    Static_Test();
    Static_Test();
    Static_Test();
    printf("PWM Frequency     = %u Hz\n",
           PWM_FREQ_HZ);

    printf("Control Frequency = %u Hz\n",
           CONTROL_FREQ_HZ);

    printf("Control Ts        = %.6f s\n",
           control_ts);


    motor_enable = MOTOR_ENABLE;

    iq_ref = 8.0f;

    Current_Limit();

    printf("Iq Ref After Limit = %.2f A\n",
           iq_ref);


    ADC_Interrupt_Handler();

    Motor_Control_ISR();

    Motor_Control_ISR();

    Motor_Control_ISR();


    return 0;
}