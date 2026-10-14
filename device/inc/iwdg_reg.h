/**
 * @file iwdg_reg.h
 * @brief IWDG registers: the independent watchdog (RM0390 section 20.4).
 *
 * Clocked by the LSI, so it can still reset the chip if SYSCLK hangs.
 * - Write IWDG_KEY_ENABLE_ACCESS to KR before writing PR or RLR.
 * - Wait for SR.PVU / SR.RVU to clear before writing PR / RLR again.
 *
 * The F446's IWDG has no window register, so the block ends at SR (0x0C),
 * matching ST's SVD.
 */
#ifndef IWDG_REG_H
#define IWDG_REG_H

#include <stdint.h>

/**
 * @addtogroup iwdg_registers
 * @{
 */

#define IWDG_BASE (0x40003000UL) ///< IWDG base address (APB1)

/** @brief IWDG register block. */
typedef struct IwdgRegisters_t
{
    volatile uint32_t KR;       ///< 0x00: key, \regaccess{wo}
    volatile uint32_t PR;       ///< 0x04: prescaler
    volatile uint32_t RLR;      ///< 0x08: reload value
    volatile const uint32_t SR; ///< 0x0C: status, \regaccess{ro}
} IwdgRegisters_t;

#define IWDG ((IwdgRegisters_t *)IWDG_BASE) ///< The IWDG instance

/** @name KR - key values (0x00)
 *  @{ */
#define IWDG_KEY_RELOAD (0xAAAAU)        ///< Reload the counter from RLR (refresh)
#define IWDG_KEY_ENABLE_ACCESS (0x5555U) ///< Unlock PR and RLR for writing
#define IWDG_KEY_START (0xCCCCU)         ///< Start the watchdog; it cannot be stopped
/** @} */

/** @name PR - prescaler (0x04)
 *  @{ */
#define IWDG_PR_PR_Pos (0U) ///< Bits 2:0
/** \regaccess{rw} LSI divider: /4 << value, up to /256. */
#define IWDG_PR_PR_Msk ((uint32_t)0x7U << IWDG_PR_PR_Pos)
/** @} */

/** @name RLR - reload (0x08)
 *  @{ */
#define IWDG_RLR_RL_Pos (0U) ///< Bits 11:0
/** \regaccess{rw} Counter reload value. */
#define IWDG_RLR_RL_Msk ((uint32_t)0xFFFU << IWDG_RLR_RL_Pos)
/** @} */

/** @name SR - status (0x0C)
 *  @{ */
#define IWDG_SR_PVU_Pos (0U) ///< Bit 0
/** \regaccess{ro} A PR update is in progress. */
#define IWDG_SR_PVU ((uint32_t)1U << IWDG_SR_PVU_Pos)
#define IWDG_SR_RVU_Pos (1U) ///< Bit 1
/** \regaccess{ro} An RLR update is in progress. */
#define IWDG_SR_RVU ((uint32_t)1U << IWDG_SR_RVU_Pos)
/** @} */

/** @} */

#endif /* IWDG_REG_H */
