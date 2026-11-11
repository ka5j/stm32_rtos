/**
 * @file systick_reg.h
 * @brief SysTick registers: the Armv7-M 24-bit system timer (PM0214, SysTick chapter).
 */
#ifndef SYSTICK_REG_H
#define SYSTICK_REG_H

#include <stdint.h>

/**
 * @addtogroup systick_registers
 * @{
 */

#define SYSTICK_BASE (0xE000E010UL) ///< SysTick base address (System Control Space)

/** @brief SysTick register block. */
typedef struct SysTickRegisters_t
{
    volatile uint32_t CTRL;        ///< 0x00: control and status
    volatile uint32_t LOAD;        ///< 0x04: reload value
    volatile uint32_t VAL;         ///< 0x08: current value; any write clears it
    volatile const uint32_t CALIB; ///< 0x0C: calibration, \regaccess{ro}
} SysTickRegisters_t;

#define SYSTICK ((SysTickRegisters_t *)SYSTICK_BASE) ///< The SysTick instance

/** @name CTRL - control and status (0x00)
 *  @{ */
#define SYSTICK_CTRL_ENABLE_Pos (0U) ///< Bit 0
/** \regaccess{rw} Counter enable. */
#define SYSTICK_CTRL_ENABLE ((uint32_t)1U << SYSTICK_CTRL_ENABLE_Pos)
#define SYSTICK_CTRL_TICKINT_Pos (1U) ///< Bit 1
/** \regaccess{rw} Raise the SysTick exception when the counter reaches 0. */
#define SYSTICK_CTRL_TICKINT ((uint32_t)1U << SYSTICK_CTRL_TICKINT_Pos)
#define SYSTICK_CTRL_CLKSOURCE_Pos (2U) ///< Bit 2
/** \regaccess{rw} Clock source: 1 = processor clock (HCLK), 0 = HCLK / 8. */
#define SYSTICK_CTRL_CLKSOURCE ((uint32_t)1U << SYSTICK_CTRL_CLKSOURCE_Pos)
#define SYSTICK_CTRL_COUNTFLAG_Pos (16U) ///< Bit 16
/** \regaccess{ro} The counter reached 0 since the last read; reading clears it. */
#define SYSTICK_CTRL_COUNTFLAG ((uint32_t)1U << SYSTICK_CTRL_COUNTFLAG_Pos)
/** @} */

/** @name LOAD, VAL - reload and current value (0x04, 0x08)
 *  @{ */
#define SYSTICK_LOAD_RELOAD_Pos (0U) ///< Bits 23:0
/** \regaccess{rw} Reload value: the tick period is RELOAD + 1 clocks. */
#define SYSTICK_LOAD_RELOAD_Msk ((uint32_t)0xFFFFFFU << SYSTICK_LOAD_RELOAD_Pos)
#define SYSTICK_VAL_CURRENT_Pos (0U) ///< Bits 23:0
/** \regaccess{rw} Current counter value. */
#define SYSTICK_VAL_CURRENT_Msk ((uint32_t)0xFFFFFFU << SYSTICK_VAL_CURRENT_Pos)
/** @} */

/** @name CALIB - calibration (0x0C)
 *  @{ */
#define SYSTICK_CALIB_TENMS_Pos (0U) ///< Bits 23:0
/** \regaccess{ro} Reload value for 10 ms (0 if unknown). */
#define SYSTICK_CALIB_TENMS_Msk ((uint32_t)0xFFFFFFU << SYSTICK_CALIB_TENMS_Pos)
#define SYSTICK_CALIB_SKEW_Pos (30U) ///< Bit 30
/** \regaccess{ro} TENMS is not exactly 10 ms. */
#define SYSTICK_CALIB_SKEW ((uint32_t)1U << SYSTICK_CALIB_SKEW_Pos)
#define SYSTICK_CALIB_NOREF_Pos (31U) ///< Bit 31
/** \regaccess{ro} No external reference clock. */
#define SYSTICK_CALIB_NOREF ((uint32_t)1U << SYSTICK_CALIB_NOREF_Pos)
/** @} */

/** @} */

#endif /* SYSTICK_REG_H */
