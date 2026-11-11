/**
 * @file uart_reg.h
 * @brief USART/UART registers (RM0390 section 25.6).
 *
 * All six instances share one layout; UART4/5 leave the synchronous and
 * smartcard bits unimplemented. Modelled: what an asynchronous driver needs
 * (baud rate, enables, framing, status flags and their interrupt enables).
 * Not modelled: synchronous clock output, smartcard/IrDA and hardware flow
 * control. The NUCLEO's USART2 link to the ST-LINK has no CTS/RTS.
 */
#ifndef UART_REG_H
#define UART_REG_H

#include <stdint.h>

/**
 * @addtogroup uart_registers
 * @{
 */

/** @name Base addresses
 *  @{ */
#define USART1_BASE (0x40011000UL) ///< USART1 (APB2)
#define USART2_BASE (0x40004400UL) ///< USART2 (APB1), the ST-LINK virtual COM port
#define USART3_BASE (0x40004800UL) ///< USART3 (APB1)
#define UART4_BASE (0x40004C00UL)  ///< UART4 (APB1)
#define UART5_BASE (0x40005000UL)  ///< UART5 (APB1)
#define USART6_BASE (0x40011400UL) ///< USART6 (APB2)
/** @} */

/** @brief USART/UART register block. */
typedef struct UartRegisters_t
{
    volatile uint32_t SR;   ///< 0x00: status
    volatile uint32_t DR;   ///< 0x04: data
    volatile uint32_t BRR;  ///< 0x08: baud rate
    volatile uint32_t CR1;  ///< 0x0C: control 1
    volatile uint32_t CR2;  ///< 0x10: control 2
    volatile uint32_t CR3;  ///< 0x14: control 3
    volatile uint32_t GTPR; ///< 0x18: guard time and prescaler (USART only)
} UartRegisters_t;

/** @name Instances
 *  @{ */
#define USART1 ((UartRegisters_t *)USART1_BASE) ///< USART1
#define USART2 ((UartRegisters_t *)USART2_BASE) ///< USART2
#define USART3 ((UartRegisters_t *)USART3_BASE) ///< USART3
#define UART4 ((UartRegisters_t *)UART4_BASE)   ///< UART4
#define UART5 ((UartRegisters_t *)UART5_BASE)   ///< UART5
#define USART6 ((UartRegisters_t *)USART6_BASE) ///< USART6
/** @} */

/** @name SR - status (0x00)
 *  PE, FE, NE and ORE clear by reading SR then DR.
 *  @{ */
#define USART_SR_PE_Pos (0U) ///< Bit 0
/** \regaccess{ro} Parity error. */
#define USART_SR_PE ((uint32_t)1U << USART_SR_PE_Pos)
#define USART_SR_FE_Pos (1U) ///< Bit 1
/** \regaccess{ro} Framing error. */
#define USART_SR_FE ((uint32_t)1U << USART_SR_FE_Pos)
#define USART_SR_NE_Pos (2U) ///< Bit 2
/** \regaccess{ro} Noise detected (RM0390 names it NF). */
#define USART_SR_NE ((uint32_t)1U << USART_SR_NE_Pos)
#define USART_SR_ORE_Pos (3U) ///< Bit 3
/** \regaccess{ro} Overrun error. */
#define USART_SR_ORE ((uint32_t)1U << USART_SR_ORE_Pos)
#define USART_SR_IDLE_Pos (4U) ///< Bit 4
/** \regaccess{ro} Idle line detected. */
#define USART_SR_IDLE ((uint32_t)1U << USART_SR_IDLE_Pos)
#define USART_SR_RXNE_Pos (5U) ///< Bit 5
/** \regaccess{rc_w0} Receive data ready; a DR read also clears it. */
#define USART_SR_RXNE ((uint32_t)1U << USART_SR_RXNE_Pos)
#define USART_SR_TC_Pos (6U) ///< Bit 6
/** \regaccess{rc_w0} Transmission complete: the shift register is empty. */
#define USART_SR_TC ((uint32_t)1U << USART_SR_TC_Pos)
#define USART_SR_TXE_Pos (7U) ///< Bit 7
/** \regaccess{ro} DR can take the next byte; a DR write clears it. */
#define USART_SR_TXE ((uint32_t)1U << USART_SR_TXE_Pos)
/** @} */

/** @name BRR - baud rate (0x08)
 *  With 16x oversampling, BRR = round(pclk / baud).
 *  @{ */
