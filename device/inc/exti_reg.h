/**
 * @file exti_reg.h
 * @brief EXTI registers: external interrupt and event lines (RM0390 section 10.3).
 *
 * Each register is a bitmask with bit N for line N. Lines 0-15 are GPIO pin
 * N of the port SYSCFG_EXTICR selects (see syscfg_reg.h); lines 16-22 are the
 * fixed internal sources below. Line 19 has no source on the F446.
 */
#ifndef EXTI_REG_H
#define EXTI_REG_H

#include <stdint.h>

/**
 * @addtogroup exti_registers
 * @{
 */

#define EXTI_BASE (0x40013C00UL) ///< EXTI base address (APB2)

/** @brief EXTI register block. Every register is \regaccess{rw} except PR. */
typedef struct ExtiRegisters_t
{
    volatile uint32_t IMR;   ///< 0x00: interrupt mask (1 = enabled)
    volatile uint32_t EMR;   ///< 0x04: event mask (1 = enabled)
    volatile uint32_t RTSR;  ///< 0x08: rising-edge trigger
    volatile uint32_t FTSR;  ///< 0x0C: falling-edge trigger
    volatile uint32_t SWIER; ///< 0x10: software interrupt trigger
    volatile uint32_t PR;    ///< 0x14: pending, \regaccess{rc_w1}
} ExtiRegisters_t;

#define EXTI ((ExtiRegisters_t *)EXTI_BASE) ///< The EXTI instance

/** @name Internal line numbers (RM0390 section 10.2.5)
 *  @{ */
#define EXTI_LINE_PVD (16U)         ///< PVD output
#define EXTI_LINE_RTC_ALARM (17U)   ///< RTC alarm
#define EXTI_LINE_OTG_FS_WKUP (18U) ///< USB OTG FS wakeup
#define EXTI_LINE_OTG_HS_WKUP (20U) ///< USB OTG HS (in FS mode) wakeup
#define EXTI_LINE_TAMP_STAMP (21U)  ///< Tamper and timestamp
#define EXTI_LINE_RTC_WKUP (22U)    ///< RTC wakeup
/** @} */

/** @} */

#endif /* EXTI_REG_H */
