/**
 * @file nvic.h
 * @brief NVIC driver: device interrupt enable, pending and priority, and
 *        system-handler priority.
 *
 * - **Device interrupts** (::IRQn_e >= 0) use the NVIC: ISER/ICER, ISPR/ICPR and
 *   IP. 0 to ::FMPI2C1_Error_IRQn is accepted, except the reserved positions
 *   (61-62, 79-80, 82-83, 85-86, 88-90), which have no vector.
 * - **System handlers** (MemManage, BusFault, UsageFault, SVCall, PendSV,
 *   SysTick) have only a priority, in SCB SHPR1-3: use
 *   nvicSetSystemHandlerPriority(). NMI and HardFault are fixed.
 * - **Priorities** run from 0 (most urgent) to ::NVIC_PRIORITY_MAX.
 *   Reset_Handler sets PRIGROUP = 3, so all four bits are preemption priority;
 *   there is deliberately no grouping setter.
 *
 * @code
 * if (nvicSetPriority(NVIC, USART2_IRQn, 5U) != DRIVER_STATUS_OK) { ... }
 * if (nvicEnableIrq(NVIC, USART2_IRQn) != DRIVER_STATUS_OK) { ... }
 * @endcode
 */
#ifndef NVIC_H
#define NVIC_H

#include "driver_status.h"
#include "nvic_reg.h"
#include "scb_reg.h"

#include <stdint.h>

/**
 * @addtogroup nvic_driver
 * @{
 */

/** Lowest-urgency priority value: 15, from ::NVIC_PRIO_BITS implemented bits. */
#define NVIC_PRIORITY_MAX ((1U << NVIC_PRIO_BITS) - 1U)

/**
 * @brief Enable a device interrupt.
 *
 * Writes only its ISER bit, between compiler barriers, so state set up before
 * the call is not reordered after the enable.
 *
 * @param nvic NVIC block (e.g. NVIC).
 * @param irqn Device interrupt.
 * @retval DRIVER_STATUS_OK                Enabled.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p irqn is negative, above ::FMPI2C1_Error_IRQn
 *                                         or reserved.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicEnableIrq(NvicRegisters_t *nvic, IRQn_e irqn);

/**
 * @brief Disable a device interrupt.
 *
 * Writes its ICER bit, then DSB and ISB. Once this returns the interrupt is
 * not taken, even if it was already pending.
 *
 * @param nvic NVIC block (e.g. NVIC).
 * @param irqn Device interrupt.
 * @retval DRIVER_STATUS_OK                Disabled.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p irqn is not a valid device interrupt.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicDisableIrq(NvicRegisters_t *nvic, IRQn_e irqn);

/**
 * @brief Make a device interrupt pending (ISPR).
 * @param nvic NVIC block (e.g. NVIC).
 * @param irqn Device interrupt.
 * @retval DRIVER_STATUS_OK                Pending.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p irqn is not a valid device interrupt.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicSetPending(NvicRegisters_t *nvic, IRQn_e irqn);

/**
 * @brief Clear a device interrupt's pending state (ICPR).
 * @param nvic NVIC block (e.g. NVIC).
 * @param irqn Device interrupt.
 * @retval DRIVER_STATUS_OK                Cleared.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p irqn is not a valid device interrupt.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicClearPending(NvicRegisters_t *nvic, IRQn_e irqn);

/**
 * @brief Set a device interrupt's priority.
 * @param nvic     NVIC block (e.g. NVIC).
 * @param irqn     Device interrupt.
 * @param priority 0 (most urgent) to ::NVIC_PRIORITY_MAX.
 * @retval DRIVER_STATUS_OK                Set.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p irqn is not valid, or @p priority is too large.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicSetPriority(NvicRegisters_t *nvic, IRQn_e irqn,
                                                 uint32_t priority);

/**
 * @brief Read a device interrupt's priority, as nvicSetPriority() takes it.
 * @param nvic     NVIC block (e.g. NVIC).
 * @param irqn     Device interrupt.
 * @param priority Receives 0 to ::NVIC_PRIORITY_MAX; written only on success.
 * @retval DRIVER_STATUS_OK                Read.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p irqn is not valid, or @p priority is NULL.
 */
DRIVER_MUST_CHECK DriverStatus_e nvicGetPriority(const NvicRegisters_t *nvic, IRQn_e irqn,
                                                 uint32_t *priority);

/**
 * @brief Set a configurable system handler's priority (SCB SHPR1-3).
 *
 * Changes only that handler's byte; SHPR3 is shared by PendSV and SysTick.
 *
 * @param scb      SCB block (e.g. SCB).
 * @param irqn     ::MemoryManagement_IRQn, ::BusFault_IRQn, ::UsageFault_IRQn, ::SVCall_IRQn,
 *                 ::PendSV_IRQn or ::SysTick_IRQn.
 * @param priority 0 (most urgent) to ::NVIC_PRIORITY_MAX.
 * @pre For PendSV and SysTick: called before SysTick is enabled, so no tick runs at the
 *      default priority.
 * @retval DRIVER_STATUS_OK                Set.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p irqn is not one of the six handlers, or
 *                                         @p priority is too large.
 * @warning The SHPR read-modify-write is not atomic; see [Concurrency](DESIGN.md#concurrency).
 */
DRIVER_MUST_CHECK DriverStatus_e nvicSetSystemHandlerPriority(ScbRegisters_t *scb, IRQn_e irqn,
                                                              uint32_t priority);

/** @} */

#endif /* NVIC_H */
