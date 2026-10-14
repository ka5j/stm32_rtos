/**
 * @file flash.h
 * @brief Flash interface driver - access-latency (wait-state) and
 *        prefetch/cache configuration for the STM32F446xx embedded flash
 *        (device/inc/flash_reg.h). No application-facing logic; the
 *        latency half is consumed by rcc.c as part of a SYSCLK frequency
 *        change.
 *
 * Takes FLASH's register block as a parameter (FlashRegisters_t *) even
 * though the flash interface is a hardware singleton - see
 * CONTRIBUTING.md's error-handling contract section for why every driver
 * in this project does this regardless of instance count.
 */
#ifndef FLASH_H
#define FLASH_H

#include "driver_status.h"
#include "flash_reg.h"

#include <stdint.h>

/**
 * @addtogroup driver_layer
 * @{
 */

/**
 * @brief Set the flash access latency (ACR.LATENCY) for the current SYSCLK
 *        frequency range.
 *
 * Must be raised to at least RM0390 Table 5's (section 3.4.1) minimum for the target
 * SYSCLK/voltage-scale combination *before* rcc.c switches RCC_CFGR.SW to
 * a faster source - reading flash at too few wait states for the new
 * speed corrupts instruction fetch without a visible fault (see
 * flash_reg.h's file-level comment). It is always electrically safe to
 * run at a higher latency than the current SYSCLK strictly requires
 * (costs flash throughput, never correctness) - but that safety only
 * runs one direction. rcc.c's rccSysclkSwitch() therefore does *not*
 * apply a lower target latency this same way: it defers a lowering
 * change until after CFGR.SWS confirms SYSCLK has actually dropped to
 * the slower frequency the lower latency is rated for, and only applies
 * a raising change here, before the switch. See rccSysclkSwitch()'s own
 * doc comment for the two-sided sequencing this asymmetry requires.
 *
 * RM0390 documents no busy/ready flag for this field, so there is
 * nothing to poll and no timeout case. It does, however, require the
 * programmed value be read back and confirmed before relying on it, so
 * this function verifies the write landed rather than assuming it did.
 *
 * @param flash   Flash interface register block (e.g. FLASH).
 * @param latency Target value for ACR.LATENCY (0-15, the field's full
 *                width - RM0390 defines all 16 encodings as valid wait-
 *                state counts).
 * @return DRIVER_STATUS_OK once ACR.LATENCY has been written and read
 *         back as the value written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if latency exceeds the 4-bit
 *         field (> 15).
 * @return DRIVER_STATUS_ERR_HW_FAULT if ACR.LATENCY does not read back
 *         the programmed value - the flash interface did not accept it,
 *         and raising SYSCLK on that assumption would corrupt
 *         instruction fetch.
 */
DRIVER_MUST_CHECK DriverStatus_e flashSetLatency(FlashRegisters_t *flash, uint32_t latency);

/**
 * @brief Turn on the flash prefetch buffer and the instruction and data
 *        caches (ACR.PRFTEN, ICEN, DCEN).
 *
 * A single read-modify-write that leaves ACR.LATENCY, and anything else
 * in ACR, as it was. The caches come out of reset empty, so there is no
 * reset step: ACR.ICRST/DCRST are deliberately not touched here. They
 * are only meaningful to flush a cache that has been running, which
 * would be a caller that had enabled it before; none exists in this
 * project, and ST's own HAL_Init() also enables these three without a
 * reset.
 *
 * Has no failure path (a deterministic register write, nothing to poll),
 * so it returns void rather than DriverStatus_e. It is independent of
 * the wait-state count - flashSetLatency() still decides correctness at
 * a given SYSCLK; the caches only recover throughput lost to wait states
 * - so it can be called at any point in bring-up.
 *
 * @param flash Flash interface register block (e.g. FLASH).
 */
void flashEnableCaches(FlashRegisters_t *flash);

/** @} */

#endif /* FLASH_H */
