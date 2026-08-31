#include "pi.h"

//.c文件**模块的实现 / 具体功能。**
/* =========================
 * 初始化 PI
 * ========================= */

void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float output_min,
             float output_max)
{
    pi->kp = kp;
    pi->ki = ki;

    pi->integral = 0.0f;

    pi->output_min = output_min;
    pi->output_max = output_max;

    pi->output = 0.0f;
}


/* =========================
 * 运行一次 PI
 * ========================= */

float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback)
{
    float error;

    error = ref - feedback;


    /* 积分 */
    pi->integral += error;


    /* PI 输出 */
    pi->output =
        pi->kp * error
        +
        pi->ki * pi->integral;


    /* 输出上限 */
    if (pi->output > pi->output_max)
    {
        pi->output = pi->output_max;
    }


    /* 输出下限 */
    if (pi->output < pi->output_min)
    {
        pi->output = pi->output_min;
    }


    return pi->output;
}


/* =========================
 * 复位 PI
 * ========================= */

void PI_Reset(PI_Controller_t *pi)
{
    pi->integral = 0.0f;
    pi->output = 0.0f;
}

float PI_GetOutput(PI_Controller_t *pi)
{
    return pi->output;
}

// 如果在非main.c文件中出现了static函数，并且没在pi.h中声明，那么这个函数就只能在这个文件中使用，说明作者并不喜欢其他文件访问到这个函数。
//可以理解为static函数是pi.c私有函数，非static函数是公有函数。
static float PI_Limit(float value,
                      float min,
                      float max)
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

