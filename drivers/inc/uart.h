/**
 * @file uart.h
 * @brief USART/UART driver - asynchronous configuration (8 data bits,
 *        optional parity, 1 or 2 stop bits) and blocking transmit/receive
 *        for the STM32F446xx (device/inc/uart_reg.h).
 *        No application-facing logic; consumed by bsp/ and, once it exists, api/.
 *
 * 8 data bits only, matching uart_reg.h's own documented scope -
 * CR1.M is not a configurable parameter here. uartInit() derives it from
 * the parity setting: RM0390 counts the parity bit inside the frame length
 * M selects, so parity off leaves M clear (8 bits on the wire), and parity
 * on sets M (9 bits on the wire: 8 data + parity). The extra bit is parity,
 * generated and checked by hardware, so DR transfers stay uint8_t.
 * Genuine 9-data-bit mode is architecturally real (RM0390) but was never
 * actually supported by this driver: an earlier revision exposed CR1.M as a
 * parameter without widening transmit/receive past uint8_t, so a caller
 * requesting 9-bit mode would have silently truncated every byte through
 * DR instead of erroring. Add real 9-bit support (uint16_t transmit/
 * receive) if a future consumer actually needs it; don't re-expose the
 * parameter without it.
 *
 * CR2.STOP similarly offers only 1 or 2 stop bits, not the full 4-value
 * field RM0390 documents. The other two encodings, 0.5 and 1.5 stop
 * bits, exist for smartcard mode - uart_reg.h's own file-level comment
 * already scopes CR3's smartcard bits (SCEN/IREN/IRLP) as omitted, since
 * this driver never enables that mode - and RM0390 additionally
 * documents both as unavailable specifically on UART4/UART5. Accepting
 * either here would report DRIVER_STATUS_OK while writing a CR2.STOP
 * encoding this driver has no way to honor: nothing in this driver ever
 * enables the smartcard mode the encoding is for, on any instance. Same
 * class of bug as the 9-bit-mode parameter above - removed rather than
 * shipped accepting a value the rest of the driver cannot act on.
 *
 * Every transmit/receive function here blocks by spin-polling TXE/RXNE
 * with a bounded, caller-supplied poll count - the only thing implementable before
 * an RTOS scheduler exists to block/wake a task against an interrupt
 * (drivers/inc/nvic.h now exists to configure the interrupt itself). A task that calls
 * uartTransmit()/uartReceive() once real tasks exist will monopolize the
 * CPU for the duration of the transfer, defeating preemption for
 * whatever else wanted to run - this is a known, deliberate limitation
 * of this revision, not the intended final form. Revisit with an
 * interrupt- or DMA-driven variant once rtos/kernel/ exists to build
 * one against.
 *
 * Timeouts are supplied by the caller, as a count of SR polls (@c timeout
 * on each function), not baked into the driver. A fixed count cannot be
 * right: one SR poll costs roughly 10-30 core cycles depending on the
 * build (-O0 is about 2-3x slower than -O2) and the core clock (16 MHz
 * HSI to 180 MHz), so any constant is a different number of milliseconds
 * in every configuration, and how long a wait should last is a property
 * of the caller - a transmit is bounded by the frame time at the
 * configured baud rate (10-12 bit times per byte, two frames for a wait
 * on TC), while a receive waits for whenever the sender next speaks,
 * which is not related to the baud rate at all. Size a timeout as
 * milliseconds x core_hz / 1000 / 30 for a safe lower bound on polls (30
 * cycles per poll is the slow end), or pass UINT32_MAX for an effectively
 * unbounded wait (about 2^32 polls: tens of seconds to minutes). A
 * @c timeout of 0 polls exactly once, so it turns a call into a
 * non-blocking check that reports DRIVER_STATUS_ERR_TIMEOUT when the flag
 * is not already set. These are bounded retry counts, not wall-clock
 * timeouts, for the reason in rcc.h's file-level comment; revisit with a
 * SysTick deadline once the kernel provides one.
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
 * @brief Configure a UART/USART peripheral for asynchronous 8-data-bit
 *        operation (optionally with parity) and enable it.
 *
 * Clears CR1.UE first, then sets BRR (from @p pclk_hz and @p baud,
 * standard 16x oversampling - OVER8 is not exposed by this driver and is
 * cleared), CR2.STOP, and CR1's PCE/PS/TE/RE fields (CR1.M is set exactly
 * when parity is enabled, giving 8 data bits plus parity - see this file's
 * file-level comment; 9 data bits is not offered), then sets CR1.UE last
 * so the peripheral only turns on once every other field is already
 * correct.
 *
 * Disabling the peripheral up front matters on a *re*-configuration, not
 * a first call: RM0390 requires BRR and the framing fields be programmed
 * with the USART disabled, so calling this on an already-running
 * instance (e.g. to change baud rate) would otherwise mutate those
 * fields underneath an in-flight frame. On a first call after reset UE
 * is already clear and the write is a no-op.
 *
 * @param uart          UART register block (e.g. USART2).
 * @param pclk_hz       The peripheral's bus clock frequency in Hz (APB1
 *                      for USART2/3/UART4/5, APB2 for USART1/6 - see
 *                      device/inc/uart_reg.h's base-address comments).
 * @param baud          Target baud rate in bit/s (e.g. 115200).
 * @param stop_bits     CR2.STOP field value: ::USART_CR2_STOP_1 or
 *                      ::USART_CR2_STOP_2. ::USART_CR2_STOP_0_5 and
 *                      ::USART_CR2_STOP_1_5 are not accepted - see this
 *                      file's top comment for why.
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
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if stop_bits, parity_enable,
 *         parity_select, or direction is not one of its documented
 *         values, if @p baud or @p pclk_hz is `0U`, or if the computed
 *         baud-rate divisor doesn't fit BRR: @p baud too low for
 *         @p pclk_hz (divisor overflows the 16-bit field), or above
 *         @p pclk_hz / 16 (a divisor below 1, which 16x oversampling
 *         cannot generate).
 */
