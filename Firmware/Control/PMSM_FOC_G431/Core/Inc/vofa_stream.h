/*
 * vofa_stream.h
 *
 *  Created on: 2026年10月21日
 *      Author: CYC.YK
 */

#ifndef INC_VOFA_STREAM_H_
#define INC_VOFA_STREAM_H_

#include <stdint.h>

/* Send 6 float channels to VOFA+ JustFloat (USART3, DMA, 115200 baud).
 * ONLY call from foreground/main loop; never from the 20kHz ADC IRQ.
 * Returns 1 when DMA was started, 0 when previous transfer is busy/failed.
 * Frames are intentionally droppable; motor control takes priority.
 */
uint8_t VOFA_Stream_TrySend(float speed_ref_rpm,
                            float speed_actual_rpm,
                            float iq_ref_a,
                            float iq_actual_a,
                            float vdc_bus_v,
                            float uart_rx_ping_count);

#endif /* INC_VOFA_STREAM_H_ */
