/**
 * @file pwr.h
 * @brief PWR driver: regulator voltage scale and over-drive.
 *
 * Voltage scaling is split around the PLL: CR.VOS can be written only while
 * the PLL is off, and the regulator reaches the new scale only once the PLL
 * is on. So pwrSetVoltageScale() runs before rccPllEnable() and
 * pwrWaitVoltageScaleReady() after it. See rcc.h for the full order and
 * [Clock bring-up order](DESIGN.md#clock-bring-up-order).
 * @code
 * rccPwrClockEnable(RCC);
 * if (pwrSetVoltageScale(PWR, PWR_CR_VOS_SCALE1) != DRIVER_STATUS_OK) { ... }
 * // ... rccPllConfig(), rccPllEnable() ...
 * if (pwrWaitVoltageScaleReady(PWR) != DRIVER_STATUS_OK) { ... }
 * if (pwrEnableOverdrive(PWR) != DRIVER_STATUS_OK) { ... } // only for 180 MHz
 * @endcode
 *
 * Maximum HCLK per scale (RM0390 section 3.4.1):
 *
 * | Scale | Normal  | With over-drive |
 * | ----- | ------- | --------------- |
 * | 3     | 120 MHz | -               |
 * | 2     | 144 MHz | 168 MHz         |
 * | 1     | 168 MHz | 180 MHz         |
 */
#ifndef PWR_H
#define PWR_H

#include "driver_status.h"
#include "pwr_reg.h"

#include <stdint.h>

/**
 * @addtogroup pwr_driver
 * @{
 */

/**
 * @brief Select the regulator voltage scale for the coming PLL frequency.
 *
 * Takes effect when the PLL is turned on, so there is nothing to wait for here.
 *
 * @param pwr PWR block (e.g. PWR).
 * @param vos ::PWR_CR_VOS_SCALE1, ::PWR_CR_VOS_SCALE2 or ::PWR_CR_VOS_SCALE3.
 * @pre The PWR clock is on (rccPwrClockEnable()).
 * @pre The PLL is off. This is not checked: PLLON is in RCC.
 * @retval DRIVER_STATUS_OK                CR.VOS written.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p vos is not a scale value.
 */
DRIVER_MUST_CHECK DriverStatus_e pwrSetVoltageScale(PwrRegisters_t *pwr, uint32_t vos);

/**
 * @brief Wait until the regulator reaches the selected scale (CSR.VOSRDY).
 * @param pwr PWR block (e.g. PWR); only CSR is read.
 * @pre pwrSetVoltageScale() has run and the PLL has since been turned on.
 *      With the PLL off this can only time out.
 * @retval DRIVER_STATUS_OK          VOSRDY is set.
 * @retval DRIVER_STATUS_ERR_TIMEOUT VOSRDY did not set within the poll budget.
 */
DRIVER_MUST_CHECK DriverStatus_e pwrWaitVoltageScaleReady(const PwrRegisters_t *pwr);

/**
 * @brief Enable over-drive, to run above the scale's normal maximum.
 *
 * Sets ODEN and waits for ODRDY, then sets ODSWEN and waits for ODSWRDY,
 * as ST's HAL does. CR.VOS is preserved.
 *
 * @param pwr PWR block (e.g. PWR).
 * @pre The scale is selected and ready (pwrWaitVoltageScaleReady()).
 * @pre SYSCLK is still HSI or HSE, not the PLL.
 * @pre No peripheral clock other than PWR is on (RM0390 section 5.1.3).
 * @retval DRIVER_STATUS_OK          ODSWRDY is set.
 * @retval DRIVER_STATUS_ERR_TIMEOUT ODRDY did not set (ODSWEN is then not written), or
 *                                   ODSWRDY did not.
 */
DRIVER_MUST_CHECK DriverStatus_e pwrEnableOverdrive(PwrRegisters_t *pwr);

/** @} */

#endif /* PWR_H */
