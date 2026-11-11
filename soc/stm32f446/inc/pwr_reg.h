/**
 * @file pwr_reg.h
 * @brief PWR registers: regulator voltage scaling and over-drive (RM0390 section 5.4).
 *
 * The voltage scale sets the maximum HCLK, and flash latency must match the
 * resulting SYSCLK (RM0390 Table 5). See
 * [Clock bring-up order](DESIGN.md#clock-bring-up-order).
 */
#ifndef PWR_REG_H
#define PWR_REG_H

#include <stdint.h>

/**
 * @addtogroup pwr_registers
 * @{
 */

#define PWR_BASE (0x40007000UL) ///< PWR base address (APB1)

/** @brief PWR register block. */
typedef struct PwrRegisters_t
{
    volatile uint32_t CR;  ///< 0x00: power control
    volatile uint32_t CSR; ///< 0x04: power control/status
} PwrRegisters_t;

#define PWR ((PwrRegisters_t *)PWR_BASE) ///< The PWR instance

/** @name CR - power control (0x00)
 *  @{ */
#define PWR_CR_LPDS_Pos (0U) ///< Bit 0
/** \regaccess{rw} Low-power regulator in Stop mode. */
#define PWR_CR_LPDS ((uint32_t)1U << PWR_CR_LPDS_Pos)
#define PWR_CR_PDDS_Pos (1U) ///< Bit 1
/** \regaccess{rw} Enter Standby instead of Stop on deep sleep. */
#define PWR_CR_PDDS ((uint32_t)1U << PWR_CR_PDDS_Pos)
#define PWR_CR_CWUF_Pos (2U) ///< Bit 2
/** \regaccess{rc_w1} Clear the wakeup flag. */
#define PWR_CR_CWUF ((uint32_t)1U << PWR_CR_CWUF_Pos)
#define PWR_CR_CSBF_Pos (3U) ///< Bit 3
/** \regaccess{rc_w1} Clear the standby flag. */
#define PWR_CR_CSBF ((uint32_t)1U << PWR_CR_CSBF_Pos)
#define PWR_CR_PVDE_Pos (4U) ///< Bit 4
/** \regaccess{rw} Enable the programmable voltage detector. */
#define PWR_CR_PVDE ((uint32_t)1U << PWR_CR_PVDE_Pos)
#define PWR_CR_PLS_Pos (5U) ///< Bits 7:5
/** \regaccess{rw} PVD threshold level. */
#define PWR_CR_PLS_Msk ((uint32_t)0x7U << PWR_CR_PLS_Pos)
#define PWR_CR_DBP_Pos (8U) ///< Bit 8
/** \regaccess{rw} Disable backup-domain write protection. */
#define PWR_CR_DBP ((uint32_t)1U << PWR_CR_DBP_Pos)
#define PWR_CR_VOS_Pos (14U) ///< Bits 15:14
/** \regaccess{rw} Regulator voltage scale; values below. Writable only with the PLL off. */
#define PWR_CR_VOS_Msk ((uint32_t)0x3U << PWR_CR_VOS_Pos)
#define PWR_CR_ODEN_Pos (16U) ///< Bit 16
/** \regaccess{rw} Enable over-drive. */
#define PWR_CR_ODEN ((uint32_t)1U << PWR_CR_ODEN_Pos)
#define PWR_CR_ODSWEN_Pos (17U) ///< Bit 17
/** \regaccess{rw} Switch the regulator to over-drive. */
#define PWR_CR_ODSWEN ((uint32_t)1U << PWR_CR_ODSWEN_Pos)
/** @} */

/** @name CR.VOS values
 *  Maximum HCLK per scale: RM0390 section 3.4.1.
 *  @{ */
#define PWR_CR_VOS_SCALE3 (0x1U) ///< Scale 3: lowest power, lowest maximum frequency
#define PWR_CR_VOS_SCALE2 (0x2U) ///< Scale 2
#define PWR_CR_VOS_SCALE1 (0x3U) ///< Scale 1: highest maximum frequency (reset value)
/** @} */

/** @name CSR - power control/status (0x04)
 *  @{ */
#define PWR_CSR_WUF_Pos (0U) ///< Bit 0
/** \regaccess{ro} A wakeup event occurred. */
#define PWR_CSR_WUF ((uint32_t)1U << PWR_CSR_WUF_Pos)
#define PWR_CSR_SBF_Pos (1U) ///< Bit 1
/** \regaccess{ro} The device was in Standby. */
#define PWR_CSR_SBF ((uint32_t)1U << PWR_CSR_SBF_Pos)
#define PWR_CSR_PVDO_Pos (2U) ///< Bit 2
/** \regaccess{ro} VDD is below the PVD threshold. */
#define PWR_CSR_PVDO ((uint32_t)1U << PWR_CSR_PVDO_Pos)
#define PWR_CSR_VOSRDY_Pos (14U) ///< Bit 14
/** \regaccess{ro} Voltage scaling is ready. Sets only once the PLL is on. */
#define PWR_CSR_VOSRDY ((uint32_t)1U << PWR_CSR_VOSRDY_Pos)
#define PWR_CSR_ODRDY_Pos (16U) ///< Bit 16
/** \regaccess{ro} Over-drive is ready. */
#define PWR_CSR_ODRDY ((uint32_t)1U << PWR_CSR_ODRDY_Pos)
#define PWR_CSR_ODSWRDY_Pos (17U) ///< Bit 17
/** \regaccess{ro} The over-drive switch is complete. */
#define PWR_CSR_ODSWRDY ((uint32_t)1U << PWR_CSR_ODSWRDY_Pos)
/** @} */

/** @} */

#endif /* PWR_REG_H */
