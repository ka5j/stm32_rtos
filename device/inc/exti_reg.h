/**
 * @file exti_reg.h
 * @brief EXTI (External interrupt/event controller) register definitions
 *        for the STM32F446xx family.
 *
 * Register layout and field values derived from RM0390 (STM32F446xx
 * reference manual), section 12: External interrupt/event controller
 * (EXTI).
 *
 * Lines 0-15 map directly to GPIO pin number N (the mux selecting which
 * port feeds line N is configured via SYSCFG and is not modeled here);
 * lines 16-22 are fixed internal sources (see the EXTI_LINE_* constants
 * below). Each register here is a flat 23-bit-wide bitmask; bit N
 * always represents line N in every one of IMR/EMR/RTSR/FTSR/SWIER/PR,
 * so there are no packed sub-fields to define beyond the line number
 * itself, unlike the other register headers in this project.
 */
#ifndef EXTI_REG_H
#define EXTI_REG_H

#include <stdint.h>

/**
 * @addtogroup exti_registers
 * @{
 */

#define EXTI_BASE (0x40013C00UL) ///< EXTI peripheral base address

/**
 * @brief EXTI register map (RM0390 section 12.3).
 */
typedef struct ExtiRegisters_t
{
  volatile uint32_t IMR;   ///< 0x00: interrupt mask register
  volatile uint32_t EMR;   ///< 0x04: event mask register
  volatile uint32_t RTSR;  ///< 0x08: rising trigger selection register
  volatile uint32_t FTSR;  ///< 0x0C: falling trigger selection register
  volatile uint32_t SWIER; ///< 0x10: software interrupt event register
  volatile uint32_t PR;    ///< 0x14: pending register
} ExtiRegisters_t;

#define EXTI ((ExtiRegisters_t *)EXTI_BASE) ///< Pointer to the EXTI register block

/**
 * @brief Fixed internal EXTI line numbers (RM0390 12.2) - lines 0-15 are
 *        GPIO pins 0-15 instead and need no such constant.
 *
 * Line 19 (Ethernet Wakeup) is architecturally present in the register
 * width but has no source on the F446, which has no Ethernet MAC, and
 * is therefore intentionally omitted here.
 */
#define EXTI_LINE_PVD (16U)         ///< PVD output
#define EXTI_LINE_RTC_ALARM (17U)   ///< RTC alarm
#define EXTI_LINE_OTG_FS_WKUP (18U) ///< USB OTG FS wakeup
#define EXTI_LINE_OTG_HS_WKUP (20U) ///< USB OTG HS (configured in FS) wakeup
#define EXTI_LINE_TAMP_STAMP (21U)  ///< Tamper and TimeStamp
#define EXTI_LINE_RTC_WKUP (22U)    ///< RTC wakeup

/** @} */

#endif /* EXTI_REG_H */