DRIVER_MUST_CHECK DriverStatus_e uartInit(UartRegisters_t *uart, uint32_t pclk_hz, uint32_t baud,
                                          uint32_t stop_bits, uint32_t parity_enable,
                                          uint32_t parity_select, uint32_t direction);

/**
 * @brief Reset a UART/USART peripheral's CR1/CR2/CR3/BRR/GTPR back to
 *        their power-on-reset values (disabled, 8N1, no baud rate
 *        configured).
 *
 * @param uart UART register block (e.g. USART2).
 * @pre Any transmission in progress has already completed - call
 *      uartFlush() first if the last thing this instance did was
 *      transmit. Disabling the peripheral mid-frame truncates whatever
 *      is still in the shift register, and this function cannot wait on
 *      the caller's behalf: it has no failure path to report a wait that
 *      never completed.
 */
void uartDeinit(UartRegisters_t *uart);

/**
 * @brief Block until the transmitter has finished shifting out the last
 *        byte written (SR.TC).
 *
 * SR.TXE, which uartTransmitByte() waits on, only reports that DR is
 * free to accept the *next* byte - the previous one is still being
 * shifted onto the wire at that point. SR.TC is what reports the line
 * actually idle. Anything that disturbs the peripheral or its clock
 * immediately after a transmit - uartDeinit(), gating the instance's
 * clock off, reconfiguring the baud rate, entering a low-power mode -
 * truncates the final character unless this ran first.
 *
 * uartTransmit() already calls this itself once its buffer is written,
 * so a caller that only ever uses the buffer API does not need to.
 * Callers driving uartTransmitByte() directly do.
 *
 * @param uart UART register block (e.g. USART2). Taken as a pointer to
 *             const: this function only reads SR, and reading SR alone
 *             does not clear TC (RM0390 clears it via an SR read
 *             followed by a DR write, and this function never writes
 *             DR).
 * @param timeout Maximum number of SR polls to wait for TC - see this
 *             file's comment on timeouts.
 * @pre uartInit() has already enabled this instance with
 *      ::USART_CR1_TE set in its @c direction argument.
 * @return DRIVER_STATUS_OK once SR.TC is observed set.
 * @return DRIVER_STATUS_ERR_TIMEOUT if SR.TC never set within
 *         @p timeout polls.
 */
DRIVER_MUST_CHECK DriverStatus_e uartFlush(const UartRegisters_t *uart, uint32_t timeout);

/**
 * @brief Send one byte, blocking until the transmit data register is
 *        empty and ready for it.
 *
 * @param uart UART register block (e.g. USART2).
 * @param byte Byte to send.
 * @param timeout Maximum number of SR polls to wait for TXE - see this
 *             file's comment on timeouts. A full frame time at the
 *             configured baud rate is the longest legitimate wait.
 * @pre uartInit() has already enabled this instance with
 *      ::USART_CR1_TE set in its @c direction argument.
 * @return DRIVER_STATUS_OK once DR has been written.
 * @return DRIVER_STATUS_ERR_TIMEOUT if SR.TXE never set within
 *         @p timeout polls.
 */
DRIVER_MUST_CHECK DriverStatus_e uartTransmitByte(UartRegisters_t *uart, uint8_t byte,
                                                  uint32_t timeout);

