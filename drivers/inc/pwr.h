/**
 * @file pwr.h
 * @brief PWR (power control) driver - regulator voltage-scale
 *        configuration for the STM32F446xx (device/inc/pwr_reg.h). No
 *        application-facing logic; consumed by rcc.c as part of a SYSCLK
 *        frequency change.
 *
 * Takes PWR's register block as a parameter (PwrRegisters_t *) even
 * though PWR is a hardware singleton - see CONTRIBUTING.md's
 * error-handling contract section for why every driver in this project
 * does this regardless of instance count.
 */
#ifndef PWR_H
#define PWR_H

#include "driver_status.h"
#include "pwr_reg.h"

/**
 * @addtogroup driver_layer
 * @{
 */

/**
 * @brief Set the main regulator's voltage scale (CR.VOS) and wait for the
 *        switch to complete.
 *
 * Must be raised to at least the scale RM0390 Table 15 requires for the
 * target SYSCLK frequency *before* rcc.c switches RCC_CFGR.SW to a faster
 * source - see pwr_reg.h's file-level comment. Scale 1 supports the full
 * frequency range, so it is always electrically safe to select a more
 * permissive scale than the current SYSCLK strictly requires (costs
 * power, never correctness), which is why rcc.c applies the target scale
 * unconditionally before every switch, regardless of direction.
 *
 * @param pwr PWR register block (e.g. PWR).
 * @param vos Target value for CR.VOS - one of ::PWR_CR_VOS_SCALE1,
 *            ::PWR_CR_VOS_SCALE2, ::PWR_CR_VOS_SCALE3 (0x0 is reserved
 *            per RM0390 and is not a valid input).
 * @pre RCC_APB1ENR_PWREN is already enabled (rcc.h's rccPwrClockEnable()) -
 *      PWR's registers are not accessible before this.
 * @return DRIVER_STATUS_OK once CSR.VOSRDY confirms the switch completed.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if vos is not one of the three
 *         documented scale values.
 * @return DRIVER_STATUS_ERR_TIMEOUT if CSR.VOSRDY never set within
 *         PWR_VOSRDY_TIMEOUT_ITERATIONS iterations of writing CR.VOS. This
 *         is a bounded retry count, not a wall-clock timeout - SysTick is
 *         not available this early in clock bring-up (its reload value
 *         depends on the SYSCLK frequency this sequence is establishing),
 *         the same reasoning vendor startup code (e.g. CMSIS
 *         system_stm32f4xx.c) uses for HSE/PLL ready-waits.
 */
DRIVER_MUST_CHECK DriverStatus_e pwrSetVoltageScale(PwrRegisters_t *pwr, uint32_t vos);

/** @} */

#endif /* PWR_H */
