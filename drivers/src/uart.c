/**
 * @file uart.c
 * @brief UART driver implementation (API in uart.h).
 */
#include "uart.h"

/**
 * @addtogroup uart_driver
 * @{
 */

/** Smallest valid BRR: USARTDIV below 1, a baud rate above pclk / 16, cannot be generated. */
#define UART_BRR_MIN (16U)

/**
 * @brief Compute BRR for 16x oversampling: `round(pclk_hz / baud)`.
 *
 * BRR's mantissa (bits 15:4) and sixteenths (bits 3:0) together are just
 * USARTDIV x 16 = pclk_hz / baud, so the rounded quotient is the register
 * value. Rounding once at the end matters: 42 MHz at 115200 is 364.58,
 * which rounds to 365 where a truncated intermediate gives 364. The sum is
 * done in 64 bits because @p pclk_hz is unbounded.
 *
 * @param pclk_hz Bus clock, in Hz.
 * @param baud    Baud rate, in bit/s.
 * @param brr     Receives the value; untouched on failure.
 * @retval DRIVER_STATUS_OK                Computed.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM An input is 0, or the result is below
 *                                         ::UART_BRR_MIN or above 16 bits.
 */
static DriverStatus_e uartComputeBrr(uint32_t pclk_hz, uint32_t baud, uint32_t *brr)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((pclk_hz == 0U) || (baud == 0U))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else
    {
        uint64_t rounded = ((uint64_t)pclk_hz + ((uint64_t)baud / 2U)) / (uint64_t)baud;

        if (rounded < UART_BRR_MIN)
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
        else if (rounded > (USART_BRR_DIV_MANTISSA_Msk | USART_BRR_DIV_FRACTION_Msk))
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
        else
        {
            *brr = (uint32_t)rounded;
        }
    }

    return status;
}

DriverStatus_e uartInit(UartRegisters_t *uart, uint32_t pclk_hz, uint32_t baud, uint32_t stop_bits,
                        uint32_t parity_enable, uint32_t parity_select, uint32_t direction)
{
    uint32_t brr = 0U;
    DriverStatus_e status = uartComputeBrr(pclk_hz, baud, &brr);

    if (status == DRIVER_STATUS_OK)
    {
        /* 0.5 and 1.5 stop bits are smartcard-only; not offered. */
        if ((stop_bits != USART_CR2_STOP_1) && (stop_bits != USART_CR2_STOP_2))
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    if (status == DRIVER_STATUS_OK)
    {
        if ((parity_enable != 0U) && (parity_enable != USART_CR1_PCE))
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    if (status == DRIVER_STATUS_OK)
    {
        if ((parity_select != 0U) && (parity_select != USART_CR1_PS))
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    if (status == DRIVER_STATUS_OK)
    {
        if ((direction & ~(USART_CR1_TE | USART_CR1_RE)) != 0U)
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    if (status == DRIVER_STATUS_OK)
    {
        /* OVER8 is cleared because BRR assumes 16x oversampling. M is set with
         * parity: the parity bit counts inside the M frame, so M clear would
         * leave 7 data bits and overwrite bit 7 with parity. */
        uint32_t cr1_mask = USART_CR1_M | USART_CR1_OVER8 | USART_CR1_PCE | USART_CR1_PS
                            | USART_CR1_TE | USART_CR1_RE;
        uint32_t frame_length = 0U;

        if (parity_enable != 0U)
        {
            frame_length = USART_CR1_M;
        }

        /* Disable first so a reconfiguration never changes BRR mid-frame. */
        uart->CR1 &= ~USART_CR1_UE;

        uart->BRR = brr;
        uart->CR2 = (uart->CR2 & ~USART_CR2_STOP_Msk) | (stop_bits << USART_CR2_STOP_Pos);
        uart->CR1 =
            (uart->CR1 & ~cr1_mask) | frame_length | parity_enable | parity_select | direction;

        /* Enable last, once every field is set. */
        uart->CR1 |= USART_CR1_UE;
    }

    return status;
}

void uartDeinit(UartRegisters_t *uart)
{
    uart->CR1 = 0U;
    uart->CR2 = 0U;
    uart->CR3 = 0U;
    uart->BRR = 0U;
    uart->GTPR = 0U;
}

/**
 * @brief Poll an SR flag until it sets or @p timeout polls pass.
 *
 * The flag is re-read after the loop, so one that sets on the last poll
 * counts as success.
 *
 * @param uart    Instance.
 * @param flag    ::USART_SR_TXE, ::USART_SR_TC or ::USART_SR_RXNE.
 * @param timeout Polls to allow; 0 checks once.
 * @retval DRIVER_STATUS_OK          The flag is set.
 * @retval DRIVER_STATUS_ERR_TIMEOUT It did not set in time.
 */
static DriverStatus_e uartWaitSrFlag(const UartRegisters_t *uart, uint32_t flag, uint32_t timeout)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t remaining = timeout;

    while (((uart->SR & flag) == 0U) && (remaining > 0U))
    {
        remaining--;
    }

    if ((uart->SR & flag) == 0U)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }

    return status;
}

DriverStatus_e uartFlush(const UartRegisters_t *uart, uint32_t timeout)
{
    return uartWaitSrFlag(uart, USART_SR_TC, timeout);
}

DriverStatus_e uartTransmitByte(UartRegisters_t *uart, uint8_t byte, uint32_t timeout)
{
    DriverStatus_e status = uartWaitSrFlag(uart, USART_SR_TXE, timeout);

    if (status == DRIVER_STATUS_OK)
    {
        uart->DR = byte;
    }

    return status;
}

DriverStatus_e uartTransmit(UartRegisters_t *uart, const uint8_t *data, size_t length,
                            uint32_t timeout)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((data == NULL) && (length > 0U))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    for (size_t i = 0U; (i < length) && (status == DRIVER_STATUS_OK); i++)
    {
        status = uartTransmitByte(uart, data[i], timeout);
    }

    /* Wait for TC so the line is idle. Skipped for length 0: nothing was sent. */
    if ((status == DRIVER_STATUS_OK) && (length > 0U))
    {
        status = uartFlush(uart, timeout);
    }

    return status;
}

/* cppcheck-suppress constParameterPointer
 * Nothing is written through `uart`, but reading DR clears RXNE and the error
 * flags, so the call changes the peripheral: see docs/DESIGN.md,
 * "const register parameters". */
DriverStatus_e uartReceiveByte(UartRegisters_t *uart, uint8_t *byte, uint32_t timeout)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if (byte == NULL)
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else
    {
        status = uartWaitSrFlag(uart, USART_SR_RXNE, timeout);
    }

    if (status == DRIVER_STATUS_OK)
    {
        uint32_t sr = uart->SR;

        *byte = (uint8_t)uart->DR;

        if ((sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0U)
        {
            status = DRIVER_STATUS_ERR_HW_FAULT;
        }
    }

    return status;
}

DriverStatus_e uartReceive(UartRegisters_t *uart, uint8_t *data, size_t length, uint32_t timeout)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((data == NULL) && (length > 0U))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    for (size_t i = 0U; (i < length) && (status == DRIVER_STATUS_OK); i++)
    {
        status = uartReceiveByte(uart, &data[i], timeout);
    }

    return status;
}

/** @} */
