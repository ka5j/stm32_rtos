/**
 * @file flash.c
 * @brief Flash driver implementation (API in flash.h).
 */
#include "flash.h"

/**
 * @addtogroup flash_driver
 * @{
 */

DriverStatus_e flashSetLatency(FlashRegisters_t *flash, uint32_t latency)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if (latency > (FLASH_ACR_LATENCY_Msk >> FLASH_ACR_LATENCY_Pos))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else
    {
        flash->ACR = (flash->ACR & ~FLASH_ACR_LATENCY_Msk) | (latency << FLASH_ACR_LATENCY_Pos);

        /* RM0390 section 3.4.1 asks for this read-back before the clock rises.
         * It is the project's only coverage exclusion: an in-memory struct
         * always reads back what was written, so the failure branch needs
         * silicon that rejects the write (see docs/DESIGN.md, "Host testing
         * and its limits"). */
        /* GCOVR_EXCL_START */
        if ((flash->ACR & FLASH_ACR_LATENCY_Msk) != (latency << FLASH_ACR_LATENCY_Pos))
        {
            status = DRIVER_STATUS_ERR_HW_FAULT;
        }
        /* GCOVR_EXCL_STOP */
    }

    return status;
}

void flashEnableCaches(FlashRegisters_t *flash)
{
    flash->ACR |= FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
}

/** @} */
