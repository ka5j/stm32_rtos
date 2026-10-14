/**
 * @file wwdg.c
 * @brief WWDG driver implementation (API in wwdg.h).
 */
#include "wwdg.h"

/**
 * @addtogroup wwdg_driver
 * @{
 */

/**
 * @brief Accept a counter or window value: 0x40 to 0x7F.
 * @param value Candidate value.
 * @retval DRIVER_STATUS_OK                In range.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM Otherwise.
 */
static DriverStatus_e wwdgValidateCount(uint32_t value)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((value < WWDG_COUNTER_MIN) || (value > WWDG_COUNTER_MAX))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    return status;
}

DriverStatus_e wwdgStart(WwdgRegisters_t *wwdg, uint32_t prescaler, uint32_t window,
                         uint32_t counter)
{
    DriverStatus_e status = wwdgValidateCount(window);

    if (status == DRIVER_STATUS_OK)
    {
        status = wwdgValidateCount(counter);
    }

    if ((status == DRIVER_STATUS_OK) && (prescaler > WWDG_CFR_WDGTB_DIV8))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        wwdg->CFR = (wwdg->CFR & WWDG_CFR_EWI) | (window << WWDG_CFR_W_Pos)
                    | (prescaler << WWDG_CFR_WDGTB_Pos);
        wwdg->CR = WWDG_CR_WDGA | (counter << WWDG_CR_T_Pos);

        /* The counter free-runs from the moment the clock is enabled, even
         * with WDGA clear, and sets EWIF each time it passes 0x40 whether or
         * not EWI is enabled. That stale flag would raise the early wakeup at
         * once, and a refresh from it would come above the window. Cleared
         * after the start, the counter is back at @p counter and cannot set
         * it again for a full period. */
        wwdg->SR = 0U;
    }

    return status;
}

DriverStatus_e wwdgRefresh(WwdgRegisters_t *wwdg, uint32_t counter)
{
    DriverStatus_e status = wwdgValidateCount(counter);

    if (status == DRIVER_STATUS_OK)
    {
        wwdg->CR = WWDG_CR_WDGA | (counter << WWDG_CR_T_Pos);
    }

    return status;
}

void wwdgEnableEarlyWakeup(WwdgRegisters_t *wwdg)
{
    wwdg->CFR |= WWDG_CFR_EWI;
}

void wwdgClearEarlyWakeup(WwdgRegisters_t *wwdg)
{
    /* EWIF is the only SR bit, and it clears on a written 0. */
    wwdg->SR = 0U;
}

void wwdgFreezeInDebug(DbgmcuRegisters_t *dbgmcu)
{
    dbgmcu->APB1_FZ |= DBGMCU_APB1_FZ_DBG_WWDG_STOP;
}

/** @} */
