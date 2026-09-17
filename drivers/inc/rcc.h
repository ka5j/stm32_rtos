/**
 * @file rcc.h
 * @brief RCC peripheral driver - peripheral clock gating and HSI/HSE-to-PLL
 *        SYSCLK bring-up for the STM32F446xx (device/inc/rcc_reg.h). No
 *        application-facing logic; consumed by api/.
 *
 * Two halves:
 *   - Clock gating: enabling/disabling the AHB1/APB1/APB2 peripheral
 *     clocks the GPIO, USART2, SYSCFG, and PWR blocks need before their
 *     own registers become accessible.
 *   - SYSCLK bring-up: enabling HSI/HSE, configuring and enabling the
 *     PLL, setting the AHB/APB1/APB2 bus prescalers, and switching SYSCLK
 *     to the configured source via rccSysclkSwitch() - which also applies
 *     the flash access latency (flash.h) the new frequency requires, per
 *     RM0390 Table 15.
 *
 * The PWR voltage scale that same table pairs with each frequency range
 * is *not* sequenced from inside this driver, because the hardware
 * forbids it: RM0390 section 5.1.4 allows PWR_CR.VOS to be written only
 * while the PLL is off, and CSR.VOSRDY only reports the regulator
 * settled after the PLL has been switched on. Both of those fall outside
 * the window rccSysclkSwitch() runs in - by the time it is called the
 * PLL must already be locked. The caller therefore owns the full order:
 *
 *   1. rccPwrClockEnable(RCC)                  - PWR registers accessible
 *   2. pwrSetVoltageScale(PWR, scale)          - PLL still off
 *   3. rccHsiEnable(RCC) / rccHseEnable(RCC, bypass)
 *   4. rccPllConfig(RCC, source, m, n, p)
 *   5. rccPllEnable(RCC)                       - scale now takes effect
 *   6. pwrWaitVoltageScaleReady(PWR)           - regulator settled
 *   7. rccBusPrescalerConfig(RCC, hpre, ppre1, ppre2)
 *   8. rccSysclkSwitch(RCC, FLASH, source, latency)
 *
 * Steps 1, 2 and 6 are skippable only when running SYSCLK straight off
 * HSI or HSE with no PLL, where the regulator's reset default (scale 3)
 * already covers the frequency range.
 *
 * Every hardware-ready wait here (HSIRDY/HSERDY/PLLRDY/SWS/VOSRDY) is a
 * bounded iteration-count retry, not a wall-clock timeout: SysTick is not
 * configured this early in bring-up (its reload value depends on the
 * SYSCLK frequency this code is in the middle of establishing), the same
 * reasoning CMSIS's own system_stm32f4xx.c SetSysClock() uses for its
 * HSE/PLL ready-waits. This is what makes bring-up host-testable off-
 * target the same way clock gating already is: the ready bit (e.g.
 * CR.HSERDY) is a field distinct from what the driver itself writes
 * (CR.HSEON), so a test can hold it clear indefinitely in a plain
 * in-memory RccRegisters_t to exercise the timeout branch - see
 * tests/unit/test_rcc.c.
 *
 * Takes RCC's register block as a parameter (RccRegisters_t *) even
 * though RCC is a hardware singleton, and identifies a GPIO port by its
 * existing GpioRegisters_t * (GPIOA, GPIOB, ...) rather than a new port
 * enum - see CONTRIBUTING.md's error-handling contract section for why
 * this project parameterizes every driver by its register block
 * regardless of instance count.
 */
#ifndef RCC_H
#define RCC_H

#include "driver_status.h"
#include "flash_reg.h"
#include "gpio_reg.h"
#include "rcc_reg.h"

/**
 * @addtogroup driver_layer
 * @{
 */

/* ======================================================================
 * Peripheral clock gating
 * ==================================================================== */

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

/* ======================================================================
 * Oscillators (HSI, HSE)
 * ==================================================================== */

/**
 * @brief Enable the internal 16 MHz RC oscillator (HSI) and wait for it
 *        to stabilize.
 *
 * HSI is already running at reset (it is the boot clock); calling this
 * again is harmless and returns quickly once HSIRDY is already set.
 *
 * @param rcc RCC register block (e.g. RCC).
 * @return DRIVER_STATUS_OK once CR.HSIRDY is observed set.
 * @return DRIVER_STATUS_ERR_TIMEOUT if CR.HSIRDY never set within
 *         RCC_CLOCK_READY_TIMEOUT_ITERATIONS iterations - see this file's
 *         top comment on why this is a bounded retry, not a wall-clock
 *         timeout.
 */
DRIVER_MUST_CHECK DriverStatus_e rccHsiEnable(RccRegisters_t *rcc);

/**
 * @brief Disable the internal 16 MHz RC oscillator (HSI).
 *
 * @param rcc RCC register block (e.g. RCC).
 * @return DRIVER_STATUS_OK once CR.HSION is cleared.
 * @return DRIVER_STATUS_ERR_BUSY if CFGR.SWS currently reports HSI as the
 *         active SYSCLK source - disabling it would stop the running
 *         clock. Switch SYSCLK to a different, already-ready source via
 *         rccSysclkSwitch() first.
 */
