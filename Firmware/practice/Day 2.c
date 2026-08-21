#include <stdio.h>
#include <stdint.h>


void Read_Phase_Current(float *ia,
                        float *ib,
                        float *ic) //另一种常见的写法
{
    *ia = 1.0f;
    *ib = -0.4f;
    *ic = -0.6f;
}

/* =========================
 * 三相电流
 * ========================= */

float current_abc[3] =  // 数组是从0开始的，这点一定要记住，不要数组越界了！！！
{
    2.00f,
   -1.00f,
   -1.00f
};


/* =========================
 * αβ 电流
 * ========================= */

float current_alpha_beta[2] =
{
    0.0f,
    0.0f
};


/* =========================
 * dq 电流
 * ========================= */

float current_dq[2] =
{
    0.0f,
    0.0f
};


/* =========================
 * 修改变量值
 * ========================= */

void Set_Value(float *value, float new_value)
{
    *value = new_value;
}


/* =========================
 * 电流限幅
 * ========================= */

void Current_Limit(float *current_ref, float limit) //如果函数需要修改某个外部变量，指针就是一种常见手段。
{
    if (*current_ref > limit)
    {
        *current_ref = limit;
    }

    if (*current_ref < -limit)
    {
        *current_ref = -limit;
    }
}


/* =========================
 * 打印三相电流
 * ========================= */

void Print_ABC_Current(float current[3])
{
    current[0] = 3.00f;  //修改了数组的第一个元素，注意这里是通过指针传递的，所以会影响到原始数组。
    printf("Ia = %.2f A\n", current[0]);
    printf("Ib = %.2f A\n", current[1]);
    printf("Ic = %.2f A\n", current[2]);
}


/* =========================
 * 主程序
 * ========================= */

int main(void)
{
    float iq_ref = 5.0f;
    float ia;
    float ib;
    float ic;

    Read_Phase_Current(&ia, &ib, &ic);
    printf("ia = %.2f A\n", ia);
    printf("ib = %.2f A\n", ib);
    printf("ic = %.2f A\n", ic);

    printf("===== Original ABC Current =====\n");

    Print_ABC_Current(current_abc);


    printf("\n===== Pointer Test =====\n");

    printf("Original iq_ref = %.2f A\n", iq_ref);

    Set_Value(&iq_ref, 4.0f);

    printf("After Set_Value = %.2f A\n", iq_ref);


    printf("\n===== Current Limit Test =====\n");

    iq_ref = -8.0f;

    printf("Before Limit = %.2f A\n", iq_ref);

    Current_Limit(&iq_ref, 3.0f);

    printf("After Limit  = %.2f A\n", iq_ref);

    return 0;
}