/*
 * pi_controller.h
 *
 *  Created on: 2026年9月10日
 *      Author: CYC.YK
 */

#ifndef INC_PI_CONTROLLER_H_
#define INC_PI_CONTROLLER_H_



typedef struct
{
    float kp;
    float ki;

    float integral;

    float output_max;
    float output_min;

    float output;

} PI_Controller_t;


void PI_Init(PI_Controller_t *pi,
             float kp,
             float ki,
             float output_min,
             float output_max);

float PI_Run(PI_Controller_t *pi,
             float ref,
             float feedback,
             float ts);

void PI_Reset(PI_Controller_t *pi);



#endif /* INC_PI_CONTROLLER_H_ */
