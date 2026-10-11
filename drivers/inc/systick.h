/**
 * @file systick.h
 * @brief SysTick driver - periodic tick timer configuration for the
 *        Cortex-M4 core's SysTick (core/inc/systick_reg.h). No
 *        application-facing logic; consumed by the kernel for its
 *        scheduler tick.
 *
 * This driver only programs the timer. The SysTick_Handler exception
 * routine and any tick counter belong to the consumer (the kernel), not
 * here: startup already provides a weak SysTick_Handler alias that a
 * strong definition elsewhere overrides, and a counter the driver owned
 * would have no meaning outside the scheduler that reads it. The handler's
 * priority is set separately with nvic.h's nvicSetSystemHandlerPriority(),
 * which should be done before systickInit() enables the interrupt.
 *
 * SysTick counts down from LOAD to 0 and reloads, raising the SysTick
 * exception each wrap while CTRL.TICKINT is set. LOAD is 24 bits wide, so
 * the longest period is 2^24 core clocks; a LOAD of 0 never reloads (the
 * counter parks at 0), so the shortest useful period is 2 clocks. The
 * period in core clocks is LOAD + 1, hence the "- 1" when converting a
 * tick rate to a reload value.
 *
 * The timer is clocked from the processor clock (CTRL.CLKSOURCE = 1),
 * not the optional external reference clock, so the caller passes the
 * actual core frequency. CALIB is not used.
 *
 * Takes SysTick's register block as a parameter (SysTickRegisters_t *)
 * even though SysTick is a hardware singleton - see CONTRIBUTING.md's
 * error-handling contract section for why every driver in this project
 * does this regardless of instance count.
 */
#ifndef SYSTICK_H
#define SYSTICK_H

#include "driver_status.h"
#include "systick_reg.h"

#include <stdint.h>

/**
 * @addtogroup driver_layer
 * @{
 */

/**
 * @brief Configure SysTick to raise its exception @p tick_hz times per
 *        second from the processor clock, and start it.
 *
 * Stops the timer first, then programs LOAD = core_hz / tick_hz - 1,
 * clears the current value, and finally sets CLKSOURCE, TICKINT and
 * ENABLE together, so the timer only runs once it is fully configured and
 * a re-initialisation never reloads underneath a running count.
 *
 * The division truncates: unless @p core_hz is an exact multiple of
 * @p tick_hz the period is up to one core clock short per tick, a
 * relative error below 1 / (core_hz / tick_hz). Exact for the intended
 * 180 MHz / 1 kHz and 16 MHz / 1 kHz configurations.
 *
 * @param systick SysTick register block (e.g. SYSTICK).
 * @param core_hz Processor (HCLK) frequency in Hz.
 * @param tick_hz Desired tick rate in Hz.
 * @pre SysTick's exception priority has already been set with
 *      nvicSetSystemHandlerPriority(), and a SysTick_Handler exists - the
 *      first tick can arrive as soon as this returns.
 * @return DRIVER_STATUS_OK once the timer is running.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p core_hz or @p tick_hz is
 *         zero, or if the reload value core_hz / tick_hz - 1 is below 1
 *         (period under 2 clocks, including @p tick_hz above @p core_hz)
 *         or above SYSTICK_LOAD_RELOAD_Msk (period over 2^24 clocks). No
 *         register is written in that case.
 */
DRIVER_MUST_CHECK DriverStatus_e systickInit(SysTickRegisters_t *systick, uint32_t core_hz,
                                             uint32_t tick_hz);

/**
 * @brief Stop SysTick and restore CTRL, LOAD and VAL to their reset
 *        value of zero.
 *
 * A single deterministic register sequence with no failure path, so it
 * returns void rather than DriverStatus_e. Does not touch SysTick's
 * exception priority or any pending SysTick exception.
 *
 * @param systick SysTick register block (e.g. SYSTICK).
 */
void systickDeinit(SysTickRegisters_t *systick);

/** @} */

#endif /* SYSTICK_H */
