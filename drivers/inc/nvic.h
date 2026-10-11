/**
 * @file nvic.h
 * @brief NVIC driver - interrupt enable/pending/priority control for the
 *        device interrupts, and priority control for the Cortex-M4 system
 *        handlers, for the STM32F446RE (core/inc/nvic_reg.h,
 *        core/inc/scb_reg.h). No application-facing logic; consumed by the
 *        kernel and by any driver that needs an interrupt line.
 *
 * Two register banks are involved, and which one applies depends on the
 * sign of the IRQn_e value (see nvic_reg.h):
 *   - Device interrupts (IRQn >= 0) are controlled through the NVIC
 *     itself: ISER/ICER (enable), ISPR/ICPR (pending), IP (priority).
 *   - The configurable system exceptions (IRQn < 0: MemManage, BusFault,
 *     UsageFault, SVCall, PendSV, SysTick) have no NVIC enable or pending
 *     bits; their priority lives in SCB->SHPR1-3. They are handled by
 *     nvicSetSystemHandlerPriority(), not by the functions that take an
 *     NvicRegisters_t. NMI and HardFault have fixed priorities and are
 *     not configurable at all.
 *
 * Priority values are 0-15 with 0 the most urgent, in the hardware's
 * implemented-bits representation: this driver shifts them into the top
 * four bits of the priority byte (NVIC_PRIO_Pos) so callers never see the
 * unimplemented low bits. Startup (Reset_Handler) sets
 * SCB->AIRCR.PRIGROUP to 3 before any C code runs, which makes all four
 * bits preemption priority and none sub-priority; this driver relies on
 * that and deliberately offers no grouping setter, so the setting has a
 * single owner.
 *
 * A device IRQn is accepted if it is in 0 to FMPI2C1_Error_IRQn. IRQn_e
 * does not name the reserved positions inside that range (nvic_reg.h
 * lists them); writing the NVIC bit for one is architecturally harmless
 * (no vector exists for it to select), so they are not rejected.
 *
 * ISER/ICER/ISPR/ICPR are write-1-to-act: this driver writes only the
 * single bit it means to change and never reads them back, so enabling
 * one interrupt cannot disturb another.
 *
 * Takes the NVIC's and SCB's register blocks as parameters
 * (NvicRegisters_t *, ScbRegisters_t *) even though both are hardware
 * singletons - see CONTRIBUTING.md's error-handling contract section for
 * why every driver in this project does this regardless of instance
 * count.
 */
#ifndef NVIC_H
#define NVIC_H

#include "driver_status.h"
#include "nvic_reg.h"
#include "scb_reg.h"

#include <stdint.h>

/**
 * @addtogroup driver_layer
 * @{
 */

/** Highest priority value a caller may pass: NVIC_PRIO_BITS implemented
 *  bits, with 0 the most urgent. */
#define NVIC_PRIORITY_MAX ((1U << NVIC_PRIO_BITS) - 1U)

/**
 * @brief Enable a device interrupt in the NVIC (ISER).
 *
 * @param nvic NVIC register block (e.g. NVIC).
 * @param irqn Device interrupt number, 0 to ::FMPI2C1_Error_IRQn.
 * @return DRIVER_STATUS_OK once the enable bit has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p irqn is negative (a
 *         system exception, which the NVIC does not control) or above
 *         ::FMPI2C1_Error_IRQn.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicEnableIrq(NvicRegisters_t *nvic, IRQn_e irqn);

/**
 * @brief Disable a device interrupt in the NVIC (ICER).
 *
 * @param nvic NVIC register block (e.g. NVIC).
 * @param irqn Device interrupt number, 0 to ::FMPI2C1_Error_IRQn.
 * @return DRIVER_STATUS_OK once the disable bit has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p irqn is not a device
 *         interrupt in range, as for nvicEnableIrq().
 */
DRIVER_MUST_CHECK DriverStatus_e nvicDisableIrq(NvicRegisters_t *nvic, IRQn_e irqn);

/**
 * @brief Force a device interrupt into the pending state (ISPR).
 *
 * @param nvic NVIC register block (e.g. NVIC).
 * @param irqn Device interrupt number, 0 to ::FMPI2C1_Error_IRQn.
 * @return DRIVER_STATUS_OK once the pending bit has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p irqn is not a device
 *         interrupt in range, as for nvicEnableIrq().
 */
DRIVER_MUST_CHECK DriverStatus_e nvicSetPending(NvicRegisters_t *nvic, IRQn_e irqn);

/**
 * @brief Clear a device interrupt's pending state (ICPR).
 *
 * @param nvic NVIC register block (e.g. NVIC).
 * @param irqn Device interrupt number, 0 to ::FMPI2C1_Error_IRQn.
 * @return DRIVER_STATUS_OK once the clear-pending bit has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p irqn is not a device
 *         interrupt in range, as for nvicEnableIrq().
 */
DRIVER_MUST_CHECK DriverStatus_e nvicClearPending(NvicRegisters_t *nvic, IRQn_e irqn);

/**
 * @brief Set a device interrupt's priority (IP).
 *
 * @param nvic     NVIC register block (e.g. NVIC).
 * @param irqn     Device interrupt number, 0 to ::FMPI2C1_Error_IRQn.
 * @param priority 0 (most urgent) to ::NVIC_PRIORITY_MAX.
 * @return DRIVER_STATUS_OK once the priority byte has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p irqn is not a device
 *         interrupt in range (as for nvicEnableIrq()), or if @p priority
 *         exceeds ::NVIC_PRIORITY_MAX.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicSetPriority(NvicRegisters_t *nvic, IRQn_e irqn,
                                                 uint32_t priority);

/**
 * @brief Read back a device interrupt's priority (IP), in the same 0 to
 *        ::NVIC_PRIORITY_MAX representation nvicSetPriority() takes.
 *
 * @param nvic     NVIC register block (e.g. NVIC).
 * @param irqn     Device interrupt number, 0 to ::FMPI2C1_Error_IRQn.
 * @param priority Receives the priority; written only on success.
 * @return DRIVER_STATUS_OK once @p priority has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p irqn is not a device
 *         interrupt in range, as for nvicEnableIrq(), or if @p priority
 *         is NULL.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicGetPriority(const NvicRegisters_t *nvic, IRQn_e irqn,
                                                 uint32_t *priority);

/**
 * @brief Set the priority of a configurable system handler (SCB SHPR1-3).
 *
 * Read-modify-writes the one priority byte for @p irqn within its SHPR
 * word, leaving the other handlers sharing that word untouched - SHPR3
 * holds both PendSV and SysTick, for instance.
 *
 * @param scb      SCB register block (e.g. SCB).
 * @param irqn     One of ::MemoryManagement_IRQn, ::BusFault_IRQn,
 *                 ::UsageFault_IRQn, ::SVCall_IRQn, ::PendSV_IRQn,
 *                 ::SysTick_IRQn.
 * @param priority 0 (most urgent) to ::NVIC_PRIORITY_MAX.
 * @pre Call this for PendSV and SysTick before SysTick is enabled, so no
 *      tick can be taken at the reset-default priority; the shared SHPR3
 *      read-modify-write is not atomic against an exception that writes
 *      the same word.
 * @return DRIVER_STATUS_OK once the priority byte has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if @p irqn is not one of the
 *         six handlers above (this includes every device interrupt, NMI,
 *         HardFault and DebugMonitor), or if @p priority exceeds
 *         ::NVIC_PRIORITY_MAX.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicSetSystemHandlerPriority(ScbRegisters_t *scb, IRQn_e irqn,
                                                              uint32_t priority);

/** @} */

#endif /* NVIC_H */
