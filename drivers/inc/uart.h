/**
 * @file uart.h
 * @brief UART driver: asynchronous configuration and blocking transfer.
 *
 * 115200 baud, 8N1 on USART2 (the NUCLEO's ST-LINK virtual COM port, PA2/PA3,
 * AF7), with APB1 at 45 MHz:
 * @code
 * rccUsart2ClockEnable(RCC);
 * // PA2/PA3: gpioSetAlternateFunction(..., 7U), then gpioInit(..., GPIO_MODE_AF, ...)
 * if (uartInit(USART2, BSP_PCLK1_HZ, 115200U, USART_CR2_STOP_1, 0U, 0U,
 *              USART_CR1_TE | USART_CR1_RE) != DRIVER_STATUS_OK) { ... }
 * static const uint8_t msg[] = "hello\r\n";
 * if (uartTransmit(USART2, msg, sizeof msg - 1U, 100000U) != DRIVER_STATUS_OK) { ... }
 * @endcode
 *
 * - **Framing:** 8 data bits, optional parity, 1 or 2 stop bits. With parity
 *   on, uartInit() sets CR1.M, because RM0390 counts parity inside the M
 *   frame. Nine-bit data and the smartcard-only 0.5/1.5 stop bits are not
 *   offered, since transfers move `uint8_t`.
 * - **Timeouts:** every wait is a caller-supplied count of SR polls, not a
 *   time. One poll costs roughly 10-30 core cycles, so allow at least
 *   `ms * core_hz / 1000 / 30`. 0 polls once (a non-blocking check), and
 *   `UINT32_MAX` waits for practically ever. See
 *   [Poll-count timeouts](DESIGN.md#poll-count-timeouts).
 * - **Blocking:** transfers busy-wait. Once tasks exist they monopolise the
 *   CPU for the transfer; an interrupt- or DMA-driven variant belongs with the
 *   kernel.
 */
#ifndef UART_H
#define UART_H

