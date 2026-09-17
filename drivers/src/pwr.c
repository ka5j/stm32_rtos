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

/** Bounded retry count for CSR.VOSRDY, not a wall-clock timeout - see
 *  pwr.h's pwrWaitVoltageScaleReady() @return for why. */
#define PWR_VOSRDY_TIMEOUT_ITERATIONS (100000U)

DriverStatus_e pwrSetVoltageScale(PwrRegisters_t *pwr, uint32_t vos)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((vos != PWR_CR_VOS_SCALE1) && (vos != PWR_CR_VOS_SCALE2) && (vos != PWR_CR_VOS_SCALE3))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else
    {
        /* PWR_CR_VOS_Pos is a fixed, tested compile-time bit position
         * (device/inc/pwr_reg.h), not a runtime-computed shift -
         * cppcheck's MISRA addon can't bound a shift through a macro
         * expansion like this; see drivers/src/rcc.c's matching comment
         * on rccHseEnable() for the same finding on the same class of
         * macro. */
        // cppcheck-suppress misra-c2012-12.2
        pwr->CR = (pwr->CR & ~PWR_CR_VOS_Msk) | (vos << PWR_CR_VOS_Pos);
    }

    return status;
}

DriverStatus_e pwrWaitVoltageScaleReady(const PwrRegisters_t *pwr)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = PWR_VOSRDY_TIMEOUT_ITERATIONS;

    // cppcheck-suppress misra-c2012-12.2
    while (((pwr->CSR & PWR_CSR_VOSRDY) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    /* Re-read the flag rather than inferring the outcome from the
     * counter: VOSRDY setting on the final iteration exits the loop with
     * timeout already at 0, which a `timeout == 0` test would report as a
     * timeout despite the wait having succeeded. */
    // cppcheck-suppress misra-c2012-12.2
    if ((pwr->CSR & PWR_CSR_VOSRDY) == 0U)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }

    return status;
}

/** @} */
