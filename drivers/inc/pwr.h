/**
 * @file pwr.h
 * @brief PWR (power control) driver - regulator voltage-scale
 *        configuration for the STM32F446xx (device/inc/pwr_reg.h). No
 *        application-facing logic; consumed by the SYSCLK bring-up
 *        sequence documented in rcc.h.
 *
 * Voltage scaling is a two-phase operation on this part, and the two
 * phases are separated by the PLL being enabled - which is why this
 * driver exposes them as two functions rather than one. Per RM0390
 * section 5.1.4, CR.VOS can be modified *only while the PLL is off*, and
 * the value programmed there becomes active only once the PLL is
 * switched *on*; with the PLL off the regulator is forced to scale 3
 * regardless of what CR.VOS holds. CSR.VOSRDY follows the same rule: it
 * reports that the regulator has reached the programmed level after the
 * PLL came on, not that the CR.VOS write landed. Polling it immediately
 * after writing CR.VOS, with the PLL still off, waits for a bit that
 * cannot set yet.
 *
 * So: pwrSetVoltageScale() before the PLL is enabled,
 * pwrWaitVoltageScaleReady() after - see rcc.h's file-level comment for
 * the full ordered bring-up sequence these two sit inside.
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
 * @brief Select the main regulator's voltage scale (CR.VOS) for the
 *        SYSCLK frequency the PLL is about to be brought up to.
 *
 * Must be set to at least the scale RM0390 Table 15 requires for the
 * target SYSCLK frequency, before the PLL is enabled. Scale 1 supports
 * the full frequency range, so selecting a more permissive scale than
 * the target frequency strictly requires is always electrically safe
 * (costs power, never correctness).
 *
 * A single deterministic register write, with nothing to poll here: the
 * regulator does not begin moving to the programmed level until the PLL
 * is enabled, so there is no completion to wait for at this point.
 * pwrWaitVoltageScaleReady() covers that half, after rccPllEnable().
 *
 * @param pwr PWR register block (e.g. PWR).
 * @param vos Target value for CR.VOS - one of ::PWR_CR_VOS_SCALE1,
 *            ::PWR_CR_VOS_SCALE2, ::PWR_CR_VOS_SCALE3 (0x0 is reserved
 *            per RM0390 and is not a valid input).
 * @pre RCC_APB1ENR_PWREN is already enabled (rcc.h's rccPwrClockEnable()) -
 *      PWR's registers are not accessible before this.
 * @pre The PLL is off (CR.PLLON clear). RM0390 section 5.1.4 permits
 *      CR.VOS to be modified only while the PLL is off; a write attempted
 *      with the PLL running is discarded by the hardware, and this driver
 *      cannot detect that from PWR's own registers - PLLON lives in RCC.
 * @return DRIVER_STATUS_OK once CR.VOS has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if vos is not one of the three
 *         documented scale values.
 */
DRIVER_MUST_CHECK DriverStatus_e pwrSetVoltageScale(PwrRegisters_t *pwr, uint32_t vos);

/**
 * @brief Wait for the regulator to reach the voltage scale selected by
 *        pwrSetVoltageScale() (CSR.VOSRDY).
 *
 * Call this after rccPllEnable() and before switching SYSCLK to the PLL.
 * Enabling the PLL is what starts the regulator moving to the programmed
 * scale; running the core off the PLL before VOSRDY reports that move
 * complete is the case this guards against.
 *
 * @param pwr PWR register block (e.g. PWR). Taken as a pointer to const:
 *            this function only reads CSR, and reading CSR has no effect
 *            on the peripheral's state.
 * @pre pwrSetVoltageScale() has already selected the target scale, and
 *      the PLL has since been enabled (rcc.h's rccPllEnable()). With the
 *      PLL off, CSR.VOSRDY cannot set and this function can only time
 *      out - see this file's top comment.
 * @return DRIVER_STATUS_OK once CSR.VOSRDY is observed set.
 * @return DRIVER_STATUS_ERR_TIMEOUT if CSR.VOSRDY never set within
 *         PWR_VOSRDY_TIMEOUT_ITERATIONS iterations. This is a bounded
 *         retry count, not a wall-clock timeout - SysTick is not
 *         available this early in clock bring-up (its reload value
 *         depends on the SYSCLK frequency this sequence is establishing),
 *         the same reasoning vendor startup code (e.g. CMSIS
 *         system_stm32f4xx.c) uses for HSE/PLL ready-waits.
 */
DRIVER_MUST_CHECK DriverStatus_e pwrWaitVoltageScaleReady(const PwrRegisters_t *pwr);

/** @} */

#endif /* PWR_H */
