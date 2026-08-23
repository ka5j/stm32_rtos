/**
 * @file flash_reg.h
 * @brief Flash interface register definitions for the STM32F446xx family.
 *
 * Register layout and field values derived from RM0390 (STM32F446xx
 * reference manual), section 3.7: Embedded Flash memory interface
 * registers.
 *
 * ACR's LATENCY field matters the moment RCC's PLL raises SYSCLK past
 * the 16 MHz HSI reset default: flash access time doesn't scale with
 * clock frequency, so wait states must be set to match the new SYSCLK
 * (and VOS range from pwr_reg.h) before switching RCC_CFGR.SW to the
 * PLL - RM0390 Table 15 gives the required LATENCY value per frequency
 * range. Skipping this doesn't fault visibly; it corrupts instruction
 * fetch at the new speed.
 */
#ifndef FLASH_REG_H
#define FLASH_REG_H

#include <stdint.h>

#define FLASH_BASE (0x40023C00UL) ///< Flash interface peripheral base address

/**
 * @brief Flash interface register map (RM0390 section 3.7).
 */
typedef struct FlashRegisters_t
{
  volatile uint32_t ACR;     ///< 0x00: access control register
  volatile uint32_t KEYR;    ///< 0x04: key register
  volatile uint32_t OPTKEYR; ///< 0x08: option key register
  volatile uint32_t SR;      ///< 0x0C: status register
  volatile uint32_t CR;      ///< 0x10: control register
  volatile uint32_t OPTCR;   ///< 0x14: option control register
  volatile uint32_t OPTCR1;  ///< 0x18: option control register 1
} FlashRegisters_t;

#define FLASH ((FlashRegisters_t *)FLASH_BASE) ///< Pointer to the Flash interface register block

/* --- FLASH_ACR bit definitions --- */
#define FLASH_ACR_LATENCY_Pos (0U)                            ///< Bit position within FLASH_ACR
#define FLASH_ACR_LATENCY_Msk (0xFU << FLASH_ACR_LATENCY_Pos) ///< bits 3:0, wait states
#define FLASH_ACR_PRFTEN_Pos (8U)                             ///< Bit position within FLASH_ACR
#define FLASH_ACR_PRFTEN (1U << FLASH_ACR_PRFTEN_Pos)         ///< WRITE - enable prefetch
#define FLASH_ACR_ICEN_Pos (9U)                               ///< Bit position within FLASH_ACR
#define FLASH_ACR_ICEN (1U << FLASH_ACR_ICEN_Pos)             ///< WRITE - enable instruction cache
#define FLASH_ACR_DCEN_Pos (10U)                              ///< Bit position within FLASH_ACR
#define FLASH_ACR_DCEN (1U << FLASH_ACR_DCEN_Pos)             ///< WRITE - enable data cache
#define FLASH_ACR_ICRST_Pos (11U)                             ///< Bit position within FLASH_ACR
#define FLASH_ACR_ICRST (1U << FLASH_ACR_ICRST_Pos)           ///< WRITE - reset instruction cache
#define FLASH_ACR_DCRST_Pos (12U)                             ///< Bit position within FLASH_ACR
#define FLASH_ACR_DCRST (1U << FLASH_ACR_DCRST_Pos)           ///< WRITE - reset data cache

/* --- FLASH_SR bit definitions --- */
#define FLASH_SR_EOP_Pos (0U)                       ///< Bit position within FLASH_SR
#define FLASH_SR_EOP (1U << FLASH_SR_EOP_Pos)       ///< READ/WRITE 1 to clear - end of operation
#define FLASH_SR_WRPERR_Pos (4U)                    ///< Bit position within FLASH_SR
#define FLASH_SR_WRPERR (1U << FLASH_SR_WRPERR_Pos) ///< READ/WRITE 1 to clear - write protect error
#define FLASH_SR_BSY_Pos (16U)                      ///< Bit position within FLASH_SR
#define FLASH_SR_BSY (1U << FLASH_SR_BSY_Pos)       ///< READ only - flash busy with an operation

#endif /* FLASH_REG_H */