DRIVER_MUST_CHECK DriverStatus_e rccHsiDisable(RccRegisters_t *rcc);

/**
 * @brief Enable the external oscillator (HSE) and wait for it to
 *        stabilize.
 *
 * @param rcc    RCC register block (e.g. RCC).
 * @param bypass CR.HSEBYP field value: ::RCC_CR_HSEBYP to bypass the
 *               oscillator with an external digital clock signal on
 *               OSC_IN (e.g. a board's MCO passthrough), or `0U` for a
 *               crystal/resonator on OSC_IN/OSC_OUT (RM0390's default
 *               mode). RM0390 only permits writing HSEBYP while
 *               CR.HSEON is clear, which this function's own BUSY guard
 *               below already ensures.
 * @return DRIVER_STATUS_OK once CR.HSERDY is observed set.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if bypass is not `0U` or
 *         ::RCC_CR_HSEBYP.
 * @return DRIVER_STATUS_ERR_BUSY if CR.HSEON is already set - HSEBYP
 *         cannot be safely reconfigured without disabling HSE first (see
 *         rccHseDisable()).
 * @return DRIVER_STATUS_ERR_TIMEOUT if CR.HSERDY never set within
 *         RCC_CLOCK_READY_TIMEOUT_ITERATIONS iterations.
 */
DRIVER_MUST_CHECK DriverStatus_e rccHseEnable(RccRegisters_t *rcc, uint32_t bypass);

/**
 * @brief Disable the external oscillator (HSE).
 *
 * @param rcc RCC register block (e.g. RCC).
 * @return DRIVER_STATUS_OK once CR.HSEON is cleared.
 * @return DRIVER_STATUS_ERR_BUSY if CFGR.SWS currently reports HSE as the
 *         active SYSCLK source. Does not check whether HSE also feeds an
 *         enabled PLL that is itself the active SYSCLK source - disabling
 *         HSE in that configuration stops the PLL and hangs the system;
 *         switch SYSCLK away from the PLL first if HSE is its source.
 */
DRIVER_MUST_CHECK DriverStatus_e rccHseDisable(RccRegisters_t *rcc);

/* ======================================================================
 * PLL
 * ==================================================================== */

/**
 * @brief Configure the main PLL's input source and M/N/P dividers.
 *
 * RM0390 forbids writing PLLCFGR while the PLL is enabled - this
 * function's own BUSY guard enforces that. Does not configure PLLQ
 * (USB OTG FS/SDIO 48 MHz output) - see rcc_reg.h's file-level comment
 * on why this project omits it.
 *
 * @param rcc    RCC register block (e.g. RCC).
 * @param source PLLCFGR.PLLSRC field value: ::RCC_PLLCFGR_PLLSRC_HSI or
 *               ::RCC_PLLCFGR_PLLSRC_HSE. The selected source must
 *               already be enabled and ready (rccHsiEnable()/
 *               rccHseEnable()) before rccPllEnable() is called, though
 *               this function itself does not check that - only
 *               rccPllEnable()'s ready-wait can observe it.
 * @param m      PLLM divider (PLLCFGR bits 5:0): VCO input = source / m.
 *               Valid range 2-63 per RM0390 (0 and 1 are reserved).
 * @param n      PLLN multiplier (PLLCFGR bits 14:6): VCO output =
 *               (source / m) * n. Valid range 50-432 per RM0390.
 * @param p      PLLP field value (PLLCFGR bits 17:16): one of
 *               ::RCC_PLLCFGR_PLLP_DIV2, ::RCC_PLLCFGR_PLLP_DIV4,
 *               ::RCC_PLLCFGR_PLLP_DIV6, ::RCC_PLLCFGR_PLLP_DIV8. SYSCLK
 *               (if the PLL is selected) = VCO output / this divisor.
 * @return DRIVER_STATUS_OK once PLLCFGR has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if source, m, n, or p is
 *         outside its documented range.
 * @return DRIVER_STATUS_ERR_BUSY if CR.PLLON is already set.
 */
DRIVER_MUST_CHECK DriverStatus_e rccPllConfig(RccRegisters_t *rcc, uint32_t source, uint32_t m,
                                              uint32_t n, uint32_t p);

/**
 * @brief Enable the main PLL and wait for it to lock.
 *
 * @param rcc RCC register block (e.g. RCC).
 * @pre rccPllConfig() has already configured PLLCFGR, and its selected
 *      source is already enabled and ready.
 * @return DRIVER_STATUS_OK once CR.PLLRDY is observed set.
 * @return DRIVER_STATUS_ERR_TIMEOUT if CR.PLLRDY never set within
 *         RCC_CLOCK_READY_TIMEOUT_ITERATIONS iterations - typically means
 *         PLLCFGR's source was never actually made ready before this call.
 */
