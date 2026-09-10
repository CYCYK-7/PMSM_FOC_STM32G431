/*
 * foc_math.c
 *
 *  Created on: Sep 8, 2026
 *      Author: CYC.YK
 */



#include "foc_math.h"

#define ONE_BY_SQRT3  0.577350269f


void Clarke_Run(float iu,
                float iv,
                AlphaBeta_t *out)
{
    out->alpha = iu;

    out->beta =
        ONE_BY_SQRT3 * (iu + 2.0f * iv);
}

