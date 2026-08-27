/**
 * @file pwr_reg.h
 * @brief PWR (Power control) peripheral register definitions for the
 *        STM32F446xx family.
 *
 * Register layout and field values derived from RM0390 (STM32F446xx
 * reference manual), section 5.4: PWR registers.
 *
 * CR.VOS pairs with flash_reg.h's ACR.LATENCY: raising SYSCLK requires
 * selecting the voltage scale (and polling CSR.VOSRDY) before or
 * alongside the flash latency change, matching whatever range the
 * target SYSCLK falls into per RM0390 Table 15. See that table for the
 * exact scale/frequency/latency combination, since it is derivative-
 * and package-specific rather than a single fixed constant suitable for
 * hardcoding here.
 */
#ifndef PWR_REG_H
#define PWR_REG_H

#include <stdint.h>

/**
 * @addtogroup pwr_registers
 * @{
 */

#define PWR_BASE (0x40007000UL) ///< PWR peripheral base address

/**
 * @brief PWR register map (RM0390 section 5.4).
 */
typedef struct PwrRegisters_t
{
  volatile uint32_t CR;  ///< 0x00: power control register
  volatile uint32_t CSR; ///< 0x04: power control/status register
} PwrRegisters_t;

#define PWR ((PwrRegisters_t *)PWR_BASE) ///< Pointer to the PWR register block

/* --- PWR_CR bit definitions --- */
#define PWR_CR_LPDS_Pos (0U)                    ///< Bit position within PWR_CR
#define PWR_CR_LPDS (1U << PWR_CR_LPDS_Pos)     ///< WRITE - low-power regulator in Stop mode
#define PWR_CR_PDDS_Pos (1U)                    ///< Bit position within PWR_CR
#define PWR_CR_PDDS (1U << PWR_CR_PDDS_Pos)     ///< WRITE - power down instead of Stop on deepsleep
#define PWR_CR_CWUF_Pos (2U)                    ///< Bit position within PWR_CR
#define PWR_CR_CWUF (1U << PWR_CR_CWUF_Pos)     ///< WRITE 1 to clear - clear wakeup flag
#define PWR_CR_CSBF_Pos (3U)                    ///< Bit position within PWR_CR
#define PWR_CR_CSBF (1U << PWR_CR_CSBF_Pos)     ///< WRITE 1 to clear - clear standby flag
#define PWR_CR_PVDE_Pos (4U)                    ///< Bit position within PWR_CR
#define PWR_CR_PVDE (1U << PWR_CR_PVDE_Pos)     ///< WRITE - enable programmable voltage detector
#define PWR_CR_PLS_Pos (5U)                     ///< Bit position within PWR_CR
#define PWR_CR_PLS_Msk (0x7U << PWR_CR_PLS_Pos) ///< bits 7:5, PVD threshold level
#define PWR_CR_DBP_Pos (8U)                     ///< Bit position within PWR_CR
#define PWR_CR_DBP (1U << PWR_CR_DBP_Pos)       ///< WRITE - disable backup domain write protection
#define PWR_CR_VOS_Pos (14U)                    ///< Bit position within PWR_CR
#define PWR_CR_VOS_Msk (0x3U << PWR_CR_VOS_Pos) ///< bits 15:14, regulator voltage scaling
#define PWR_CR_ODEN_Pos (16U)                   ///< Bit position within PWR_CR
#define PWR_CR_ODEN (1U << PWR_CR_ODEN_Pos)     ///< WRITE - enable over-drive mode
#define PWR_CR_ODSWEN_Pos (17U)                 ///< Bit position within PWR_CR
#define PWR_CR_ODSWEN (1U << PWR_CR_ODSWEN_Pos) ///< WRITE - switch regulator to over-drive

/* --- PWR_CR.VOS field values (RM0390 Table 15 gives the matching frequency range) --- */
#define PWR_CR_VOS_SCALE3 (0x1U) ///< VOS field value: scale 3 - lowest power, lowest max SYSCLK
#define PWR_CR_VOS_SCALE2 (0x2U) ///< VOS field value: scale 2 - mid range
#define PWR_CR_VOS_SCALE1 (0x3U) ///< VOS field value: scale 1 - highest max SYSCLK

/* --- PWR_CSR bit definitions --- */
#define PWR_CSR_WUF_Pos (0U)                        ///< Bit position within PWR_CSR
#define PWR_CSR_WUF (1U << PWR_CSR_WUF_Pos)         ///< READ only - wakeup event occurred
#define PWR_CSR_SBF_Pos (1U)                        ///< Bit position within PWR_CSR
#define PWR_CSR_SBF (1U << PWR_CSR_SBF_Pos)         ///< READ only - standby mode entered
#define PWR_CSR_PVDO_Pos (2U)                       ///< Bit position within PWR_CSR
#define PWR_CSR_PVDO (1U << PWR_CSR_PVDO_Pos)       ///< READ only - VDD below PVD threshold
#define PWR_CSR_VOSRDY_Pos (14U)                    ///< Bit position within PWR_CSR
#define PWR_CSR_VOSRDY (1U << PWR_CSR_VOSRDY_Pos)   ///< READ only - voltage scaling switch complete
#define PWR_CSR_ODRDY_Pos (16U)                     ///< Bit position within PWR_CSR
#define PWR_CSR_ODRDY (1U << PWR_CSR_ODRDY_Pos)     ///< READ only - over-drive mode ready
#define PWR_CSR_ODSWRDY_Pos (17U)                   ///< Bit position within PWR_CSR
#define PWR_CSR_ODSWRDY (1U << PWR_CSR_ODSWRDY_Pos) ///< READ only - over-drive switch complete

/** @} */

#endif /* PWR_REG_H */
