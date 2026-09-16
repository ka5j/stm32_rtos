/**
 * @file flash.c
 * @brief Flash interface driver implementation - see flash.h for the
 *        public API and flash_reg.h for the register definitions this
 *        operates on.
 */
#include "flash.h"

/**
 * @addtogroup driver_layer
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
    }

    return status;
}

/** @} */
