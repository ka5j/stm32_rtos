/**
 * @file rcc.h
 * @brief RCC peripheral driver - peripheral clock gating for the
 *        STM32F446xx (device/inc/rcc_reg.h). No application-facing logic;
 *        consumed by api/.
 *
 * Scoped to clock gating only: enabling/disabling the AHB1/APB1/APB2
 * peripheral clocks the GPIO, USART2, SYSCFG, and PWR blocks need before
 * their own registers become accessible - what api/ and drivers/inc/gpio.h
 * need right now. HSI/HSE-to-PLL SYSCLK bring-up is a separate,
 * not-yet-implemented part of this driver: it involves a blocking
 * hardware-ready poll (not host-testable off-target the way this file is -
 * see the Makefile's TEST_DRIVER_SOURCES comment) and flash-latency/PWR-
 * voltage-scale sequencing (flash_reg.h, pwr_reg.h) this project hasn't
 * tackled yet. See docs/VERSIONING.md's 0.2.0 entry.
 *
 * Takes RCC's register block as a parameter (RccRegisters_t *) even
 * though RCC is a hardware singleton, and identifies a GPIO port by its
 * existing GpioRegisters_t * (GPIOA, GPIOB, ...) rather than a new port
 * enum - see CONTRIBUTING.md's error-handling contract section for why
 * this project parameterizes every driver by its register block
 * regardless of instance count, instead of reaching for a global macro.
 */
#ifndef RCC_H
#define RCC_H

#include "driver_status.h"
#include "gpio_reg.h"
#include "rcc_reg.h"

/**
 * @addtogroup driver_layer
 * @{
 */

/**
 * @brief Enable a GPIO port's AHB1 peripheral clock.
 *
 * Must be called before any gpio.h function touches @p port's registers -
 * see gpio.h's gpioInit() @pre.
 *
 * @param rcc  RCC register block (e.g. RCC).
 * @param port GPIO port register block to enable (e.g. GPIOA). Identified
 *             by pointer identity against GPIOA..GPIOH (gpio_reg.h) -
 *             never dereferenced, only compared.
 * @return DRIVER_STATUS_OK on success.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if port is not one of
 *         GPIOA..GPIOH.
 */
DRIVER_MUST_CHECK DriverStatus_e rccGpioClockEnable(RccRegisters_t *rcc,
                                                    const GpioRegisters_t *port);

/**
 * @brief Disable a GPIO port's AHB1 peripheral clock.
 *
 * @param rcc  RCC register block (e.g. RCC).
 * @param port GPIO port register block to disable (e.g. GPIOA). Identified
 *             by pointer identity against GPIOA..GPIOH (gpio_reg.h) -
 *             never dereferenced, only compared.
 * @return DRIVER_STATUS_OK on success.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if port is not one of
 *         GPIOA..GPIOH.
 */
DRIVER_MUST_CHECK DriverStatus_e rccGpioClockDisable(RccRegisters_t *rcc,
                                                     const GpioRegisters_t *port);

/**
 * @brief Enable USART2's APB1 peripheral clock.
 * @param rcc RCC register block (e.g. RCC).
 */
void rccUsart2ClockEnable(RccRegisters_t *rcc);

/**
 * @brief Disable USART2's APB1 peripheral clock.
 * @param rcc RCC register block (e.g. RCC).
 */
void rccUsart2ClockDisable(RccRegisters_t *rcc);

/**
 * @brief Enable SYSCFG's APB2 peripheral clock.
 *
 * Must be called before any syscfg_reg.h register is accessed (e.g. to
 * route a GPIO pin to an EXTI line via EXTICR) - see syscfg_reg.h's
 * file-level comment.
 *
 * @param rcc RCC register block (e.g. RCC).
 */
void rccSyscfgClockEnable(RccRegisters_t *rcc);

/**
 * @brief Disable SYSCFG's APB2 peripheral clock.
 * @param rcc RCC register block (e.g. RCC).
 */
void rccSyscfgClockDisable(RccRegisters_t *rcc);

/**
 * @brief Enable PWR's APB1 peripheral clock.
 *
 * Must be called before any pwr_reg.h register is accessed - see
 * device/inc/rcc_reg.h's RCC_APB1ENR_PWREN comment.
 *
 * @param rcc RCC register block (e.g. RCC).
 */
void rccPwrClockEnable(RccRegisters_t *rcc);

/**
 * @brief Disable PWR's APB1 peripheral clock.
 * @param rcc RCC register block (e.g. RCC).
 */
void rccPwrClockDisable(RccRegisters_t *rcc);

/** @} */

#endif /* RCC_H */