#define USART_BRR_DIV_FRACTION_Pos (0U) ///< Bits 3:0
/** \regaccess{rw} USARTDIV fraction, in sixteenths. */
#define USART_BRR_DIV_FRACTION_Msk ((uint32_t)0xFU << USART_BRR_DIV_FRACTION_Pos)
#define USART_BRR_DIV_MANTISSA_Pos (4U) ///< Bits 15:4
/** \regaccess{rw} USARTDIV mantissa. */
#define USART_BRR_DIV_MANTISSA_Msk ((uint32_t)0xFFFU << USART_BRR_DIV_MANTISSA_Pos)
/** @} */

/** @name CR1 - control 1 (0x0C)
 *  Every bit is \regaccess{rw}.
 *  @{ */
#define USART_CR1_SBK_Pos (0U)                                  ///< Bit 0
#define USART_CR1_SBK ((uint32_t)1U << USART_CR1_SBK_Pos)       ///< Send a break
#define USART_CR1_RWU_Pos (1U)                                  ///< Bit 1
#define USART_CR1_RWU ((uint32_t)1U << USART_CR1_RWU_Pos)       ///< Receiver in mute mode
#define USART_CR1_RE_Pos (2U)                                   ///< Bit 2
#define USART_CR1_RE ((uint32_t)1U << USART_CR1_RE_Pos)         ///< Receiver enable
#define USART_CR1_TE_Pos (3U)                                   ///< Bit 3
#define USART_CR1_TE ((uint32_t)1U << USART_CR1_TE_Pos)         ///< Transmitter enable
#define USART_CR1_IDLEIE_Pos (4U)                               ///< Bit 4
#define USART_CR1_IDLEIE ((uint32_t)1U << USART_CR1_IDLEIE_Pos) ///< IDLE interrupt enable
#define USART_CR1_RXNEIE_Pos (5U)                               ///< Bit 5
#define USART_CR1_RXNEIE ((uint32_t)1U << USART_CR1_RXNEIE_Pos) ///< RXNE/ORE interrupt enable
#define USART_CR1_TCIE_Pos (6U)                                 ///< Bit 6
#define USART_CR1_TCIE ((uint32_t)1U << USART_CR1_TCIE_Pos)     ///< TC interrupt enable
#define USART_CR1_TXEIE_Pos (7U)                                ///< Bit 7
#define USART_CR1_TXEIE ((uint32_t)1U << USART_CR1_TXEIE_Pos)   ///< TXE interrupt enable
#define USART_CR1_PEIE_Pos (8U)                                 ///< Bit 8
#define USART_CR1_PEIE ((uint32_t)1U << USART_CR1_PEIE_Pos)     ///< PE interrupt enable
#define USART_CR1_PS_Pos (9U)                                   ///< Bit 9
#define USART_CR1_PS ((uint32_t)1U << USART_CR1_PS_Pos)         ///< Parity: 0 even, 1 odd
#define USART_CR1_PCE_Pos (10U)                                 ///< Bit 10
#define USART_CR1_PCE ((uint32_t)1U << USART_CR1_PCE_Pos)       ///< Parity control enable
#define USART_CR1_WAKE_Pos (11U)                                ///< Bit 11
#define USART_CR1_WAKE ((uint32_t)1U << USART_CR1_WAKE_Pos) ///< Wakeup: 0 idle line, 1 address mark
#define USART_CR1_M_Pos (12U)                               ///< Bit 12
/** Word length: 0 = 8 bits, 1 = 9 bits on the wire (parity included). */
#define USART_CR1_M ((uint32_t)1U << USART_CR1_M_Pos)
#define USART_CR1_UE_Pos (13U)                                ///< Bit 13
#define USART_CR1_UE ((uint32_t)1U << USART_CR1_UE_Pos)       ///< USART enable
#define USART_CR1_OVER8_Pos (15U)                             ///< Bit 15
#define USART_CR1_OVER8 ((uint32_t)1U << USART_CR1_OVER8_Pos) ///< Oversampling: 0 = 16x, 1 = 8x
/** @} */

/** @name CR2 - control 2 (0x10)
 *  @{ */
#define USART_CR2_STOP_Pos (12U) ///< Bits 13:12
/** \regaccess{rw} Stop bits; values below. */
#define USART_CR2_STOP_Msk ((uint32_t)0x3U << USART_CR2_STOP_Pos)
/** @} */

/** @name CR2.STOP values
 *  0.5 and 1.5 stop bits are for smartcard mode, which the driver does not
 *  support.
 *  @{ */
#define USART_CR2_STOP_1 (0x0U)   ///< 1 stop bit
#define USART_CR2_STOP_0_5 (0x1U) ///< 0.5 stop bit (smartcard)
#define USART_CR2_STOP_2 (0x2U)   ///< 2 stop bits
#define USART_CR2_STOP_1_5 (0x3U) ///< 1.5 stop bits (smartcard)
/** @} */

/** @} */

#endif /* UART_REG_H */
