/**
 * @file syscfg_reg.h
 * @brief SYSCFG (System configuration controller) register definitions for
 *        the STM32F446xx family.
 *
 * Register layout and field values derived from RM0390 (STM32F446xx
 * reference manual), section 9: System configuration controller (SYSCFG).
 * Requires RCC_APB2ENR_SYSCFGEN (rcc_reg.h) enabled before any register
 * here is accessible.
 *
 * Field coverage is scoped to what is required to route a GPIO pin to
 * an EXTI line (EXTICR1-4) plus the I/O compensation cell
 * control/status (CMPCR). MEMRMP (boot memory remap) and PMC
 * (peripheral mode configuration, including Ethernet PHY interface
 * selection, not applicable to the F446, which has no Ethernet MAC) are
 * modeled as bare registers for correct struct layout and offsets but
 * have no bit-level macros, since this project does not use them.
 */
#ifndef SYSCFG_REG_H
#define SYSCFG_REG_H

#include <stdint.h>

/**
 * @addtogroup device_peripherals
 * @{
 */

#define SYSCFG_BASE (0x40013800UL) ///< SYSCFG peripheral base address

/**
 * @brief SYSCFG register map (RM0390 section 9.2).
 *
 * RESERVED0 preserves the fixed offset to CMPCR (see the top-of-file
 * note above); this is an actual gap in the memory map, not a
 * struct-packing artifact.
 */
typedef struct SyscfgRegisters_t
{
  volatile uint32_t MEMRMP;       ///< 0x00: memory remap register
  volatile uint32_t PMC;          ///< 0x04: peripheral mode configuration register
  volatile uint32_t EXTICR[4];    ///< 0x08-0x14: external interrupt configuration registers 1-4
  volatile uint32_t RESERVED0[2]; ///< 0x18-0x1C: reserved
  volatile uint32_t CMPCR;        ///< 0x20: compensation cell control register
} SyscfgRegisters_t;

#define SYSCFG ((SyscfgRegisters_t *)SYSCFG_BASE) ///< Pointer to the SYSCFG register block

/**
 * @brief SYSCFG_EXTICRx port-selector field values (RM0390 9.2.3-9.2.6).
 *
 * Each EXTICRx register packs four 4-bit fields, one per EXTI line,
 * with 4 lines per register: EXTICR[0] covers EXTI0-3, EXTICR[1]
 * covers EXTI4-7, and so on. A given line's field position is
 * (line % 4) * 4 bits wide, indexing EXTICR[line / 4]. Each field
 * selects which GPIO port feeds that EXTI line; these values apply to
 * any of the 16 fields across all four registers, not to a single one.
 */
#define SYSCFG_EXTICR_PA (0x0U) ///< EXTICRx field value: GPIOA feeds this EXTI line
#define SYSCFG_EXTICR_PB (0x1U) ///< EXTICRx field value: GPIOB feeds this EXTI line
#define SYSCFG_EXTICR_PC (0x2U) ///< EXTICRx field value: GPIOC feeds this EXTI line
#define SYSCFG_EXTICR_PD (0x3U) ///< EXTICRx field value: GPIOD feeds this EXTI line
#define SYSCFG_EXTICR_PE (0x4U) ///< EXTICRx field value: GPIOE feeds this EXTI line
#define SYSCFG_EXTICR_PF (0x5U) ///< EXTICRx field value: GPIOF feeds this EXTI line
#define SYSCFG_EXTICR_PG (0x6U) ///< EXTICRx field value: GPIOG feeds this EXTI line
#define SYSCFG_EXTICR_PH (0x7U) ///< EXTICRx field value: GPIOH feeds this EXTI line

/* --- SYSCFG_CMPCR bit definitions --- */
#define SYSCFG_CMPCR_CMP_PD_Pos (0U)                        ///< Bit position within SYSCFG_CMPCR
#define SYSCFG_CMPCR_CMP_PD (1U << SYSCFG_CMPCR_CMP_PD_Pos) ///< WRITE - 0=power-down, 1=enable
#define SYSCFG_CMPCR_READY_Pos (8U)                         ///< Bit position within SYSCFG_CMPCR
#define SYSCFG_CMPCR_READY (1U << SYSCFG_CMPCR_READY_Pos)   ///< READ only - compensation cell ready

/** @} */

#endif /* SYSCFG_REG_H */
