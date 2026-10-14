/**
 * @file iwdg.h
 * @brief IWDG driver: start, refresh and debug-freeze the independent watchdog.
 *
 * The IWDG counts down from RLR at the LSI clock divided by the prescaler,
 * and resets the chip at 0. Once started it cannot be stopped, except by a
 * reset. The LSI is nominally 32 kHz but ranges from 17 to 47 kHz (DS10693),
 * so allow for that when choosing a timeout: here about 100 ms nominal,
 * 69-190 ms worst case.
 * @code
 * iwdgFreezeInDebug(DBGMCU); // a breakpoint must not reset the chip
 * if (iwdgStart(IWDG, IWDG_PR_DIV32, 100U) != DRIVER_STATUS_OK) { ... }
 * for (;;) { ...; iwdgRefresh(IWDG); }
 * @endcode
 * After a reset, rccResetFlags() reports whether the IWDG caused it.
 */
#ifndef IWDG_H
#define IWDG_H

#include "dbgmcu_reg.h"
#include "driver_status.h"
#include "iwdg_reg.h"

#include <stdint.h>

/**
 * @addtogroup iwdg_driver
 * @{
 */

/**
 * @brief Start the watchdog, then set its prescaler and reload value.
 *
 * Starting comes first because PR and RLR only take a new value while the
 * LSI runs, and starting the IWDG is what turns the LSI on. The sequence is
 * KR = start, KR = unlock, write PR and RLR, wait for SR.PVU and SR.RVU to
 * clear, then refresh, so the new reload value is in the counter on return.
 * Until then it runs from the previous values (/4 and 0xFFF after reset:
 * about 0.5 s). Can be called again to change the timeout.
 *
 * @param iwdg      IWDG block (e.g. IWDG).
 * @param prescaler An `IWDG_PR_DIV*` value.
 * @param reload    Counter reload, 0 to 0xFFF. Timeout = (reload + 1) x divider / f_LSI.
 * @retval DRIVER_STATUS_OK                Running with the new values, just refreshed.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p prescaler or @p reload is out of range. Nothing
 *                                         written; a running watchdog keeps running.
 * @retval DRIVER_STATUS_ERR_TIMEOUT       SR.PVU or SR.RVU did not clear. The watchdog is
 *                                         running, and its values are not known.
 * @warning Once started the watchdog cannot be stopped. Refresh it with
 *          iwdgRefresh() before every timeout, or the chip resets.
 */
DRIVER_MUST_CHECK DriverStatus_e iwdgStart(IwdgRegisters_t *iwdg, uint32_t prescaler,
                                           uint32_t reload);

/**
 * @brief Reload the counter from RLR (KR = 0xAAAA).
 *
 * A single store, so it is safe from any context.
 *
 * @param iwdg IWDG block (e.g. IWDG).
 */
void iwdgRefresh(IwdgRegisters_t *iwdg);

/**
 * @brief Stop the IWDG counter whenever the core is halted by a debugger.
 *
 * Without this, a breakpoint held for longer than the timeout resets the
 * chip. It has no effect when no debugger halts the core. The bit is reset by
 * power-on only, so it survives a watchdog reset. OpenOCD's stm32f4x target
 * script also sets it when it attaches; this call does not depend on the
 * debug tool.
 *
 * @param dbgmcu DBGMCU block (e.g. DBGMCU).
 */
void iwdgFreezeInDebug(DbgmcuRegisters_t *dbgmcu);

/** @} */

#endif /* IWDG_H */
