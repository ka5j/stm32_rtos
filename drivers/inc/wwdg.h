/**
 * @file wwdg.h
 * @brief WWDG driver: start, refresh and debug-freeze the window watchdog.
 *
 * The 7-bit counter CR.T counts down at PCLK1 / 4096 / 2^WDGTB and resets the
 * chip when it falls from 0x40 to 0x3F. A refresh also resets the chip if it
 * comes while the counter is still above the window CFR.W, so the WWDG
 * catches code that runs too fast as well as too slow. With PCLK1 at 45 MHz
 * and /8, one count is 728 us, and 0x7F to 0x3F is about 46 ms.
 *
 * The usual pattern refreshes from the early-wakeup interrupt, raised at
 * 0x40, which is inside the window as long as W is above 0x40:
 * @code
 * rccWwdgClockEnable(RCC);
 * wwdgFreezeInDebug(DBGMCU);
 * if (wwdgStart(WWDG, WWDG_CFR_WDGTB_DIV8, 0x5FU, 0x7FU) != DRIVER_STATUS_OK) { ... }
 * wwdgEnableEarlyWakeup(WWDG);
 * if (nvicEnableIrq(NVIC, WWDG_IRQn) != DRIVER_STATUS_OK) { ... }
 * // in WWDG_IRQHandler: wwdgClearEarlyWakeup(WWDG); then refresh with 0x7F
 * @endcode
 * After a reset, rccResetFlags() reports whether the WWDG caused it.
 */
#ifndef WWDG_H
#define WWDG_H

#include "dbgmcu_reg.h"
#include "driver_status.h"
#include "wwdg_reg.h"

#include <stdint.h>

/**
 * @addtogroup wwdg_driver
 * @{
 */

#define WWDG_COUNTER_MIN (0x40U) ///< Lowest counter or window value: 0x3F is the reset point
#define WWDG_COUNTER_MAX (0x7FU) ///< Highest counter or window value (7 bits)

/**
 * @brief Set the window and timer base, then start the watchdog counting from @p counter.
 *
 * Writes CFR, then CR with WDGA and @p counter in one store, so the window
 * and timer base are in place before the counter starts. CFR.EWI is kept.
 *
 * It then clears SR.EWIF. The counter runs from the moment its clock is
 * enabled, even before WDGA is set, and sets EWIF each time it passes 0x40,
 * enabled or not. Left set, that stale flag would raise the early wakeup at
 * once, above the window.
 *
 * @param wwdg      WWDG block (e.g. WWDG).
 * @param prescaler A `WWDG_CFR_WDGTB_DIV*` value.
 * @param window    0x40 to 0x7F. A refresh while the counter is above it resets the chip; 0x7F
 *                  allows a refresh at any time.
 * @param counter   0x40 to 0x7F, the start value. Timeout = (counter - 0x3F) counts.
 * @pre The WWDG clock is on (rccWwdgClockEnable()).
 * @retval DRIVER_STATUS_OK                Running.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM A parameter is out of range. Nothing written.
 * @warning Once started, only a reset stops it. Refresh it with wwdgRefresh()
 *          below the window and before the counter reaches 0x3F.
 */
DRIVER_MUST_CHECK DriverStatus_e wwdgStart(WwdgRegisters_t *wwdg, uint32_t prescaler,
                                           uint32_t window, uint32_t counter);

/**
 * @brief Reload the counter with @p counter (CR = WDGA | counter).
 *
 * A single store, so it is safe from any context.
 *
 * @param wwdg    WWDG block (e.g. WWDG).
 * @param counter 0x40 to 0x7F.
 * @retval DRIVER_STATUS_OK                Reloaded.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p counter is out of range. Nothing written.
 * @warning A refresh while the counter is above the window resets the chip at once.
 */
DRIVER_MUST_CHECK DriverStatus_e wwdgRefresh(WwdgRegisters_t *wwdg, uint32_t counter);

/**
 * @brief Raise `WWDG_IRQn` when the counter reaches 0x40, one count before reset.
 *
 * CFR.EWI can only be set; a reset clears it.
 *
 * @param wwdg WWDG block (e.g. WWDG).
 */
void wwdgEnableEarlyWakeup(WwdgRegisters_t *wwdg);

/**
 * @brief Clear the early-wakeup flag (SR.EWIF, write 0 to clear).
 *
 * Call it in `WWDG_IRQHandler`, or the interrupt is taken again at once.
 *
 * @param wwdg WWDG block (e.g. WWDG).
 */
void wwdgClearEarlyWakeup(WwdgRegisters_t *wwdg);

/**
 * @brief Stop the WWDG counter whenever the core is halted by a debugger.
 *
 * As iwdgFreezeInDebug(), for the WWDG's bit.
 *
 * @param dbgmcu DBGMCU block (e.g. DBGMCU).
 */
void wwdgFreezeInDebug(DbgmcuRegisters_t *dbgmcu);

/** @} */

#endif /* WWDG_H */
