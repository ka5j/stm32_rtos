/**
 * @file pwr.c
 * @brief PWR peripheral driver implementation - see pwr.h for the public
 *        API and pwr_reg.h for the register definitions this operates on.
 */
#include "pwr.h"

/**
 * @addtogroup driver_layer
 * @{
 */

/** Bounded retry count for each CSR ready-flag wait (VOSRDY, ODRDY,
 *  ODSWRDY), not a wall-clock timeout - see pwr.h's
 *  pwrWaitVoltageScaleReady() @return for why. */
#define PWR_READY_TIMEOUT_ITERATIONS (100000U)

/**
 * @brief Poll a CSR ready flag for up to PWR_READY_TIMEOUT_ITERATIONS.
 *
 * @param pwr  PWR register block; only CSR is read.
 * @param flag The CSR bit to wait on (PWR_CSR_VOSRDY, _ODRDY, _ODSWRDY).
 * @return DRIVER_STATUS_OK once the flag is observed set.
 * @return DRIVER_STATUS_ERR_TIMEOUT if it never set within the bound.
 */
static DriverStatus_e pwrWaitCsrFlag(const PwrRegisters_t *pwr, uint32_t flag)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = PWR_READY_TIMEOUT_ITERATIONS;

    while (((pwr->CSR & flag) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    /* Re-read the flag rather than inferring the outcome from the
     * counter: the flag setting on the final iteration exits the loop with
     * timeout already at 0, which a `timeout == 0` test would report as a
     * timeout despite the wait having succeeded. */
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

    /* Both steps are read-modify-writes so CR.VOS (and every other CR
     * field) is preserved. ODSWEN is written only once ODRDY confirms
     * ODEN took effect: switching before then is the case the two-step
     * sequence exists to prevent. */
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
