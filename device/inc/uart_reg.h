/**
 * @file uart_reg.h
 * @brief USART/UART peripheral register definitions for the STM32F446xx
 *        family.
 *
 * Register layout and field values derived from RM0390 (STM32F446xx
 * reference manual), section 19: Universal synchronous asynchronous
 * receiver transmitter (USART). One register layout is shared by all
 * six instances (USART1/2/3, UART4/5, USART6); UART4/5 simply leave the
 * synchronous-mode and smartcard-only bits unimplemented in hardware.
 *
 * Field coverage here is scoped to what a standard 8N1 asynchronous
 * driver requires (baud rate, TX/RX enable, data/parity framing, the
 * core status flags, and the matching interrupt enables). Synchronous
 * clock output (CR2 CPOL/CPHA/CLKEN/LBCL), smartcard/IrDA (CR3
 * SCEN/IREN/IRLP), and hardware flow control (CR3 RTSE/CTSE) are
 * omitted, since the NUCLEO-F446RE's USART2 ST-LINK VCP path (PA2/PA3)
 * does not route CTS/RTS and none of those modes apply here.
 */
#ifndef UART_REG_H
#define UART_REG_H

#include <stdint.h>

/**
 * @addtogroup uart_registers
 * @{
 */

#define USART1_BASE (0x40011000UL) ///< USART1 peripheral base address (APB2)
#define USART2_BASE (0x40004400UL) ///< USART2 peripheral base address (APB1)
#define USART3_BASE (0x40004800UL) ///< USART3 peripheral base address (APB1)
#define UART4_BASE (0x40004C00UL)  ///< UART4 peripheral base address (APB1)
#define UART5_BASE (0x40005000UL)  ///< UART5 peripheral base address (APB1)
#define USART6_BASE (0x40011400UL) ///< USART6 peripheral base address (APB2)

/**
 * @brief USART/UART peripheral register map (RM0390 section 19.6).
 *
 * One instance per peripheral, overlaid on its base address (see the
 * USARTx_BASE/UARTx_BASE defines above and the pointer macros below).
 */
typedef struct UartRegisters_t
{
    volatile uint32_t SR;   ///< 0x00: status register
    volatile uint32_t DR;   ///< 0x04: data register
    volatile uint32_t BRR;  ///< 0x08: baud rate register
    volatile uint32_t CR1;  ///< 0x0C: control register 1
    volatile uint32_t CR2;  ///< 0x10: control register 2
    volatile uint32_t CR3;  ///< 0x14: control register 3
    volatile uint32_t GTPR; ///< 0x18: guard time and prescaler register
} UartRegisters_t;

#define USART1 ((UartRegisters_t *)USART1_BASE) ///< Pointer to the USART1 register block
#define USART2 ((UartRegisters_t *)USART2_BASE) ///< Pointer to the USART2 register block
#define USART3 ((UartRegisters_t *)USART3_BASE) ///< Pointer to the USART3 register block
#define UART4 ((UartRegisters_t *)UART4_BASE)   ///< Pointer to the UART4 register block
#define UART5 ((UartRegisters_t *)UART5_BASE)   ///< Pointer to the UART5 register block
#define USART6 ((UartRegisters_t *)USART6_BASE) ///< Pointer to the USART6 register block

/* --- USART_SR bit definitions --- */
#define USART_SR_PE_Pos (0U)                    ///< Bit position within USART_SR
#define USART_SR_PE (1U << USART_SR_PE_Pos)     ///< READ only - parity error
#define USART_SR_FE_Pos (1U)                    ///< Bit position within USART_SR
#define USART_SR_FE (1U << USART_SR_FE_Pos)     ///< READ only - framing error
#define USART_SR_NE_Pos (2U)                    ///< Bit position within USART_SR
#define USART_SR_NE (1U << USART_SR_NE_Pos)     ///< READ only - noise detected flag
#define USART_SR_ORE_Pos (3U)                   ///< Bit position within USART_SR
#define USART_SR_ORE (1U << USART_SR_ORE_Pos)   ///< READ only - overrun error
#define USART_SR_IDLE_Pos (4U)                  ///< Bit position within USART_SR
#define USART_SR_IDLE (1U << USART_SR_IDLE_Pos) ///< READ only - idle line detected
#define USART_SR_RXNE_Pos (5U)                  ///< Bit position within USART_SR
#define USART_SR_RXNE (1U << USART_SR_RXNE_Pos) ///< READ only, clears on DR read
#define USART_SR_TC_Pos (6U)                    ///< Bit position within USART_SR
#define USART_SR_TC (1U << USART_SR_TC_Pos)     ///< READ/WRITE 0 to clear - transmission complete
#define USART_SR_TXE_Pos (7U)                   ///< Bit position within USART_SR
#define USART_SR_TXE (1U << USART_SR_TXE_Pos)   ///< READ only, clears on DR write

