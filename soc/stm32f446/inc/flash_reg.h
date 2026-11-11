/**
 * @file flash_reg.h
 * @brief Flash interface registers: access latency, prefetch and caches
 *        (RM0390 section 3.8).
 *
 * LATENCY must match SYSCLK and the supply voltage (RM0390 Table 5). A wrong
 * value raises no fault; flash reads are simply unreliable. Only the bits the
 * clock bring-up uses are modelled: there are no CR programming bits and only
 * EOP, WRPERR and BSY in SR, because the project does not write flash at run
 * time.
 */
#ifndef FLASH_REG_H
#define FLASH_REG_H

#include <stdint.h>

/**
 * @addtogroup flash_registers
 * @{
 */

#define FLASH_BASE (0x40023C00UL) ///< Flash interface base address (AHB1)

/** @brief Flash interface register block. */
typedef struct FlashRegisters_t
{
    volatile uint32_t ACR;     ///< 0x00: access control
    volatile uint32_t KEYR;    ///< 0x04: key
    volatile uint32_t OPTKEYR; ///< 0x08: option key
    volatile uint32_t SR;      ///< 0x0C: status
    volatile uint32_t CR;      ///< 0x10: control
    volatile uint32_t OPTCR;   ///< 0x14: option control
    volatile uint32_t OPTCR1;  ///< 0x18: option control 1 (offset from ST's CMSIS header)
} FlashRegisters_t;

#define FLASH ((FlashRegisters_t *)FLASH_BASE) ///< The Flash interface instance

/** @name ACR - access control (0x00)
 *  @{ */
#define FLASH_ACR_LATENCY_Pos (0U) ///< Bits 3:0
/** \regaccess{rw} Wait states (0-15). Read back to confirm a change took effect. */
#define FLASH_ACR_LATENCY_Msk ((uint32_t)0xFU << FLASH_ACR_LATENCY_Pos)
#define FLASH_ACR_PRFTEN_Pos (8U) ///< Bit 8
/** \regaccess{rw} Enable prefetch. */
#define FLASH_ACR_PRFTEN ((uint32_t)1U << FLASH_ACR_PRFTEN_Pos)
#define FLASH_ACR_ICEN_Pos (9U) ///< Bit 9
/** \regaccess{rw} Enable the instruction cache. */
#define FLASH_ACR_ICEN ((uint32_t)1U << FLASH_ACR_ICEN_Pos)
#define FLASH_ACR_DCEN_Pos (10U) ///< Bit 10
/** \regaccess{rw} Enable the data cache. */
#define FLASH_ACR_DCEN ((uint32_t)1U << FLASH_ACR_DCEN_Pos)
#define FLASH_ACR_ICRST_Pos (11U) ///< Bit 11
/** \regaccess{rw} Reset the instruction cache; only while it is disabled. */
#define FLASH_ACR_ICRST ((uint32_t)1U << FLASH_ACR_ICRST_Pos)
#define FLASH_ACR_DCRST_Pos (12U) ///< Bit 12
/** \regaccess{rw} Reset the data cache; only while it is disabled. */
#define FLASH_ACR_DCRST ((uint32_t)1U << FLASH_ACR_DCRST_Pos)
/** @} */

/** @name SR - status (0x0C)
 *  @{ */
#define FLASH_SR_EOP_Pos (0U) ///< Bit 0
/** \regaccess{rc_w1} End of operation. */
#define FLASH_SR_EOP ((uint32_t)1U << FLASH_SR_EOP_Pos)
#define FLASH_SR_WRPERR_Pos (4U) ///< Bit 4
/** \regaccess{rc_w1} Write-protection error. */
#define FLASH_SR_WRPERR ((uint32_t)1U << FLASH_SR_WRPERR_Pos)
#define FLASH_SR_BSY_Pos (16U) ///< Bit 16
/** \regaccess{ro} A flash operation is in progress. */
#define FLASH_SR_BSY ((uint32_t)1U << FLASH_SR_BSY_Pos)
/** @} */

/** @} */

#endif /* FLASH_REG_H */