DRIVER_MUST_CHECK DriverStatus_e rccPllEnable(RccRegisters_t *rcc);

/**
 * @brief Disable the main PLL.
 *
 * @param rcc RCC register block (e.g. RCC).
 * @return DRIVER_STATUS_OK once CR.PLLON is cleared.
 * @return DRIVER_STATUS_ERR_BUSY if CFGR.SWS currently reports the PLL as
 *         the active SYSCLK source - disabling it would stop the running
 *         clock. Switch SYSCLK to a different, already-ready source via
 *         rccSysclkSwitch() first.
 */
DRIVER_MUST_CHECK DriverStatus_e rccPllDisable(RccRegisters_t *rcc);

/* ======================================================================
 * Bus prescalers and SYSCLK switch
 * ==================================================================== */

/**
 * @brief Configure the AHB, APB1, and APB2 bus prescalers.
 *
 * Takes effect immediately and synchronously - RM0390 documents no
 * ready/busy flag for these fields, so there is nothing to poll. Get this
 * right before switching to a fast SYSCLK source: APB1's peripherals are
 * rated to 45 MHz max and APB2's to 90 MHz max on the F446, regardless of
 * SYSCLK.
 *
 * @param rcc   RCC register block (e.g. RCC).
 * @param hpre  CFGR.HPRE field value - one of the 9 ::RCC_CFGR_HPRE_DIV1
 *              .. ::RCC_CFGR_HPRE_DIV512 macros (device/inc/rcc_reg.h;
 *              not every 4-bit code is a distinct, documented divisor).
 * @param ppre1 CFGR.PPRE1 field value - one of the 5
 *              ::RCC_CFGR_PPRE_DIV1 .. ::RCC_CFGR_PPRE_DIV16 macros.
 * @param ppre2 CFGR.PPRE2 field value - one of the same 5
 *              ::RCC_CFGR_PPRE_DIV1 .. ::RCC_CFGR_PPRE_DIV16 macros.
 * @return DRIVER_STATUS_OK once CFGR has been written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if hpre, ppre1, or ppre2 is not
 *         one of its documented values.
 */
DRIVER_MUST_CHECK DriverStatus_e rccBusPrescalerConfig(RccRegisters_t *rcc, uint32_t hpre,
                                                       uint32_t ppre1, uint32_t ppre2);

/**
 * @brief Switch SYSCLK to the given source, applying the flash access
 *        latency the new frequency requires first.
 *
 * Always applies @p latency before touching CFGR.SW, regardless of
 * whether the switch raises or lowers SYSCLK - see flash.h's
 * flashSetLatency() for why this unconditional ordering is safe in both
 * directions. Confirms the requested source is actually ready before
 * switching (a caller that forgot to call rccHsiEnable()/rccHseEnable()/
 * rccPllEnable() first gets a clear DRIVER_STATUS_ERR_NOT_INITIALIZED
 * here rather than a much harder to diagnose failure from the CFGR.SWS
 * poll below).
 *
 * The PWR voltage scale is deliberately not this function's concern -
 * see this file's top comment for the hardware ordering constraint that
 * puts pwrSetVoltageScale() and pwrWaitVoltageScaleReady() on either
 * side of rccPllEnable(), both before this call.
 *
 * @param rcc     RCC register block (e.g. RCC).
 * @param flash   Flash interface register block (e.g. FLASH).
 * @param source  CFGR.SW field value: ::RCC_CFGR_SYSCLK_HSI,
 *                ::RCC_CFGR_SYSCLK_HSE, or ::RCC_CFGR_SYSCLK_PLL.
 * @param latency Target flash access latency - see flashSetLatency().
 * @pre For a PLL source, the full bring-up order in this file's top
 *      comment has already run - in particular the voltage scale is
 *      selected and settled, since this function requires the PLL to be
 *      locked by the time it is called and the scale can no longer be
 *      changed at that point.
 * @return DRIVER_STATUS_OK once CFGR.SWS confirms the switch completed.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if source is not one of the
 *         three documented values, or propagated from flashSetLatency()
 *         if latency is invalid.
 * @return DRIVER_STATUS_ERR_HW_FAULT propagated from flashSetLatency() if
 *         ACR.LATENCY does not read back the value written.
 * @return DRIVER_STATUS_ERR_TIMEOUT if CFGR.SWS never reports @p source
 *         within RCC_CLOCK_READY_TIMEOUT_ITERATIONS iterations after the
 *         switch.
 * @return DRIVER_STATUS_ERR_NOT_INITIALIZED if @p source's ready bit
 *         (HSIRDY/HSERDY/PLLRDY) is not set - it was never enabled, or
 *         never finished becoming ready.
 */
DRIVER_MUST_CHECK DriverStatus_e rccSysclkSwitch(RccRegisters_t *rcc, FlashRegisters_t *flash,
                                                 uint32_t source, uint32_t latency);

/** @} */

#endif /* RCC_H */
