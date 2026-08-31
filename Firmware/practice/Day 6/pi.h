#ifndef PI_H
#define PI_H


typedef struct
{
    /* PI 参数 */
    float kp;
    float ki;

    /* 控制周期 */
    float ts;    //真正的数字PI必须知道自己的采样(控制)周期

    /* 运行状态 */
    float integral;
    float output;

    /* 输出限制 */
    float output_min;
    float output_max;

} PI_Controller_t;


/* 初始化 PI */
void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float ts,
             float output_min,
             float output_max);


/* 执行一次 PI */
float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback);


/* 清除积分和输出 */
void PI_Reset(PI_Controller_t *pi);


#endif