/**
 * @file exti.h
 * @brief EXTI driver: route a GPIO pin to an EXTI line, choose its edges, and
 *        manage its interrupt and pending bit.
 *
 * Lines 0-15 come from GPIO pin `n` of the port extiSelectPort() picks; lines
 * 16-22 are the internal sources in exti_reg.h. Line 19 has no source on the
 * F446. Lines 0-4 have their own NVIC vector; 5-9 share `EXTI9_5_IRQn` and
 * 10-15 share `EXTI15_10_IRQn`, so a shared handler checks each line with
 * extiIsPending(). The B1 button (PC13), falling edge:
 * @code
 * rccSyscfgClockEnable(RCC);
 * if (extiSelectPort(SYSCFG, 13U, GPIOC) != DRIVER_STATUS_OK) { ... }
 * if (extiConfigure(EXTI, 13U, EXTI_EDGE_FALLING) != DRIVER_STATUS_OK) { ... }
 * if (extiEnableInterrupt(EXTI, 13U) != DRIVER_STATUS_OK) { ... }
 * if (nvicEnableIrq(NVIC, EXTI15_10_IRQn) != DRIVER_STATUS_OK) { ... }
 * // in EXTI15_10_IRQHandler: extiClearPending(EXTI, 13U)
 * @endcode
 *
 * Event mode (EMR, for WFE) is not offered: nothing uses it yet.
 */
#ifndef EXTI_H
#define EXTI_H

#include "driver_status.h"
#include "exti_reg.h"
#include "gpio_reg.h"
#include "syscfg_reg.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @addtogroup exti_driver
 * @{
 */

#define EXTI_LINE_MAX (22U)      ///< Highest EXTI line on the F446
#define EXTI_GPIO_LINE_MAX (15U) ///< Highest line fed by a GPIO pin

/** @brief Which edges of a line's input set its pending bit. */
typedef enum ExtiEdge_e
{
    EXTI_EDGE_RISING = 1,  ///< Rising edge only (RTSR)
    EXTI_EDGE_FALLING = 2, ///< Falling edge only (FTSR)
    EXTI_EDGE_BOTH = 3,    ///< Both edges
} ExtiEdge_e;

/**
 * @brief Route GPIO pin @p line of @p port to EXTI line @p line.
 * @param syscfg SYSCFG block (e.g. SYSCFG).
 * @param line   0 to ::EXTI_GPIO_LINE_MAX.
 * @param port   GPIOA to GPIOH; compared by address, never dereferenced. GPIOH has only pins 0
 *               and 1.
 * @pre SYSCFG's clock is on (rccSyscfgClockEnable()).
 * @retval DRIVER_STATUS_OK                The line's EXTICR field selects @p port.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p line is above 15, @p port is not GPIOA to GPIOH, or
 *                                         it is GPIOH with @p line above 1. Nothing written.
 */
DRIVER_MUST_CHECK DriverStatus_e extiSelectPort(SyscfgRegisters_t *syscfg, uint32_t line,
                                                const GpioRegisters_t *port);

/**
 * @brief Set which edges trigger @p line; the other edge is cleared.
 *
 * A change of edge can itself be seen as an edge, so configure a line while
 * its interrupt is disabled and clear its pending bit before enabling it.
 *
 * @param exti EXTI block (e.g. EXTI).
 * @param line 0 to ::EXTI_LINE_MAX, except 19.
 * @param edge Edges to trigger on.
 * @retval DRIVER_STATUS_OK                RTSR and FTSR set for @p line.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p line or @p edge is invalid. Nothing written.
 */
DRIVER_MUST_CHECK DriverStatus_e extiConfigure(ExtiRegisters_t *exti, uint32_t line,
                                               ExtiEdge_e edge);

/**
 * @brief Unmask @p line's interrupt (IMR).
 * @param exti EXTI block (e.g. EXTI).
 * @param line 0 to ::EXTI_LINE_MAX, except 19.
 * @retval DRIVER_STATUS_OK                Unmasked.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p line is invalid. Nothing written.
 */
DRIVER_MUST_CHECK DriverStatus_e extiEnableInterrupt(ExtiRegisters_t *exti, uint32_t line);

/**
 * @brief Mask @p line's interrupt (IMR). Its pending bit is left alone.
 * @param exti EXTI block (e.g. EXTI).
 * @param line 0 to ::EXTI_LINE_MAX, except 19.
 * @retval DRIVER_STATUS_OK                Masked.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p line is invalid. Nothing written.
 */
DRIVER_MUST_CHECK DriverStatus_e extiDisableInterrupt(ExtiRegisters_t *exti, uint32_t line);

/**
 * @brief Raise @p line's interrupt from software (SWIER).
 *
 * Sets the pending bit only if the line's interrupt is unmasked; the edge
 * configuration does not matter. Hardware clears SWIER when the pending bit
 * is cleared.
 *
 * @param exti EXTI block (e.g. EXTI).
 * @param line 0 to ::EXTI_LINE_MAX, except 19.
 * @retval DRIVER_STATUS_OK                SWIER bit set.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p line is invalid. Nothing written.
 */
DRIVER_MUST_CHECK DriverStatus_e extiTriggerSoftware(ExtiRegisters_t *exti, uint32_t line);

/**
 * @brief Report whether @p line's pending bit (PR) is set.
 * @param exti    EXTI block (e.g. EXTI).
 * @param line    0 to ::EXTI_LINE_MAX, except 19.
 * @param pending Receives the bit; written only on success.
 * @retval DRIVER_STATUS_OK                @p pending written.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p line is invalid or @p pending is NULL.
 */
DRIVER_MUST_CHECK DriverStatus_e extiIsPending(const ExtiRegisters_t *exti, uint32_t line,
                                               bool *pending);

/**
 * @brief Clear @p line's pending bit, and its SWIER bit with it.
 *
 * PR is write-1-to-clear, so this is a single store of the line's bit: it
 * cannot clear another line's pending bit and needs no serialising.
 *
 * @param exti EXTI block (e.g. EXTI).
 * @param line 0 to ::EXTI_LINE_MAX, except 19.
 * @retval DRIVER_STATUS_OK                Cleared.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p line is invalid. Nothing written.
 * @warning Clear it in the handler before returning. Otherwise the interrupt is
 *          taken again at once.
 */
DRIVER_MUST_CHECK DriverStatus_e extiClearPending(ExtiRegisters_t *exti, uint32_t line);

/** @} */

#endif /* EXTI_H */
