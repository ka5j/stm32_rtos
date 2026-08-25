/**
 * @file systick_reg.h
 * @brief SysTick core peripheral register definitions for the ARM
 *        Cortex-M4 (Armv7-M architecture).
 *
 * Register layout and field values derived from PM0214 (Cortex-M4
 * programming manual), section on the SysTick timer. Base address is
 * fixed by the Armv7-M architecture, not the STM32F446 vendor.
 */
#ifndef SYSTICK_REG_H
#define SYSTICK_REG_H

#include <stdint.h>

#define SYSTICK_BASE (0xE000E010UL) ///< SYSTICK peripheral base address

/**
 * @brief SysTick timer register map.
 */
typedef struct SysTickRegisters_t
{
  volatile uint32_t CTRL;        ///< 0x00: control and status register
  volatile uint32_t LOAD;        ///< 0x04: reload value register
  volatile uint32_t VAL;         ///< 0x08: current value register
  volatile const uint32_t CALIB; ///< 0x0C: calibration value register (READ only)
} SysTickRegisters_t;

#define SYSTICK ((SysTickRegisters_t *)SYSTICK_BASE) ///< Pointer to the SysTick register block

/* --- SYSTICK_CTRL bit definitions --- */
#define SYSTICK_CTRL_ENABLE_Pos (0U)                          ///< Bit position within SYSTICK_CTRL
#define SYSTICK_CTRL_ENABLE (1U << SYSTICK_CTRL_ENABLE_Pos)   ///< WRITE
#define SYSTICK_CTRL_TICKINT_Pos (1U)                         ///< Bit position within SYSTICK_CTRL
#define SYSTICK_CTRL_TICKINT (1U << SYSTICK_CTRL_TICKINT_Pos) ///< WRITE - enable SysTick IRQ
#define SYSTICK_CTRL_CLKSOURCE_Pos (2U)                       ///< Bit position within SYSTICK_CTRL
#define SYSTICK_CTRL_CLKSOURCE (1U << SYSTICK_CTRL_CLKSOURCE_Pos) ///< WRITE, 1=processor clock
#define SYSTICK_CTRL_COUNTFLAG_Pos (16U) ///< Bit position within SYSTICK_CTRL
#define SYSTICK_CTRL_COUNTFLAG (1U << SYSTICK_CTRL_COUNTFLAG_Pos) ///< READ only, clears on read

#endif /* SYSTICK_REG_H */
