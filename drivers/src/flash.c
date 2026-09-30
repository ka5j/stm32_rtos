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

        /* RM0390 requires the programmed latency be read back and
         * confirmed before the clock change it is preparing for goes
         * ahead: the write is not guaranteed to have been taken into
         * account when the store retires. Proceeding on an unaccepted
         * latency is exactly the case that corrupts instruction fetch
         * silently once SYSCLK rises, so it is reported as a hardware
         * fault rather than assumed to have worked.
         *
         * The GCOVR_EXCL_START/STOP region below is deliberate, and is
         * the only coverage exclusion in this project - not a gap being
         * papered over. Every other bounded wait here is host-testable
         * precisely because the flag it polls is a field distinct from
         * what the driver writes (see the Makefile's TEST_DRIVER_SOURCES
         * comment), so a test can hold that flag clear in a plain
         * in-memory struct. A read-back check is the one shape that
         * breaks that property: it compares a register against the value
         * just written to it, and an in-memory struct always agrees by
         * construction. Exercising it needs real silicon that refuses
         * the write - see CONTRIBUTING.md's "Testing and its limits". */
        /* GCOVR_EXCL_START */
        if ((flash->ACR & FLASH_ACR_LATENCY_Msk) != (latency << FLASH_ACR_LATENCY_Pos))
        {
            status = DRIVER_STATUS_ERR_HW_FAULT;
        }
        /* GCOVR_EXCL_STOP */
    }

    return status;
}

/** @} */
