/*
 * svpwm.c
 *
 *  Created on: 2026年9月15日
 *      Author: CYC.YK
 */


#include "svpwm.h"
#include <math.h>

#define SQRT3_BY_2  0.866025404f
#define ONE_BY_SQRT3  0.577350269f


static float Clamp(float x,
                   float min,
                   float max)
{
    if (x > max)
    {
        return max;
    }

    if (x < min)
    {
        return min;
    }

    return x;
}


void SVPWM_Run(float v_alpha,
               float v_beta,
               float vdc,
               SVPWM_Output_t *out)
{
    float vu;
    float vv;
    float vw;

    float vmax;
    float vmin;
    float voffset;

    /* Prevent division by zero */
    if (vdc < 1.0f)
    {
        out->duty_u = 0.5f;
        out->duty_v = 0.5f;
        out->duty_w = 0.5f;

        return;
    }


    /* Inverse Clarke */
    vu = v_alpha;

    vv =
        -0.5f * v_alpha
        + SQRT3_BY_2 * v_beta;

    vw =
        -0.5f * v_alpha
        - SQRT3_BY_2 * v_beta;


    /* Find maximum */
    vmax = vu;

    if (vv > vmax)
    {
        vmax = vv;
    }

    if (vw > vmax)
    {
        vmax = vw;
    }


    /* Find minimum */
    vmin = vu;

    if (vv < vmin)
    {
        vmin = vv;
    }

    if (vw < vmin)
    {
        vmin = vw;
    }


    /* Common-mode voltage injection */
    voffset =
        -0.5f * (vmax + vmin);


    vu += voffset;
    vv += voffset;
    vw += voffset;


    /* Voltage -> Duty */
    out->duty_u =
        0.5f + vu / vdc;

    out->duty_v =
        0.5f + vv / vdc;

    out->duty_w =
        0.5f + vw / vdc;


    /* Safety clamp */ //安全限幅
    out->duty_u =
        Clamp(out->duty_u,
              0.0f,
              1.0f);

    out->duty_v =
        Clamp(out->duty_v,
              0.0f,
              1.0f);

    out->duty_w =
        Clamp(out->duty_w,
              0.0f,
              1.0f);
}

void SVPWM_LimitVoltage(float *v_alpha,
                        float *v_beta,
                        float vdc)
{
    float vmax;
    float magnitude_sq;
    float vmax_sq;
    float magnitude;
    float scale;

    if (vdc <= 0.0f)
    {
        *v_alpha = 0.0f;
        *v_beta  = 0.0f;

        return;
    }


    vmax =
        vdc * ONE_BY_SQRT3;


    magnitude_sq =
        (*v_alpha) * (*v_alpha)
        +
        (*v_beta) * (*v_beta);


    vmax_sq =
        vmax * vmax;


    if (magnitude_sq > vmax_sq)
    {
        magnitude =
            sqrtf(magnitude_sq);

        scale =
            vmax / magnitude;


        *v_alpha *= scale;
        *v_beta  *= scale;
    }
}
