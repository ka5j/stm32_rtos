/**
 * @file flash.h
 * @brief Flash driver: access latency (wait states), prefetch and caches.
 *
 * rccSysclkSwitch() sets the latency as part of a SYSCLK change; callers
 * rarely need flashSetLatency() directly.
 * @code
 * if (flashSetLatency(FLASH, 5U) != DRIVER_STATUS_OK) { ... } // 180 MHz at 2.7-3.6 V
 * flashEnableCaches(FLASH);
 * @endcode
 */
#ifndef FLASH_H
#define FLASH_H

#include "driver_status.h"
#include "flash_reg.h"

#include <stdint.h>

/**
 * @addtogroup flash_driver
 * @{
 */

/**
 * @brief Set the flash access latency and confirm it by reading it back.
 *
 * The latency must be at least RM0390 Table 5's value for the SYSCLK in
 * use. More wait states than needed are always safe; too few corrupt
 * instruction fetches with no fault. RM0390 has no ready flag for this
 * field, but asks for the read-back.
 *
 * @param flash   Flash interface block (e.g. FLASH).
 * @param latency Wait states, 0-15.
 * @retval DRIVER_STATUS_OK                Written and read back.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p latency is above 15.
 * @retval DRIVER_STATUS_ERR_HW_FAULT      The read-back differs: the interface did not accept it.
 * @see rccSysclkSwitch(), which raises latency before a switch and lowers it after.
 */
DRIVER_MUST_CHECK DriverStatus_e flashSetLatency(FlashRegisters_t *flash, uint32_t latency);

/**
 * @brief Turn on prefetch and the instruction and data caches.
 *
 * Other ACR fields, including LATENCY, are preserved. The caches are empty
 * out of reset, so no cache reset is done. Callable at any point in bring-up.
 *
 * @param flash Flash interface block (e.g. FLASH).
 */
void flashEnableCaches(FlashRegisters_t *flash);

/** @} */

#endif /* FLASH_H */