/* --- USART_BRR bit definitions --- */
#define USART_BRR_DIV_FRACTION_Pos (0U) ///< Bit position within USART_BRR
#define USART_BRR_DIV_FRACTION_Msk (0xFU << USART_BRR_DIV_FRACTION_Pos) ///< bits 3:0
#define USART_BRR_DIV_MANTISSA_Pos (4U) ///< Bit position within USART_BRR
#define USART_BRR_DIV_MANTISSA_Msk (0xFFFU << USART_BRR_DIV_MANTISSA_Pos) ///< bits 15:4

/* --- USART_CR1 bit definitions --- */
#define USART_CR1_SBK_Pos (0U)                        ///< Bit position within USART_CR1
#define USART_CR1_SBK (1U << USART_CR1_SBK_Pos)       ///< WRITE - send break
#define USART_CR1_RWU_Pos (1U)                        ///< Bit position within USART_CR1
#define USART_CR1_RWU (1U << USART_CR1_RWU_Pos)       ///< READ/WRITE - receiver wakeup
#define USART_CR1_RE_Pos (2U)                         ///< Bit position within USART_CR1
#define USART_CR1_RE (1U << USART_CR1_RE_Pos)         ///< WRITE - receiver enable
#define USART_CR1_TE_Pos (3U)                         ///< Bit position within USART_CR1
#define USART_CR1_TE (1U << USART_CR1_TE_Pos)         ///< WRITE - transmitter enable
#define USART_CR1_IDLEIE_Pos (4U)                     ///< Bit position within USART_CR1
#define USART_CR1_IDLEIE (1U << USART_CR1_IDLEIE_Pos) ///< WRITE - IDLE interrupt enable
#define USART_CR1_RXNEIE_Pos (5U)                     ///< Bit position within USART_CR1
#define USART_CR1_RXNEIE (1U << USART_CR1_RXNEIE_Pos) ///< WRITE - RXNE interrupt enable
#define USART_CR1_TCIE_Pos (6U)                       ///< Bit position within USART_CR1
#define USART_CR1_TCIE (1U << USART_CR1_TCIE_Pos)     ///< WRITE - TC interrupt enable
#define USART_CR1_TXEIE_Pos (7U)                      ///< Bit position within USART_CR1
#define USART_CR1_TXEIE (1U << USART_CR1_TXEIE_Pos)   ///< WRITE - TXE interrupt enable
#define USART_CR1_PEIE_Pos (8U)                       ///< Bit position within USART_CR1
#define USART_CR1_PEIE (1U << USART_CR1_PEIE_Pos)     ///< WRITE - PE interrupt enable
#define USART_CR1_PS_Pos (9U)                         ///< Bit position within USART_CR1
#define USART_CR1_PS (1U << USART_CR1_PS_Pos)         ///< WRITE, 0=even 1=odd - parity selection
#define USART_CR1_PCE_Pos (10U)                       ///< Bit position within USART_CR1
#define USART_CR1_PCE (1U << USART_CR1_PCE_Pos)       ///< WRITE - parity control enable
#define USART_CR1_WAKE_Pos (11U)                      ///< Bit position within USART_CR1
#define USART_CR1_WAKE (1U << USART_CR1_WAKE_Pos)     ///< WRITE, 0=idle line 1=address mark
#define USART_CR1_M_Pos (12U)                         ///< Bit position within USART_CR1
#define USART_CR1_M (1U << USART_CR1_M_Pos)           ///< WRITE, 0=8 data bits 1=9 data bits
#define USART_CR1_UE_Pos (13U)                        ///< Bit position within USART_CR1
#define USART_CR1_UE (1U << USART_CR1_UE_Pos)         ///< WRITE - USART enable
#define USART_CR1_OVER8_Pos (15U)                     ///< Bit position within USART_CR1
#define USART_CR1_OVER8 (1U << USART_CR1_OVER8_Pos)   ///< WRITE, 0=16x oversampling 1=8x

/* --- USART_CR2 bit definitions --- */
#define USART_CR2_STOP_Pos (12U)                        ///< Bit position within USART_CR2
#define USART_CR2_STOP_Msk (0x3U << USART_CR2_STOP_Pos) ///< bits 13:12: 00=1 01=0.5 10=2 11=1.5

/** @} */

#endif /* UART_REG_H */
