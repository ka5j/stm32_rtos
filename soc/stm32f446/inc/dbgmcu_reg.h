/**
 * @file dbgmcu_reg.h
 * @brief DBGMCU registers: the MCU debug component (RM0390, debug support
 *        chapter, DBGMCU).
 *
 * Only what the drivers use is modelled: the block layout and the two
 * watchdog freeze bits of APB1_FZ. These registers are reset by power-on only, not by
 * a system reset, so a freeze bit survives a watchdog reset.
 */
#ifndef DBGMCU_REG_H
#define DBGMCU_REG_H

#include <stdint.h>

/**
 * @addtogroup dbgmcu_registers
 * @{
 */

#define DBGMCU_BASE (0xE0042000UL) ///< DBGMCU base address (PPB)

/** @brief DBGMCU register block. */
typedef struct DbgmcuRegisters_t
{
    volatile const uint32_t IDCODE; ///< 0x00: device and revision ID, \regaccess{ro}
    volatile uint32_t CR;           ///< 0x04: debug in low-power modes
    volatile uint32_t APB1_FZ;      ///< 0x08: APB1 peripheral freeze while the core is halted
    volatile uint32_t APB2_FZ;      ///< 0x0C: APB2 peripheral freeze while the core is halted
} DbgmcuRegisters_t;

#define DBGMCU ((DbgmcuRegisters_t *)DBGMCU_BASE) ///< The DBGMCU instance

/** @name APB1_FZ - APB1 freeze (0x08)
 *  @{ */
#define DBGMCU_APB1_FZ_DBG_WWDG_STOP_Pos (11U) ///< Bit 11
/** \regaccess{rw} Stop the WWDG counter while the core is halted. */
#define DBGMCU_APB1_FZ_DBG_WWDG_STOP ((uint32_t)1U << DBGMCU_APB1_FZ_DBG_WWDG_STOP_Pos)
#define DBGMCU_APB1_FZ_DBG_IWDG_STOP_Pos (12U) ///< Bit 12
/** \regaccess{rw} Stop the IWDG counter while the core is halted. */
#define DBGMCU_APB1_FZ_DBG_IWDG_STOP ((uint32_t)1U << DBGMCU_APB1_FZ_DBG_IWDG_STOP_Pos)
/** @} */

/** @} */

#endif /* DBGMCU_REG_H */
