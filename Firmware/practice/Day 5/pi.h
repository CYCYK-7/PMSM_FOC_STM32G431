#ifndef PI_H
#define PI_H

//.h文件**模块对外公开的说明书 / 接口。**
/* =========================
 * PI 控制器数据类型
 * ========================= */

typedef struct
{
    float kp;
    float ki;

    float integral;

    float output_max;
    float output_min;

    float output;

} PI_Controller_t;


/* =========================
 * PI 模块公开函数
 * ========================= */

void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float output_min,
             float output_max);

float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback);

void PI_Reset(PI_Controller_t *pi);
float PI_GetOutput(PI_Controller_t *pi);

#endif