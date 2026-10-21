#include "vofa_stream.h"
#include "main.h"
#include <string.h>

extern UART_HandleTypeDef huart3;

/* CH0..CH4: existing FOC signals; CH5: USART3 ASCII 'P' RX counter.
 * 6 float32 values (24 bytes) + JustFloat tail (4 bytes) = 28 bytes.
 * UART DMA buffer must persist until transfer is finished. */
#define VOFA_CHANNELS 6U
#define VOFA_TX_LEN ((VOFA_CHANNELS + 1U) * 4U)
static uint8_t vofa_tx_buf[VOFA_TX_LEN];

uint8_t VOFA_Stream_TrySend(float speed_ref_rpm,
                            float speed_actual_rpm,
                            float iq_ref_a,
                            float iq_actual_a,
                            float vdc_bus_v,
                            float uart_rx_ping_count)
{
    /* Tx DMA and Rx byte interrupts run independently on USART3. */
    if (huart3.gState != HAL_UART_STATE_READY)
        return 0U;

    const float values[VOFA_CHANNELS] = {
        speed_ref_rpm, speed_actual_rpm, iq_ref_a, iq_actual_a,
        vdc_bus_v, uart_rx_ping_count
    };
    /* STM32G431 float32 little-endian payload; JustFloat fixed terminator. */
    memcpy(vofa_tx_buf, values, sizeof(values));
    vofa_tx_buf[24] = 0x00U;
    vofa_tx_buf[25] = 0x00U;
    vofa_tx_buf[26] = 0x80U;
    vofa_tx_buf[27] = 0x7FU;

    return (HAL_UART_Transmit_DMA(&huart3, vofa_tx_buf,
                                  (uint16_t)sizeof(vofa_tx_buf)) == HAL_OK)
             ? 1U : 0U;
}
