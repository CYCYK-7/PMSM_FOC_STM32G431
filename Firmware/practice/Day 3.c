#include <stdio.h>


/* =========================
 * PI 控制器结构体
 * ========================= */

typedef struct  // typedef 的作用是给结构体类型起一个别名，方便后续使用
{
    float kp;
    float ki;

    float integral;

    float output_max;
    float output_min;

    float output;

} PI_Controller_t;  //结构体后面名字后面有_t,只是非常常见的命名习惯,通常就是在告诉你,这是一个 type，数据类型。


/* =========================
 * FOC 电流数据结构体
 * ========================= */

typedef struct
{
    float ia;
    float ib;
    float ic;

    float i_alpha;
    float i_beta;

    float id;
    float iq;

} FOC_Current_t;


/* =========================
 * 电机运行状态结构体
 * ========================= */

typedef struct
{
    float speed_rpm;
    float electrical_angle;

    float vbus;

    unsigned char enable;

} Motor_State_t;


/* =========================
 * 初始化 PI
 * ========================= */

void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float output_min,
             float output_max)
{
    pi->kp = kp;  //其实和(*pi).kp = kp;是一样的，都是访问结构体成员的方式，只是语法不同而已。
                   // pi 代表结构体地址，*pi代表地址对应的结构体本身
                   // 结构体对象时用“.”，结构体指针时用“->”。
    pi->ki = ki;

    pi->integral = 0.0f;

    pi->output_min = output_min;
    pi->output_max = output_max;

    pi->output = 0.0f;
}


/* =========================
 * 复位 PI
 * ========================= */

void PI_Reset(PI_Controller_t *pi)
{
    pi->integral = 0.0f;
    pi->output = 0.0f;
}


/* =========================
 * 打印 PI 参数
 * ========================= */

void PI_Print(PI_Controller_t *pi)
{
    printf("Kp       = %.2f\n", pi->kp);
    printf("Ki       = %.2f\n", pi->ki);
    printf("Integral = %.2f\n", pi->integral);
    printf("Output   = %.2f\n", pi->output);
}


/* =========================
 * 主程序
 * ========================= */

int main(void)
{
    PI_Controller_t pi_d;  //PI_Controller_t 不是变量，是一种数据类型。
    PI_Controller_t pi_q;

    FOC_Current_t current;

    Motor_State_t motor;


    /* 初始化 d 轴 PI */
    PI_Init(&pi_d,
            1.0f,
            20.0f,
           -10.0f,
            10.0f);


    /* 初始化 q 轴 PI */
    PI_Init(&pi_q,
            1.2f,
            25.0f,
           -10.0f,
            10.0f);


    /* 设置 FOC 电流 */
    current.ia = 1.2f;
    current.ib = -0.5f;
    current.ic = -0.7f;

    current.id = 0.1f;
    current.iq = 2.0f;


    /* 设置电机状态 */
    motor.speed_rpm = 1000.0f;
    motor.electrical_angle = 1.57f;
    motor.vbus = 48.0f;
    motor.enable = 1;

    pi_d.integral = 100.0f;
    pi_d.output = 8.0f;

    printf("===== D-axis PI =====\n");
    PI_Print(&pi_d);

    PI_Reset(&pi_d);

    printf("===== D-axis PI =====\n");
    PI_Print(&pi_d);

    printf("\n===== Q-axis PI =====\n");
    PI_Print(&pi_q);


    printf("\n===== FOC Current =====\n");

    printf("Ia = %.2f A\n", current.ia);
    printf("Ib = %.2f A\n", current.ib);
    printf("Ic = %.2f A\n", current.ic);

    printf("Id = %.2f A\n", current.id);
    printf("Iq = %.2f A\n", current.iq);


    printf("\n===== Motor State =====\n");

    printf("Speed = %.2f rpm\n", motor.speed_rpm);
    printf("Angle = %.2f rad\n", motor.electrical_angle);
    printf("Vbus  = %.2f V\n", motor.vbus);
    printf("Enable= %d\n", motor.enable);


    return 0;
}