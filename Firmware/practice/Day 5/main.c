#include <stdio.h>

#include "pi.h"


int main(void)
{
    PI_Controller_t pi_d;
    PI_Controller_t pi_q;


    float id_ref = 0.0f;
    float id = 0.2f;


    float iq_ref = 2.0f;
    float iq = 0.5f;


    float vd;
    float vq;


    /* =========================
     * 初始化两个 PI
     * ========================= */

    PI_Init(&pi_d,
            1.0f,
            0.1f,
           -10.0f,
            10.0f);


    PI_Init(&pi_q,
            1.2f,
            0.1f,
           -10.0f,
            10.0f);


    /* =========================
     * 模拟一次电流环运行
     * ========================= */

    vd = PI_Run(&pi_d,
                id_ref,
                id);


    vq = PI_Run(&pi_q,
                iq_ref,
                iq);


    printf("===== Current Loop =====\n");

    printf("Id Ref = %.2f A\n", id_ref);
    printf("Id     = %.2f A\n", id);
    printf("Vd     = %.2f V\n", vd);

    printf("\n");

    printf("Iq Ref = %.2f A\n", iq_ref);
    printf("Iq     = %.2f A\n", iq);
    printf("Vq     = %.2f V\n", vq);


    return 0;
}