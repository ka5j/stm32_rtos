/**
 * @file uart.h
 * @brief USART/UART driver - 8N1-class asynchronous configuration and
 *        blocking transmit/receive for the STM32F446xx
 *        (device/inc/uart_reg.h). No application-facing logic; consumed
 *        by api/.
 *
 * Takes UART's register block as a parameter (UartRegisters_t *) even
 * though every instance (USART1/2/3, UART4/5, USART6) shares this same
 * layout - see CONTRIBUTING.md's error-handling contract section for why
 * every driver in this project does this. Unlike rcc.c's GPIO-port
 * functions, no instance-identity parameter is needed here: every
 * function operates generically on whichever register block is passed,
 * with no cross-referencing against a fixed set of known instances.
 */
#ifndef UART_H
#define UART_H

#include "driver_status.h"
#include "uart_reg.h"

#include <stddef.h>

/**
 * @addtogroup driver_layer
 * @{
 */

/**
 * @brief Configure a UART/USART peripheral for asynchronous operation and
 *        enable it.
 *
 * Sets BRR (from @p pclk_hz and @p baud, standard 16x oversampling -
 * OVER8 is not exposed by this driver, left clear), CR2.STOP, and CR1's
 * M/PCE/PS/TE/RE fields, then sets CR1.UE last so the peripheral only
 * turns on once every other field is already correct.
 *
 * @param uart          UART register block (e.g. USART2).
 * @param pclk_hz       The peripheral's bus clock frequency in Hz (APB1
 *                      for USART2/3/UART4/5, APB2 for USART1/6 - see
 *                      device/inc/uart_reg.h's base-address comments).
 * @param baud          Target baud rate in bit/s (e.g. 115200).
 * @param word_length   CR1.M field value: ::USART_CR1_M_8BIT or
 *                      ::USART_CR1_M_9BIT.
 * @param stop_bits     CR2.STOP field value: ::USART_CR2_STOP_1,
 *                      ::USART_CR2_STOP_0_5, ::USART_CR2_STOP_2, or
 *                      ::USART_CR2_STOP_1_5.
 * @param parity_enable CR1.PCE field value: `0U` (no parity) or
 *                      ::USART_CR1_PCE.
 * @param parity_select CR1.PS field value: `0U` (even) or ::USART_CR1_PS.
 *                      Ignored by hardware when @p parity_enable is
 *                      `0U`, but still validated here.
 * @param direction     CR1.TE/RE fields: any combination of
 *                      ::USART_CR1_TE and ::USART_CR1_RE OR'd together
 *                      (or `0U` for neither).
 * @pre The port's RCC clock-gate bit for this instance is already
 *      enabled (e.g. rcc.h's rccUsart2ClockEnable() for USART2), and any
 *      GPIO pins this instance uses are already configured for their
 *      alternate function (gpio.h's gpioSetAlternateFunction()).
 * @return DRIVER_STATUS_OK on success.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if word_length, stop_bits,
 *         parity_enable, parity_select, or direction is not one of its
 *         documented values, if @p baud or @p pclk_hz is `0U`, or if the
 *         computed baud-rate divisor doesn't fit BRR (@p baud too high
 *         or too low for @p pclk_hz).
 */
DRIVER_MUST_CHECK DriverStatus_e uartInit(UartRegisters_t *uart, uint32_t pclk_hz, uint32_t baud,
                                          uint32_t word_length, uint32_t stop_bits,
                                          uint32_t parity_enable, uint32_t parity_select,
                                          uint32_t direction);

/**
 * @brief Reset a UART/USART peripheral's CR1/CR2/CR3/BRR/GTPR back to
 *        their power-on-reset values (disabled, 8N1, no baud rate
 *        configured).
 *
 * @param uart UART register block (e.g. USART2).
 */
void uartDeinit(UartRegisters_t *uart);

/**
 * @brief Send one byte, blocking until the transmit data register is
 *        empty and ready for it.
 *
 * @param uart UART register block (e.g. USART2).
 * @param byte Byte to send.
 * @pre uartInit() has already enabled this instance with
 *      ::USART_CR1_TE set in its @c direction argument.
 * @return DRIVER_STATUS_OK once DR has been written.
 * @return DRIVER_STATUS_ERR_TIMEOUT if SR.TXE never set within
 *         UART_READY_TIMEOUT_ITERATIONS iterations of this call - a
 *         bounded retry count, not a wall-clock timeout, matching every
 *         other bounded hardware-ready wait in this project (see rcc.h's
 *         file-level comment for why).
 */
DRIVER_MUST_CHECK DriverStatus_e uartTransmitByte(UartRegisters_t *uart, uint8_t byte);

/**
 * @brief Send a buffer of bytes, blocking on each one in turn.
 *
 * @param uart   UART register block (e.g. USART2).
 * @param data   Bytes to send.
 * @param length Number of bytes in @p data.
 * @return DRIVER_STATUS_OK once every byte has been sent.
 * @return DRIVER_STATUS_ERR_TIMEOUT propagated from uartTransmitByte()
 *         the moment any byte times out; bytes already sent are not
 *         undone, and no further bytes are sent after the failure.
 */
DRIVER_MUST_CHECK DriverStatus_e uartTransmit(UartRegisters_t *uart, const uint8_t *data,
                                              size_t length);

/**
 * @brief Receive one byte, blocking until one is available.
 *
 * Always reads DR once SR.RXNE sets, whether or not a hardware fault is
 * also reported - RM0390 clears SR.ORE/NE/FE/PE via an SR read (already
 * done by this function to check them) followed by a DR read, so
 * skipping the DR read on a fault would leave the flag set and corrupt
 * the next call's fault check.
 *
 * @param uart UART register block (e.g. USART2).
 * @param byte Set to the received byte - written even when this function
 *             returns DRIVER_STATUS_ERR_HW_FAULT, since DR is always
 *             read; treat the byte as unreliable in that case.
 * @pre uartInit() has already enabled this instance with
 *      ::USART_CR1_RE set in its @c direction argument.
 * @return DRIVER_STATUS_OK once a byte with no reported fault has been
 *         read into @p byte.
 * @return DRIVER_STATUS_ERR_HW_FAULT if SR reports ORE, NE, FE, or PE
 *         for this byte.
 * @return DRIVER_STATUS_ERR_TIMEOUT if SR.RXNE never set within
 *         UART_READY_TIMEOUT_ITERATIONS iterations of this call.
 */
DRIVER_MUST_CHECK DriverStatus_e uartReceiveByte(const UartRegisters_t *uart, uint8_t *byte);

/**
 * @brief Receive a buffer of bytes, blocking on each one in turn.
 *
 * @param uart   UART register block (e.g. USART2).
 * @param data   Buffer to receive into.
 * @param length Number of bytes to receive into @p data.
 * @return DRIVER_STATUS_OK once every byte has been received with no
 *         fault.
 * @return DRIVER_STATUS_ERR_HW_FAULT or DRIVER_STATUS_ERR_TIMEOUT
 *         propagated from uartReceiveByte() the moment any byte fails;
 *         bytes already received remain in @p data, and no further
 *         bytes are received after the failure.
 */
DRIVER_MUST_CHECK DriverStatus_e uartReceive(const UartRegisters_t *uart, uint8_t *data,
                                             size_t length);

/** @} */

#endif /* UART_H */
