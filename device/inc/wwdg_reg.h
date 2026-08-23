/**
 * @file wwdg_reg.h
 * @brief WWDG (Window Watchdog) peripheral register definitions for the
 *        STM32F446xx family.
 *
 * Register layout and field values derived from RM0390 (STM32F446xx
 * reference manual), section 20: Window watchdog (WWDG).
 *
 * Unlike IWDG (iwdg_reg.h), WWDG runs off PCLK1 (via RCC, see
 * rcc_reg.h) rather than an independent clock, and faults if refreshed
 * too early as well as too late - CR.T must only be rewritten once it
 * has counted down into CFR.W's window, catching a task that's running
 * fast/looping too tightly, not just one that's hung.
 */
#ifndef WWDG_REG_H
#define WWDG_REG_H

#include <stdint.h>

#define WWDG_BASE (0x40002C00UL) ///< WWDG peripheral base address

/**
 * @brief WWDG register map (RM0390 section 20.6).
 */
typedef struct WwdgRegisters_t
{
  volatile uint32_t CR;  ///< 0x00: control register
  volatile uint32_t CFR; ///< 0x04: configuration register
  volatile uint32_t SR;  ///< 0x08: status register
} WwdgRegisters_t;

#define WWDG ((WwdgRegisters_t *)WWDG_BASE) ///< Pointer to the WWDG register block

/* --- WWDG_CR bit definitions --- */
#define WWDG_CR_T_Pos (0U)                     ///< Bit position within WWDG_CR
#define WWDG_CR_T_Msk (0x7FU << WWDG_CR_T_Pos) ///< bits 6:0, 7-bit down-counter
#define WWDG_CR_WDGA_Pos (7U)                  ///< Bit position within WWDG_CR
#define WWDG_CR_WDGA (1U << WWDG_CR_WDGA_Pos) ///< WRITE - activate the watchdog (cannot be cleared)

/* --- WWDG_CFR bit definitions --- */
#define WWDG_CFR_W_Pos (0U)                             ///< Bit position within WWDG_CFR
#define WWDG_CFR_W_Msk (0x7FU << WWDG_CFR_W_Pos)        ///< bits 6:0, 7-bit window value
#define WWDG_CFR_WDGTB_Pos (7U)                         ///< Bit position within WWDG_CFR
#define WWDG_CFR_WDGTB_Msk (0x3U << WWDG_CFR_WDGTB_Pos) ///< bits 8:7, timer base prescaler
#define WWDG_CFR_EWI_Pos (9U)                           ///< Bit position within WWDG_CFR
#define WWDG_CFR_EWI (1U << WWDG_CFR_EWI_Pos)           ///< WRITE - enable early-wakeup interrupt

/* --- WWDG_SR bit definitions --- */
#define WWDG_SR_EWIF_Pos (0U)                 ///< Bit position within WWDG_SR
#define WWDG_SR_EWIF (1U << WWDG_SR_EWIF_Pos) ///< READ/WRITE 0 to clear - early-wakeup flag

#endif /* WWDG_REG_H */
