#include <stdio.h>
#include <stdint.h>

/* =========================
 * 模拟电机控制系统中的全局变量
 * ========================= */

/* ADC原始采样值：0~4095 */
uint16_t adc_current_raw = 2048;

/* 母线电压ADC原始值 */
uint16_t adc_vbus_raw = 3000;

/* PWM计数值 */
uint16_t pwm_compare = 500;

/* 控制中断运行次数 */
uint32_t control_loop_count = 0;

/* 电机运行状态 */
uint8_t motor_enable = 0;

/* 电机转速，单位 rpm */
int16_t motor_speed_rpm = 0;

/* 实际物理量 */
float phase_current = 0.0f;
float dc_bus_voltage = 0.0f;


/* =========================
 * ADC原始值转换为电流
 * ========================= */
float ADC_To_Current(uint16_t adc_raw)
{
    float adc_offset = 2048.0f;
    float current_scale = 0.01f;

    float current;

    current = ((float)adc_raw - adc_offset) * current_scale;

    return current;
}


/* =========================
 * ADC原始值转换为母线电压
 * ========================= */
float ADC_To_Vbus(uint16_t adc_raw)
{
    float voltage_scale = 0.02f;

    float voltage;

    voltage = (float)adc_raw * voltage_scale;

    return voltage;
}


/* =========================
 * 模拟一次电机控制周期
 * ========================= */
void Motor_Control_Loop(void)
{
    control_loop_count++;

    phase_current = ADC_To_Current(adc_current_raw);

    dc_bus_voltage = ADC_To_Vbus(adc_vbus_raw);

    if (motor_enable == 1)
    {
        motor_speed_rpm = 1000;
    }
    else
    {
        motor_speed_rpm = 0;
    }
}


/* =========================
 * 主程序
 * ========================= */
int main(void)
{
    motor_enable = 1;

    Motor_Control_Loop();

    printf("Motor Enable      = %d\n", motor_enable);
    printf("ADC Current Raw   = %d\n", adc_current_raw);
    printf("Phase Current     = %.2f A\n", phase_current);
    printf("DC Bus Voltage    = %.2f V\n", dc_bus_voltage);
    printf("Motor Speed       = %d rpm\n", motor_speed_rpm);
    printf("Control Loop Count= %lu\n",
           (unsigned long)control_loop_count);

    return 0;
}