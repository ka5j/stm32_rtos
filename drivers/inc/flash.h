/**
 * @file flash.h
 * @brief Flash interface driver - access-latency (wait-state) configuration
 *        for the STM32F446xx embedded flash (device/inc/flash_reg.h). No
 *        application-facing logic; consumed by rcc.c as part of a SYSCLK
 *        frequency change.
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

/**
 * @addtogroup driver_layer
 * @{
 */

/**
 * @brief Set the flash access latency (ACR.LATENCY) for the current SYSCLK
 *        frequency range.
 *
 * Must be raised to at least RM0390 Table 15's minimum for the target
 * SYSCLK/voltage-scale combination *before* rcc.c switches RCC_CFGR.SW to
 * a faster source - reading flash at too few wait states for the new
 * speed corrupts instruction fetch without a visible fault (see
 * flash_reg.h's file-level comment). It is always electrically safe to
 * run at a higher latency than the current SYSCLK strictly requires
 * (costs flash throughput, never correctness), which is why rcc.c applies
 * the target latency unconditionally before every switch, regardless of
 * whether SYSCLK is increasing or decreasing.
 *
 * A single deterministic register write: RM0390 documents no busy/ready
 * flag for this field, so there is nothing to poll and no timeout case.
 *
 * @param flash   Flash interface register block (e.g. FLASH).
 * @param latency Target value for ACR.LATENCY (0-15, the field's full
 *                width - RM0390 defines all 16 encodings as valid wait-
 *                state counts).
 * @return DRIVER_STATUS_OK once ACR.LATENCY has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if latency exceeds the 4-bit
 *         field (> 15).
 */
DRIVER_MUST_CHECK DriverStatus_e flashSetLatency(FlashRegisters_t *flash, uint32_t latency);

/** @} */

#endif /* FLASH_H */
