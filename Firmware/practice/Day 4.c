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
 * 常量
 * ========================= */

const float control_ts = 1.0f / CONTROL_FREQ_HZ;


/* =========================
 * 模拟硬件/中断共享变量
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
    static float last_current = 0.0f;

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


/* =========================
 * 主程序
 * ========================= */

int main(void)
{
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