/**
 * @brief Send a buffer of bytes, blocking until the last one has been
 *        shifted out onto the wire.
 *
 * Finishes with uartFlush(), so a successful return means the whole
 * buffer is on the line and the transmitter is idle - not merely that
 * the last byte reached DR. That is what makes it safe to follow this
 * call directly with uartDeinit(), a clock gate, or a reconfiguration.
 *
 * @param uart   UART register block (e.g. USART2).
 * @param data   Bytes to send. May be NULL only when @p length is `0U`.
 * @param length Number of bytes in @p data. A @p length of `0U` sends
 *               nothing and returns DRIVER_STATUS_OK without waiting on
 *               TC, since no transmission was started.
 * @param timeout Maximum number of SR polls to wait *per byte* for
 *               TXE, and again for the closing TC wait - see this
 *               file's comment on timeouts.
 * @return DRIVER_STATUS_OK once every byte has been sent and SR.TC
 *         confirms the transmitter is idle.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p data is NULL while
 *         @p length is non-zero. Nothing is sent.
 * @return DRIVER_STATUS_ERR_TIMEOUT propagated from uartTransmitByte()
 *         the moment any byte times out, or from the closing uartFlush()
 *         if SR.TC never set; bytes already sent are not undone, and no
 *         further bytes are sent after the failure.
 */
DRIVER_MUST_CHECK DriverStatus_e uartTransmit(UartRegisters_t *uart, const uint8_t *data,
                                              size_t length, uint32_t timeout);

/**
 * @brief Receive one byte, blocking until one is available.
 *
 * Always reads DR once SR.RXNE sets, whether or not a hardware fault is
 * also reported - RM0390 clears SR.ORE/NE/FE/PE via an SR read (already
 * done by this function to check them) followed by a DR read, so
 * skipping the DR read on a fault would leave the flag set and corrupt
 * the next call's fault check.
 *
 * @param uart UART register block (e.g. USART2). Deliberately *not* a
 *             pointer to const, unlike gpioReadPin() or uartFlush():
 *             reading DR is not a side-effect-free observation of this
 *             peripheral, it clears SR.RXNE and the ORE/NE/FE/PE flags.
 *             Marking it const would advertise a purely-observational
 *             function and invite a caller or a later refactor to treat
 *             the receive path as repeatable or reorderable, which it is
 *             not.
 * @param byte Set to the received byte - written even when this function
 *             returns DRIVER_STATUS_ERR_HW_FAULT, since DR is always
 *             read; treat the byte as unreliable in that case. Must not
 *             be NULL.
 * @param timeout Maximum number of SR polls to wait for RXNE - see this
 *             file's comment on timeouts. An RXNE wait is bounded by
 *             when the sender next transmits, not by the baud rate, so
 *             this is usually the one to make large.
 * @pre uartInit() has already enabled this instance with
 *      ::USART_CR1_RE set in its @c direction argument.
 * @return DRIVER_STATUS_OK once a byte with no reported fault has been
 *         read into @p byte.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p byte is NULL. DR is not
 *         read, so a pending byte stays pending.
 * @return DRIVER_STATUS_ERR_HW_FAULT if SR reports ORE, NE, FE, or PE
 *         for this byte.
 * @return DRIVER_STATUS_ERR_TIMEOUT if SR.RXNE never set within
 *         @p timeout polls.
 */
DRIVER_MUST_CHECK DriverStatus_e uartReceiveByte(UartRegisters_t *uart, uint8_t *byte,
                                                 uint32_t timeout);

/**
 * @brief Receive a buffer of bytes, blocking on each one in turn.
 *
 * @param uart   UART register block (e.g. USART2).
 * @param data   Buffer to receive into. May be NULL only when
 *               @p length is `0U`.
 * @param length Number of bytes to receive into @p data.
 * @param timeout Maximum number of SR polls to wait *per byte* for RXNE
 *               - see this file's comment on timeouts.
 * @return DRIVER_STATUS_OK once every byte has been received with no
 *         fault.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p data is NULL while
 *         @p length is non-zero. Nothing is received.
 * @return DRIVER_STATUS_ERR_HW_FAULT or DRIVER_STATUS_ERR_TIMEOUT
 *         propagated from uartReceiveByte() the moment any byte fails;
 *         bytes already received remain in @p data, and no further
 *         bytes are received after the failure.
 */
DRIVER_MUST_CHECK DriverStatus_e uartReceive(UartRegisters_t *uart, uint8_t *data, size_t length,
                                             uint32_t timeout);

/** @} */

#endif /* UART_H */
