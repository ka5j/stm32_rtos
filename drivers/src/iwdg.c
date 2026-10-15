/**
 * @file iwdg.c
 * @brief IWDG driver implementation (API in iwdg.h).
 */
#include "iwdg.h"

/**
 * @addtogroup iwdg_driver
 * @{
 */

/** Polls allowed for SR.PVU and SR.RVU to clear: an update takes up to 5 LSI
 *  cycles, under 300 us even at 17 kHz (see
 *  [Poll-count timeouts](DESIGN.md#poll-count-timeouts)). */
#define IWDG_UPDATE_TIMEOUT_ITERATIONS (100000U)

/**
 * @brief Poll SR until no prescaler or reload update is in progress.
 * @param iwdg IWDG block; only SR is read.
 * @retval DRIVER_STATUS_OK          SR.PVU and SR.RVU are clear.
 * @retval DRIVER_STATUS_ERR_TIMEOUT One is still set after ::IWDG_UPDATE_TIMEOUT_ITERATIONS polls.
 */
static DriverStatus_e iwdgWaitUpdates(const IwdgRegisters_t *iwdg)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = IWDG_UPDATE_TIMEOUT_ITERATIONS;

    while (((iwdg->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    /* Re-read SR: an update that ends on the last poll still counts. */
    if ((iwdg->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }

    return status;
}

DriverStatus_e iwdgStart(IwdgRegisters_t *iwdg, uint32_t prescaler, uint32_t reload)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((prescaler > IWDG_PR_DIV256) || (reload > (IWDG_RLR_RL_Msk >> IWDG_RLR_RL_Pos)))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        iwdg->KR = IWDG_KEY_START;
        iwdg->KR = IWDG_KEY_ENABLE_ACCESS;

        /* A write to PR or RLR is ignored while its previous update runs. */
        status = iwdgWaitUpdates(iwdg);
    }

    if (status == DRIVER_STATUS_OK)
    {
        iwdg->PR = prescaler << IWDG_PR_PR_Pos;
        iwdg->RLR = reload << IWDG_RLR_RL_Pos;

        /* Refresh only once RLR has reached the counter's domain, or the
         * refresh would load the old value. */
        status = iwdgWaitUpdates(iwdg);
    }

    if (status == DRIVER_STATUS_OK)
    {
        iwdgRefresh(iwdg);
    }

    return status;
}

void iwdgRefresh(IwdgRegisters_t *iwdg)
{
    iwdg->KR = IWDG_KEY_RELOAD;
}

void iwdgFreezeInDebug(DbgmcuRegisters_t *dbgmcu)
{
    dbgmcu->APB1_FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
}

/** @} */
