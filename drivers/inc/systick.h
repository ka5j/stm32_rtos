/**
 * @file systick.h
 * @brief SysTick driver: the periodic tick timer.
 *
 * Programs the timer only: `SysTick_Handler` and any tick count belong to the
 * kernel. SysTick counts down from LOAD to 0 and reloads, so the period is
 * LOAD + 1 processor clocks: 2 at minimum and 2^24 at most.
 * @code
 * if (nvicSetSystemHandlerPriority(SCB, SysTick_IRQn, 15U) != DRIVER_STATUS_OK) { ... }
 * if (systickInit(SYSTICK, BSP_HCLK_HZ, 1000U) != DRIVER_STATUS_OK) { ... } // 1 kHz tick
 * @endcode
 */
#ifndef SYSTICK_H
#define SYSTICK_H

#include "driver_status.h"
#include "systick_reg.h"

#include <stdint.h>

/**
 * @addtogroup systick_driver
 * @{
 */

/**
 * @brief Start SysTick at @p tick_hz from the processor clock, with its interrupt on.
 *
 * Stops the timer, sets LOAD to `core_hz / tick_hz - 1`, clears VAL, then sets
 * CLKSOURCE, TICKINT and ENABLE together. The division truncates, so the
 * period is exact only when @p tick_hz divides @p core_hz, as 1 kHz does at
 * 16 and 180 MHz.
 *
 * @param systick SysTick block (e.g. SYSTICK).
 * @param core_hz Processor clock (HCLK), in Hz.
 * @param tick_hz Tick rate, in Hz.
 * @pre The SysTick priority is set (nvicSetSystemHandlerPriority()).
 * @retval DRIVER_STATUS_OK                Running.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM A frequency is 0, or the period is under 2 or over
 *                                         2^24 clocks. Nothing is written.
 * @warning The first tick can arrive as soon as this returns. Until the kernel
 *          defines `SysTick_Handler`, it lands in `Default_Handler` and hangs.
 */
DRIVER_MUST_CHECK DriverStatus_e systickInit(SysTickRegisters_t *systick, uint32_t core_hz,
                                             uint32_t tick_hz);

/**
 * @brief Stop SysTick and zero CTRL, LOAD and VAL.
 *
 * Leaves its priority and any pending SysTick exception alone.
 *
 * @param systick SysTick block (e.g. SYSTICK).
 */
void systickDeinit(SysTickRegisters_t *systick);

/** @} */

#endif /* SYSTICK_H */
