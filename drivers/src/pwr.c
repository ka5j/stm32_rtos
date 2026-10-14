/**
 * @file pwr.c
 * @brief PWR driver implementation (API in pwr.h).
 */
#include "pwr.h"

/**
 * @addtogroup pwr_driver
 * @{
 */

/** Polls allowed for each CSR ready-flag wait (see
 *  [Poll-count timeouts](DESIGN.md#poll-count-timeouts)). */
#define PWR_READY_TIMEOUT_ITERATIONS (100000U)

/**
 * @brief Poll a CSR flag until it sets or ::PWR_READY_TIMEOUT_ITERATIONS polls pass.
 * @param pwr  PWR block; only CSR is read.
 * @param flag ::PWR_CSR_VOSRDY, ::PWR_CSR_ODRDY or ::PWR_CSR_ODSWRDY.
 * @retval DRIVER_STATUS_OK          The flag is set.
 * @retval DRIVER_STATUS_ERR_TIMEOUT It did not set in time.
 */
static DriverStatus_e pwrWaitCsrFlag(const PwrRegisters_t *pwr, uint32_t flag)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = PWR_READY_TIMEOUT_ITERATIONS;

    while (((pwr->CSR & flag) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    /* Re-read the flag: one that sets on the last poll still counts. */
    if ((pwr->CSR & flag) == 0U)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }

    return status;
}

DriverStatus_e pwrSetVoltageScale(PwrRegisters_t *pwr, uint32_t vos)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((vos != PWR_CR_VOS_SCALE1) && (vos != PWR_CR_VOS_SCALE2) && (vos != PWR_CR_VOS_SCALE3))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else
    {
        pwr->CR = (pwr->CR & ~PWR_CR_VOS_Msk) | (vos << PWR_CR_VOS_Pos);
    }

    return status;
}

DriverStatus_e pwrWaitVoltageScaleReady(const PwrRegisters_t *pwr)
{
    return pwrWaitCsrFlag(pwr, PWR_CSR_VOSRDY);
}

DriverStatus_e pwrEnableOverdrive(PwrRegisters_t *pwr)
{
    DriverStatus_e status;

    /* Read-modify-writes keep CR.VOS. ODSWEN only after ODRDY confirms ODEN. */
    pwr->CR |= PWR_CR_ODEN;
    status = pwrWaitCsrFlag(pwr, PWR_CSR_ODRDY);

    if (status == DRIVER_STATUS_OK)
    {
        pwr->CR |= PWR_CR_ODSWEN;
        status = pwrWaitCsrFlag(pwr, PWR_CSR_ODSWRDY);
    }

    return status;
}

/** @} */
