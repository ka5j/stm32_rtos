/**
 * @file uart.c
 * @brief UART/USART driver implementation - see uart.h for the public API
 *        and uart_reg.h for the register definitions this operates on.
 */
#include "uart.h"

/**
 * @addtogroup driver_layer
 * @{
 */

/** Bounded retry count for SR.TXE/RXNE, not a wall-clock timeout - see
 *  uart.h's file-level comment for why. */
#define UART_READY_TIMEOUT_ITERATIONS (100000U)

/**
 * @brief Compute BRR from a bus clock and target baud rate, standard 16x
 *        oversampling (OVER8 left clear).
 *
 * USARTDIV = pclk / (16 * baud); RM0390 stores it as a 12-bit mantissa
 * plus a 4-bit fraction (fraction/16). Computed as USARTDIV*100 in a
 * 64-bit intermediate - 25*pclk can exceed uint32_t range for a
 * caller-supplied pclk_hz this function has no way to bound in advance -
 * to get two decimal digits of the fractional part without floating
 * point, matching RM0390's own worked examples; the fraction is then
 * rounded to the nearest 16th, carrying into the mantissa if that
 * rounds up to 16.
 *
 * @param pclk_hz Bus clock frequency in Hz.
 * @param baud    Target baud rate in bit/s.
 * @param brr     Set to the computed BRR value on success; untouched on
 *                failure.
 * @return DRIVER_STATUS_OK on success.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if pclk_hz or baud is 0, if the
 *         mantissa would exceed BRR's 12-bit field (baud too low for
 *         pclk_hz), or if both the mantissa and fraction compute to 0
 *         (baud too high for pclk_hz - RM0390 forbids a BRR of 0).
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
        uint64_t usartdiv_x100 = ((uint64_t)pclk_hz * 25U) / ((uint64_t)baud * 4U);
        uint32_t mantissa = (uint32_t)(usartdiv_x100 / 100U);
        uint32_t fraction_x100 = (uint32_t)(usartdiv_x100 % 100U);
        uint32_t fraction = ((fraction_x100 * 16U) + 50U) / 100U;

        if (fraction >= 16U)
        {
            mantissa++;
            fraction = 0U;
        }

        if ((mantissa == 0U) && (fraction == 0U))
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
        else if (mantissa > (USART_BRR_DIV_MANTISSA_Msk >> USART_BRR_DIV_MANTISSA_Pos))
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
        else
        {
            *brr =
                (mantissa << USART_BRR_DIV_MANTISSA_Pos) | (fraction << USART_BRR_DIV_FRACTION_Pos);
        }
    }

    return status;
}

DriverStatus_e uartInit(UartRegisters_t *uart, uint32_t pclk_hz, uint32_t baud, uint32_t stop_bits,
                        uint32_t parity_enable, uint32_t parity_select, uint32_t direction)
{
    uint32_t brr = 0U;
    DriverStatus_e status = uartComputeBrr(pclk_hz, baud, &brr);

    /* Every USART_CR1/USART_CR2_STOP value below is a fixed, tested
     * compile-time bit value (device/inc/uart_reg.h), not a runtime-
     * computed shift - cppcheck's MISRA addon can't bound a shift
     * through a macro expansion like this and flags every reference to
     * it; see drivers/src/rcc.c's matching comment on rccHseEnable() for
     * the same finding on the same class of macro. */
    if (status == DRIVER_STATUS_OK)
    {
        if ((stop_bits != USART_CR2_STOP_1) && (stop_bits != USART_CR2_STOP_0_5)
            && (stop_bits != USART_CR2_STOP_2) && (stop_bits != USART_CR2_STOP_1_5))
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    if (status == DRIVER_STATUS_OK)
    {
        // cppcheck-suppress misra-c2012-12.2
        if ((parity_enable != 0U) && (parity_enable != USART_CR1_PCE))
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    if (status == DRIVER_STATUS_OK)
    {
        // cppcheck-suppress misra-c2012-12.2
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
        /* USART_CR1_M is cleared (8 data bits, no parameter for it - see
         * this file's file-level comment) by being included in cr1_mask
         * and never OR'd back in below. */
        uint32_t cr1_mask =
            // cppcheck-suppress misra-c2012-12.2
            USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE | USART_CR1_RE;

        uart->BRR = brr;
        // cppcheck-suppress misra-c2012-12.2
        uart->CR2 = (uart->CR2 & ~USART_CR2_STOP_Msk) | (stop_bits << USART_CR2_STOP_Pos);
        uart->CR1 = (uart->CR1 & ~cr1_mask) | parity_enable | parity_select | direction;

        /* UE set last, after every other field is already correct - see
         * this function's own doc comment. */
        // cppcheck-suppress misra-c2012-12.2
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

DriverStatus_e uartTransmitByte(UartRegisters_t *uart, uint8_t byte)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = UART_READY_TIMEOUT_ITERATIONS;

    while (((uart->SR & USART_SR_TXE) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }
    else
    {
        uart->DR = byte;
    }

    return status;
}

DriverStatus_e uartTransmit(UartRegisters_t *uart, const uint8_t *data, size_t length)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    for (size_t i = 0U; (i < length) && (status == DRIVER_STATUS_OK); i++)
    {
        status = uartTransmitByte(uart, data[i]);
    }

    return status;
}

DriverStatus_e uartReceiveByte(const UartRegisters_t *uart, uint8_t *byte)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = UART_READY_TIMEOUT_ITERATIONS;

    while (((uart->SR & USART_SR_RXNE) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }
    else
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

DriverStatus_e uartReceive(const UartRegisters_t *uart, uint8_t *data, size_t length)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    for (size_t i = 0U; (i < length) && (status == DRIVER_STATUS_OK); i++)
    {
        status = uartReceiveByte(uart, &data[i]);
    }

    return status;
}

/** @} */
