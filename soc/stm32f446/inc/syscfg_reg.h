/**
 * @file syscfg_reg.h
 * @brief SYSCFG registers: EXTI line routing and the I/O compensation cell
 *        (RM0390 section 8.2).
 *
 * Accessible only once RCC_APB2ENR_SYSCFGEN is set. MEMRMP and PMC are laid
 * out but not given bit macros; SYSCFG_CFGR (0x2C) is not modelled.
 */
#ifndef SYSCFG_REG_H
#define SYSCFG_REG_H

#include <stdint.h>

/**
 * @addtogroup syscfg_registers
 * @{
 */

#define SYSCFG_BASE (0x40013800UL) ///< SYSCFG base address (APB2)

/** @brief SYSCFG register block. */
typedef struct SyscfgRegisters_t
{
    volatile uint32_t MEMRMP;       ///< 0x00: memory remap
    volatile uint32_t PMC;          ///< 0x04: peripheral mode configuration
    volatile uint32_t EXTICR[4];    ///< 0x08-0x14: EXTI line routing, 4 lines per register
    volatile uint32_t RESERVED0[2]; ///< 0x18-0x1C: gap in the memory map
    volatile uint32_t CMPCR;        ///< 0x20: compensation cell control
} SyscfgRegisters_t;

#define SYSCFG ((SyscfgRegisters_t *)SYSCFG_BASE) ///< The SYSCFG instance

/** @name EXTICR port selectors
 *  EXTI line `n` is selected by the 4-bit \regaccess{rw} field at bit
 *  `(n % 4) * 4` of `EXTICR[n / 4]`. GPIOH has only pins 0 and 1, so PH is
 *  valid only for lines 0 and 1.
 *  @{ */
#define SYSCFG_EXTICR_PA (0x0U) ///< GPIOA
#define SYSCFG_EXTICR_PB (0x1U) ///< GPIOB
#define SYSCFG_EXTICR_PC (0x2U) ///< GPIOC
#define SYSCFG_EXTICR_PD (0x3U) ///< GPIOD
#define SYSCFG_EXTICR_PE (0x4U) ///< GPIOE
#define SYSCFG_EXTICR_PF (0x5U) ///< GPIOF
#define SYSCFG_EXTICR_PG (0x6U) ///< GPIOG
#define SYSCFG_EXTICR_PH (0x7U) ///< GPIOH
/** @} */

/** @name CMPCR - compensation cell control (0x20)
 *  @{ */
#define SYSCFG_CMPCR_CMP_PD_Pos (0U) ///< Bit 0
/** \regaccess{rw} Compensation cell on (1) or powered down (0). */
#define SYSCFG_CMPCR_CMP_PD ((uint32_t)1U << SYSCFG_CMPCR_CMP_PD_Pos)
#define SYSCFG_CMPCR_READY_Pos (8U) ///< Bit 8
/** \regaccess{ro} Compensation cell ready. */
#define SYSCFG_CMPCR_READY ((uint32_t)1U << SYSCFG_CMPCR_READY_Pos)
/** @} */

/** @} */

#endif /* SYSCFG_REG_H */
