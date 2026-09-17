/*
 * svpwm.h
 *
 *  Created on: 2026年9月15日
 *      Author: CYC.YK
 */

#ifndef INC_SVPWM_H_
#define INC_SVPWM_H_


typedef struct
{
    float duty_u;
    float duty_v;
    float duty_w;

} SVPWM_Output_t;


void SVPWM_Run(float v_alpha,
               float v_beta,
               float vdc,
               SVPWM_Output_t *out);

void SVPWM_LimitVoltage(float *v_alpha,
                        float *v_beta,
                        float vdc);




#endif /* INC_SVPWM_H_ */