#include "driver_status.h"
#include "uart_reg.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @addtogroup uart_driver
 * @{
 */

/**
 * @brief Configure an instance for asynchronous 8-bit operation and enable it.
 *
 * Clears UE, writes BRR (16x oversampling), CR2.STOP and CR1's M, OVER8,
 * PCE, PS, TE and RE, then sets UE last. Other CR1 bits and all of CR3 are
 * kept, so call uartDeinit() first when reusing an instance that had
 * interrupts or DMA enabled.
 *
 * @param uart          Instance (e.g. USART2).
 * @param pclk_hz       Its bus clock: APB1 for USART2/3 and UART4/5, APB2 for USART1/6.
 * @param baud          Baud rate, in bit/s.
 * @param stop_bits     ::USART_CR2_STOP_1 or ::USART_CR2_STOP_2.
 * @param parity_enable 0 for no parity, or ::USART_CR1_PCE.
 * @param parity_select 0 for even, or ::USART_CR1_PS for odd; validated even without parity.
 * @param direction     ::USART_CR1_TE and/or ::USART_CR1_RE, or 0.
 * @pre The instance's clock is on, and its pins are set to the right alternate function.
 * @retval DRIVER_STATUS_OK                Configured and enabled.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM A parameter is not one of its values, @p pclk_hz or
 *                                         @p baud is 0, or the divisor does not fit BRR
 *                                         (@p baud is above `pclk_hz / 16` or too low).
 */
DRIVER_MUST_CHECK DriverStatus_e uartInit(UartRegisters_t *uart, uint32_t pclk_hz, uint32_t baud,
                                          uint32_t stop_bits, uint32_t parity_enable,
                                          uint32_t parity_select, uint32_t direction);

/**
 * @brief Return CR1, CR2, CR3, BRR and GTPR to their reset values (disabled).
 * @param uart Instance (e.g. USART2).
 * @pre Any transmission has finished (uartFlush()); disabling mid-frame cuts it short.
 */
void uartDeinit(UartRegisters_t *uart);

/**
 * @brief Wait until the last byte has left the shift register (SR.TC).
 *
 * TXE only means DR can take another byte. Call this before deinitialising,
 * gating the clock or changing the baud rate after uartTransmitByte();
 * uartTransmit() already does.
 *
 * @param uart    Instance (e.g. USART2); only SR is read, which does not clear TC.
 * @param timeout SR polls to allow.
 * @pre The transmitter is enabled.
 * @retval DRIVER_STATUS_OK          TC is set.
 * @retval DRIVER_STATUS_ERR_TIMEOUT TC did not set within @p timeout polls.
 */
DRIVER_MUST_CHECK DriverStatus_e uartFlush(const UartRegisters_t *uart, uint32_t timeout);

/**
 * @brief Send one byte once DR is free (SR.TXE).
 * @param uart    Instance (e.g. USART2).
 * @param byte    Byte to send.
 * @param timeout SR polls to allow; one frame time is the longest legitimate wait.
 * @pre The transmitter is enabled.
 * @retval DRIVER_STATUS_OK          The byte is in DR.
 * @retval DRIVER_STATUS_ERR_TIMEOUT TXE did not set within @p timeout polls.
 */
DRIVER_MUST_CHECK DriverStatus_e uartTransmitByte(UartRegisters_t *uart, uint8_t byte,
                                                  uint32_t timeout);

/**
 * @brief Send a buffer and wait until the line is idle.
 *
 * Ends with uartFlush(), so the instance can be reconfigured or
 * deinitialised straight after.
 *
 * @param uart    Instance (e.g. USART2).
 * @param data    Bytes to send; may be NULL only if @p length is 0.
 * @param length  Number of bytes. 0 sends nothing and returns OK without waiting.
 * @param timeout SR polls to allow per byte, and again for the final TC wait.
 * @retval DRIVER_STATUS_OK                Sent, and the transmitter is idle.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p data is NULL with a non-zero @p length.
 * @retval DRIVER_STATUS_ERR_TIMEOUT       A byte or the final TC wait timed out; sending stops
 *                                         there.
 */
DRIVER_MUST_CHECK DriverStatus_e uartTransmit(UartRegisters_t *uart, const uint8_t *data,
                                              size_t length, uint32_t timeout);

/**
 * @brief Receive one byte once SR.RXNE sets.
 *
 * DR is always read once RXNE sets, even on an error, because the SR-then-DR
 * read is what clears ORE/NE/FE/PE. That side effect is why @p uart is not
 * `const` (see [const register parameters](DESIGN.md#const-register-parameters)).
 *
 * @param uart    Instance (e.g. USART2).
 * @param byte    Receives the byte, also on HW_FAULT, when it is unreliable. Not NULL.
 * @param timeout SR polls to allow; this waits on the sender, so it is usually large.
 * @pre The receiver is enabled.
 * @retval DRIVER_STATUS_OK                A byte with no error was received.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p byte is NULL; DR is not read.
 * @retval DRIVER_STATUS_ERR_HW_FAULT      SR reported ORE, NE, FE or PE for this byte.
 * @retval DRIVER_STATUS_ERR_TIMEOUT       RXNE did not set within @p timeout polls.
 * @warning Single owner only: two contexts reading the same instance race on SR and DR.
 */
DRIVER_MUST_CHECK DriverStatus_e uartReceiveByte(UartRegisters_t *uart, uint8_t *byte,
                                                 uint32_t timeout);

/**
 * @brief Receive a buffer, one byte at a time.
 * @param uart    Instance (e.g. USART2).
 * @param data    Destination; may be NULL only if @p length is 0.
 * @param length  Number of bytes.
 * @param timeout SR polls to allow per byte.
 * @retval DRIVER_STATUS_OK                Every byte received with no error.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p data is NULL with a non-zero @p length.
 * @retval DRIVER_STATUS_ERR_HW_FAULT      A byte had an error; receiving stops there.
 * @retval DRIVER_STATUS_ERR_TIMEOUT       A byte timed out; receiving stops there.
 */
DRIVER_MUST_CHECK DriverStatus_e uartReceive(UartRegisters_t *uart, uint8_t *data, size_t length,
                                             uint32_t timeout);

/** @} */

#endif /* UART_H */
