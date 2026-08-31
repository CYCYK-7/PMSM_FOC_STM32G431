#include <stdio.h>

#include "pi.h"
//gcc main.c pi.c -o output/main.exe
//.\output\main.exe

#define CONTROL_FREQ_HZ    20000.0f
#define CONTROL_TS         (1.0f / CONTROL_FREQ_HZ)

#define CURRENT_LOOP_NUM   8  //循环次数


int main(void)
{
    /* =========================
     * 创建 Id / Iq 两个 PI
     * ========================= */

    PI_Controller_t pi_d;
    PI_Controller_t pi_q;


    /* =========================
     * 电流参考
     * ========================= */

    const float id_ref = 0.0f;
    const float iq_ref = 2.0f; //const修饰的变量说明是常量，不能被修改，编译器会在编译时进行优化处理。



    /* =========================
     * 模拟采样得到的实际 Id
     * ========================= */

    float id_feedback[CURRENT_LOOP_NUM] =
    {
        0.30f,
        0.25f,
        0.20f,
        0.15f,
        0.10f,
        0.05f,
        0.02f,
        0.00f
    };


    /* =========================
     * 模拟采样得到的实际 Iq  数组
     * ========================= */

    float iq_feedback[CURRENT_LOOP_NUM] =
    {
        0.00f,
        0.00f,
        0.00f,
        0.00f,
        0.00f,
        0.00f,
        0.00f,
        0.00f,
    };


    float vd;
    float vq;


    /* =========================
     * PI 初始化
     * ========================= */

    PI_Init(&pi_d,
            1.0f,
            20.0f,
            CONTROL_TS,
           -10.0f,
            10.0f);


    PI_Init(&pi_q,
            2.0f,
            20.0f,
            CONTROL_TS,
           -10.0f,
            10.0f);


    printf("Control Frequency = %.0f Hz\n",
           CONTROL_FREQ_HZ);

    printf("Control Ts = %.6f s\n\n",
           CONTROL_TS);


    /* =========================
     * 模拟连续 8 个控制周期
     * ========================= */

    for (int i = 0;
         i < CURRENT_LOOP_NUM;
         i++)
    {
        vd = PI_Run(&pi_d,
                    id_ref,
                    id_feedback[i]);


        vq = PI_Run(&pi_q,
                    iq_ref,
                    iq_feedback[i]);


        printf("===== Cycle %d =====\n",
               i + 1);


        printf("Id Ref = %.2f A\n",
               id_ref);

        printf("Id     = %.2f A\n",
               id_feedback[i]);

        printf("Vd     = %.4f V\n",
               vd);


        printf("Iq Ref = %.2f A\n",
               iq_ref);

        printf("Iq     = %.2f A\n",
               iq_feedback[i]);

        printf("Vq     = %.4f V\n",
               vq);


        printf("D Integral = %.6f\n",
               pi_d.integral);

        printf("Q Integral = %.6f\n\n",
               pi_q.integral);
    }
    printf("Before Reset:\n");

    printf("D Integral = %.6f\n",
               pi_d.integral);

    printf("Q Integral = %.6f\n\n",
               pi_q.integral);
    PI_Reset(&pi_d);
    PI_Reset(&pi_q);
    printf("After Reset:\n");

    printf("D Integral = %.6f\n",
              pi_d.integral);

    printf("Q Integral = %.6f\n",
              pi_q.integral);
    return 0;
}