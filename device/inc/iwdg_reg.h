/**
 * @file iwdg_reg.h
 * @brief IWDG (Independent Watchdog) peripheral register definitions for
 *        the STM32F446xx family.
 *
 * Register layout and field values derived from RM0390 (STM32F446xx
 * reference manual), section 21: Independent watchdog (IWDG).
 *
 * The IWDG runs off its own LSI clock, independent of the main system
 * clock tree in rcc_reg.h; it continues counting, and can still reset
 * the chip, even if a clock-configuration bug hangs SYSCLK entirely,
 * which is its purpose as a safety mechanism. All writes to PR/RLR/WINR
 * require KR unlocked with KEY_ENABLE first (RM0390 21.4.1), and SR
 * must be polled clear of PVU/RVU/WVU before a value written to
 * PR/RLR/WINR can be relied upon to have taken effect.
 */
#ifndef IWDG_REG_H
#define IWDG_REG_H

#include <stdint.h>

/**
 * @addtogroup iwdg_registers
 * @{
 */

#define IWDG_BASE (0x40003000UL) ///< IWDG peripheral base address

/**
 * @brief IWDG register map (RM0390 section 21.4).
 */
typedef struct IwdgRegisters_t
{
  volatile uint32_t KR;       ///< 0x00: key register (WRITE only)
  volatile uint32_t PR;       ///< 0x04: prescaler register
  volatile uint32_t RLR;      ///< 0x08: reload register
  volatile const uint32_t SR; ///< 0x0C: status register (READ only)
  volatile uint32_t WINR;     ///< 0x10: window register
} IwdgRegisters_t;

#define IWDG ((IwdgRegisters_t *)IWDG_BASE) ///< Pointer to the IWDG register block

/* --- IWDG_KR key values (RM0390 21.4.1) --- */
#define IWDG_KEY_RELOAD (0xAAAAU)        ///< WRITE to KR - reload the counter from RLR
#define IWDG_KEY_ENABLE_ACCESS (0x5555U) ///< WRITE to KR - unlock PR/RLR/WINR for writing
#define IWDG_KEY_START (0xCCCCU)         ///< WRITE to KR - start the watchdog counting

/* --- IWDG_PR bit definitions --- */
#define IWDG_PR_PR_Pos (0U)                     ///< Bit position within IWDG_PR
#define IWDG_PR_PR_Msk (0x7U << IWDG_PR_PR_Pos) ///< bits 2:0, prescaler divider selection

/* --- IWDG_RLR bit definitions --- */
#define IWDG_RLR_RL_Pos (0U)                        ///< Bit position within IWDG_RLR
#define IWDG_RLR_RL_Msk (0xFFFU << IWDG_RLR_RL_Pos) ///< bits 11:0, 12-bit reload value

/* --- IWDG_SR bit definitions --- */
#define IWDG_SR_PVU_Pos (0U)                ///< Bit position within IWDG_SR
#define IWDG_SR_PVU (1U << IWDG_SR_PVU_Pos) ///< READ only - PR write still in progress
#define IWDG_SR_RVU_Pos (1U)                ///< Bit position within IWDG_SR
#define IWDG_SR_RVU (1U << IWDG_SR_RVU_Pos) ///< READ only - RLR write still in progress
#define IWDG_SR_WVU_Pos (2U)                ///< Bit position within IWDG_SR
#define IWDG_SR_WVU (1U << IWDG_SR_WVU_Pos) ///< READ only - WINR write still in progress

/* --- IWDG_WINR bit definitions --- */
#define IWDG_WINR_WIN_Pos (0U)                          ///< Bit position within IWDG_WINR
#define IWDG_WINR_WIN_Msk (0xFFFU << IWDG_WINR_WIN_Pos) ///< bits 11:0, 12-bit window value

/** @} */

#endif /* IWDG_REG_H */
