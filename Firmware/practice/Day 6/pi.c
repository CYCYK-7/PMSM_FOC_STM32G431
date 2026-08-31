#include "pi.h"
#include <stdio.h>

/* =========================
 * PI 内部限幅函数
 * ========================= */

static float PI_Limit(float value,
                      float min,
                      float max)  //static函数只能在本文件中使用，其他文件无法访问
{
    if (value > max)
    {
        value = max;
    }

    if (value < min)
    {
        value = min;
    }

    return value;
}


/* =========================
 * PI 初始化
 * ========================= */

void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float ts,
             float output_min,
             float output_max)
{
    pi->kp = kp;
    pi->ki = ki;

    pi->ts = ts;

    pi->integral = 0.0f;
    pi->output = 0.0f;

    pi->output_min = output_min;
    pi->output_max = output_max;
}


/* =========================
 * PI 运行一次
 * ========================= */

float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback)
{
    float error;


    /* 1. 计算误差 */
    error = ref - feedback;

    printf("Error = %.2f\n", error);


    /* 2. 更新积分 */
    pi->integral += error * pi->ts;


    /* 3. PI 输出 */
    pi->output =
        pi->kp * error
        +
        pi->ki * pi->integral;


    /* 4. 输出限幅 */
    pi->output =
        PI_Limit(pi->output,
                 pi->output_min,
                 pi->output_max);


    /* 5. 返回结果 */
    return pi->output;
}


/* =========================
 * PI 复位
 * ========================= */

void PI_Reset(PI_Controller_t *pi)
{
    pi->integral = 0.0f;

    pi->output = 0.0f;
}