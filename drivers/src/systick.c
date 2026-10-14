/**
 * @file systick.c
 * @brief SysTick driver implementation (API in systick.h).
 */
#include "systick.h"

/**
 * @addtogroup systick_driver
 * @{
 */

DriverStatus_e systickInit(SysTickRegisters_t *systick, uint32_t core_hz, uint32_t tick_hz)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t reload = 0U;

    if ((core_hz == 0U) || (tick_hz == 0U))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else
    {
        /* tick_hz > core_hz gives 0 clocks, rejected below. */
        uint32_t period_clocks = core_hz / tick_hz;

        if (period_clocks < 2U)
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
        else
        {
            reload = period_clocks - 1U;
        }
    }

    if ((status == DRIVER_STATUS_OK) && (reload > SYSTICK_LOAD_RELOAD_Msk))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        systick->CTRL = 0U;
        systick->LOAD = reload;
        systick->VAL = 0U;
        systick->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
    }

    return status;
}

void systickDeinit(SysTickRegisters_t *systick)
{
    systick->CTRL = 0U;
    systick->LOAD = 0U;
    systick->VAL = 0U;
}

/** @} */
