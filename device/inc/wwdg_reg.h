/**
 * @file wwdg_reg.h
 * @brief WWDG registers: the window watchdog (RM0390 section 21.6).
 *
 * Clocked from PCLK1. A refresh (rewriting CR.T) resets the chip if it comes
 * too late or before the counter has fallen below CFR.W, so the WWDG also
 * catches code that runs too fast.
 */
#ifndef WWDG_REG_H
#define WWDG_REG_H

#include <stdint.h>

/**
 * @addtogroup wwdg_registers
 * @{
 */

#define WWDG_BASE (0x40002C00UL) ///< WWDG base address (APB1)

/** @brief WWDG register block. */
typedef struct WwdgRegisters_t
{
    volatile uint32_t CR;  ///< 0x00: control
    volatile uint32_t CFR; ///< 0x04: configuration
    volatile uint32_t SR;  ///< 0x08: status
} WwdgRegisters_t;

#define WWDG ((WwdgRegisters_t *)WWDG_BASE) ///< The WWDG instance

/** @name CR - control (0x00)
 *  @{ */
#define WWDG_CR_T_Pos (0U) ///< Bits 6:0
/** \regaccess{rw} 7-bit down-counter; reset when bit 6 falls to 0. */
#define WWDG_CR_T_Msk ((uint32_t)0x7FU << WWDG_CR_T_Pos)
#define WWDG_CR_WDGA_Pos (7U) ///< Bit 7
/** \regaccess{rs} Activate the watchdog; only a reset clears it. */
#define WWDG_CR_WDGA ((uint32_t)1U << WWDG_CR_WDGA_Pos)
/** @} */

/** @name CFR - configuration (0x04)
 *  @{ */
#define WWDG_CFR_W_Pos (0U) ///< Bits 6:0
/** \regaccess{rw} Window value: refreshing above it resets the chip. */
#define WWDG_CFR_W_Msk ((uint32_t)0x7FU << WWDG_CFR_W_Pos)
#define WWDG_CFR_WDGTB_Pos (7U) ///< Bits 8:7
/** \regaccess{rw} Timer base: PCLK1 / 4096 / 2^WDGTB. */
#define WWDG_CFR_WDGTB_Msk ((uint32_t)0x3U << WWDG_CFR_WDGTB_Pos)
#define WWDG_CFR_WDGTB_DIV1 (0x0U) ///< WDGTB value: PCLK1 / 4096
#define WWDG_CFR_WDGTB_DIV2 (0x1U) ///< WDGTB value: PCLK1 / 4096 / 2
#define WWDG_CFR_WDGTB_DIV4 (0x2U) ///< WDGTB value: PCLK1 / 4096 / 4
#define WWDG_CFR_WDGTB_DIV8 (0x3U) ///< WDGTB value: PCLK1 / 4096 / 8
#define WWDG_CFR_EWI_Pos (9U)      ///< Bit 9
/** \regaccess{rs} Enable the early-wakeup interrupt; only a reset clears it. */
#define WWDG_CFR_EWI ((uint32_t)1U << WWDG_CFR_EWI_Pos)
/** @} */

/** @name SR - status (0x08)
 *  @{ */
#define WWDG_SR_EWIF_Pos (0U) ///< Bit 0
/** \regaccess{rc_w0} Early-wakeup flag. */
#define WWDG_SR_EWIF ((uint32_t)1U << WWDG_SR_EWIF_Pos)
/** @} */

/** @} */

#endif /* WWDG_REG_H */
