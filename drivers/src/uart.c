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
 *        oversampling (uartInit() clears OVER8 to match).
 *
 * RM0390 stores USARTDIV = pclk_hz / (16 * baud) as a 12-bit mantissa
 * (BRR bits 15:4) plus a 4-bit fraction/16 (BRR bits 3:0) - which is
 * just BRR's own bit layout for the value USARTDIV * 16, since bits
 * 15:4 holding the mantissa and bits 3:0 holding sixteenths is exactly
 * the binary representation of mantissa*16 + fraction. USARTDIV * 16
 * collapses algebraically to pclk_hz / baud, so BRR is simply
 * round(pclk_hz / baud) with no separate mantissa/fraction step or
 * repacking needed - the rounded integer *is* the register value.
 * Round-to-nearest via the standard (a + b/2) / b integer-division idiom.
 * Computed entirely in a 64-bit intermediate, and every operand cast to
 * it explicitly (MISRA C:2012 Rule 10.7 - no implicit widening across a
 * mixed-width operator): pclk_hz + baud/2 does not overflow uint32_t for
 * any realistic clock, but this function has no way to bound a
 * caller-supplied pclk_hz, and the previous x100 fixed-point form needed
 * the same 64-bit guard for the same reason.
 *
 * This is strictly more accurate than an earlier version of this
 * function, which computed USARTDIV*100 in a 64-bit intermediate (an
 * integer division, truncating to 2 decimal digits of USARTDIV) and
 * then rounded that truncated value to the nearest 16th - losing
 * precision before rounding could double the worst-case error. E.g. for
 * a 42 MHz bus clock at 115200 baud, the true ratio is 364.5833 (nearest
 * integer 365), but truncating to 2 decimals first produced BRR 364,
 * one off in the wrong direction; this formula computes 365 directly.
 *
 * @param pclk_hz Bus clock frequency in Hz.
 * @param baud    Target baud rate in bit/s.
 * @param brr     Set to the computed BRR value on success; untouched on
 *                failure.
 * @return DRIVER_STATUS_OK on success.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if pclk_hz or baud is 0, if the
 *         result would exceed BRR's 16-bit field (baud too low for
 *         pclk_hz), or if it computes to 0 (baud too high for pclk_hz -
 *         RM0390 forbids a BRR of 0).
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

        if (rounded == 0U)
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

    /* Every USART_CR1/USART_CR2_STOP value below is a fixed, tested
     * compile-time bit value (device/inc/uart_reg.h), not a runtime-
     * computed shift - cppcheck's MISRA addon can't bound a shift
     * through a macro expansion like this and flags every reference to
     * it; see drivers/src/rcc.c's matching comment on rccHseEnable() for
     * the same finding on the same class of macro. */
    if (status == DRIVER_STATUS_OK)
    {
        /* USART_CR2_STOP_0_5/_1_5 are architecturally real (RM0390) but
         * not accepted here - see uart.h's file-level comment for why
         * this driver only ever offers 1 or 2 stop bits. */
        if ((stop_bits != USART_CR2_STOP_1) && (stop_bits != USART_CR2_STOP_2))
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
        /* M and OVER8 are both rewritten from scratch below, so both are
         * in cr1_mask: OVER8 because the BRR computed above assumes 16x
         * oversampling (a stale OVER8 would silently double the baud rate
         * the same divisor produces), M because it is derived from the
         * parity setting rather than taken from the caller - RM0390 counts
         * the parity bit *inside* the M-selected frame length, so 8 data bits
         * plus parity needs M set (9-bit frame), while M clear with parity
         * on is 7 data bits plus parity and would overwrite DR bit 7 with
         * the parity bit. The 9th bit is parity, not data, so transmit/
         * receive stay uint8_t - see uart.h's file-level comment. */
        // cppcheck-suppress misra-c2012-12.2
        uint32_t cr1_mask = USART_CR1_M | USART_CR1_OVER8 | USART_CR1_PCE | USART_CR1_PS
                            | USART_CR1_TE | USART_CR1_RE;
        uint32_t frame_length = 0U;

        if (parity_enable != 0U)
        {
            // cppcheck-suppress misra-c2012-12.2
            frame_length = USART_CR1_M;
        }

        /* Disable before touching BRR or the framing fields: RM0390
         * requires them programmed with the USART off, which matters
         * when this is a reconfiguration of an already-running instance
         * rather than a first call - see uartInit()'s doc comment. */
        // cppcheck-suppress misra-c2012-12.2
        uart->CR1 &= ~USART_CR1_UE;

        uart->BRR = brr;
        // cppcheck-suppress misra-c2012-12.2
        uart->CR2 = (uart->CR2 & ~USART_CR2_STOP_Msk) | (stop_bits << USART_CR2_STOP_Pos);
        uart->CR1 =
            (uart->CR1 & ~cr1_mask) | frame_length | parity_enable | parity_select | direction;

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

/**
 * @brief Spin until one of SR's ready flags reads set, or the bounded
 *        retry count is exhausted. Shared by every blocking transfer
 *        function here, which differ only in which SR bit they watch.
 *
 * The outcome is decided by re-reading the flag after the loop, not by
 * testing whether the counter reached 0: the flag setting on the very
 * last iteration exits the loop with the counter already at 0, and a
 * counter test would then report a timeout for a wait that in fact
 * succeeded.
 *
 * @param uart UART register block (e.g. USART2).
 * @param flag SR flag to wait on - ::USART_SR_TXE, ::USART_SR_TC, or
 *             ::USART_SR_RXNE.
 * @return DRIVER_STATUS_OK if flag is set on exit.
 * @return DRIVER_STATUS_ERR_TIMEOUT if it is still clear after
 *         UART_READY_TIMEOUT_ITERATIONS iterations.
 */
static DriverStatus_e uartWaitSrFlag(const UartRegisters_t *uart, uint32_t flag)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = UART_READY_TIMEOUT_ITERATIONS;

    while (((uart->SR & flag) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if ((uart->SR & flag) == 0U)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }

    return status;
}

DriverStatus_e uartFlush(const UartRegisters_t *uart)
{
    // cppcheck-suppress misra-c2012-12.2
    return uartWaitSrFlag(uart, USART_SR_TC);
}

DriverStatus_e uartTransmitByte(UartRegisters_t *uart, uint8_t byte)
{
    DriverStatus_e status = uartWaitSrFlag(uart, USART_SR_TXE);

    if (status == DRIVER_STATUS_OK)
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

    /* Wait for the last byte to leave the shift register, not merely to
     * reach DR - see uartTransmit()'s doc comment. Skipped on a zero
     * length, where nothing was started and TC reflects whatever the
     * previous transfer left behind. */
    if ((status == DRIVER_STATUS_OK) && (length > 0U))
    {
        status = uartFlush(uart);
    }

    return status;
}

/* cppcheck-suppress constParameterPointer
 *
 * cppcheck is right about the C and wrong about the hardware, and this
 * is the one place in the project where those diverge. Nothing is
 * written through `uart` below - only read - so by the language's rules
 * the parameter could indeed be `const UartRegisters_t *`. But the read
 * it performs is of DR, and reading DR clears SR.RXNE together with the
 * ORE/NE/FE/PE error flags: the peripheral is left in a different state
 * than it was found in. C's type system has no way to say "reads, but
 * with a side effect", so `const` here would advertise a repeatable,
 * reorderable observation and mislead every future reader and refactor.
 * See CONTRIBUTING.md's `const` on a register-block parameter section;
 * this function is the worked example it cites. */
DriverStatus_e uartReceiveByte(UartRegisters_t *uart, uint8_t *byte)
{
    DriverStatus_e status = uartWaitSrFlag(uart, USART_SR_RXNE);

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

DriverStatus_e uartReceive(UartRegisters_t *uart, uint8_t *data, size_t length)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    for (size_t i = 0U; (i < length) && (status == DRIVER_STATUS_OK); i++)
    {
        status = uartReceiveByte(uart, &data[i]);
    }

    return status;
}

/** @} */
