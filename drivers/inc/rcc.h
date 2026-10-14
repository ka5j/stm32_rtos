/**
 * @file rcc.h
 * @brief RCC driver: peripheral clock gating and HSI/HSE-to-PLL SYSCLK bring-up.
 *
 * Bring-up to a PLL clock must follow this order, which RM0390 imposes (see
 * [Clock bring-up order](DESIGN.md#clock-bring-up-order)); bspClockInit() is
 * the worked example:
 *
 * 1. rccPwrClockEnable()
 * 2. pwrSetVoltageScale(): the PLL must still be off
 * 3. rccHseEnable() or rccHsiEnable()
 * 4. rccPllConfig()
 * 5. rccPllEnable()
 * 6. pwrWaitVoltageScaleReady()
 * 7. pwrEnableOverdrive(): only above the scale's normal maximum (180 MHz)
 * 8. rccBusPrescalerConfig()
 * 9. rccSysclkSwitch(): also sets the flash latency
 *
 * Steps 2 and 3 can swap. Running directly from HSI or HSE needs only steps 3
 * and 9, plus 8 if the buses must be divided:
 * @code
 * if (rccHseEnable(RCC, RCC_CR_HSEBYP) != DRIVER_STATUS_OK) { ... }
 * if (rccSysclkSwitch(RCC, FLASH, RCC_CFGR_SYSCLK_HSE, 0U) != DRIVER_STATUS_OK) { ... } // 8 MHz
 * @endcode
 *
 * Every wait is a bounded poll count, not a time (see
 * [Poll-count timeouts](DESIGN.md#poll-count-timeouts)). Each enable reads the
 * enable register back, so the clock is running before the caller's first
 * access to the peripheral.
 */
#ifndef RCC_H
#define RCC_H

#include "driver_status.h"
#include "flash_reg.h"
#include "gpio_reg.h"
#include "rcc_reg.h"

#include <stdint.h>

/**
 * @addtogroup rcc_driver
 * @{
 */

/** @name Peripheral clock gating
 *  @{ */

/**
 * @brief Enable a GPIO port's clock.
 * @param rcc  RCC block (e.g. RCC).
 * @param port Port to enable, GPIOA to GPIOH; compared by address, never dereferenced.
 * @retval DRIVER_STATUS_OK                Enabled.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p port is not GPIOA to GPIOH.
 */
DRIVER_MUST_CHECK DriverStatus_e rccGpioClockEnable(RccRegisters_t *rcc,
                                                    const GpioRegisters_t *port);

/**
 * @brief Disable a GPIO port's clock.
 * @param rcc  RCC block (e.g. RCC).
 * @param port Port to disable, GPIOA to GPIOH.
 * @retval DRIVER_STATUS_OK                Disabled.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p port is not GPIOA to GPIOH.
 */
DRIVER_MUST_CHECK DriverStatus_e rccGpioClockDisable(RccRegisters_t *rcc,
                                                     const GpioRegisters_t *port);

/** @brief Enable USART2's clock.
 *  @param rcc RCC block (e.g. RCC). */
void rccUsart2ClockEnable(RccRegisters_t *rcc);

/** @brief Disable USART2's clock.
 *  @param rcc RCC block (e.g. RCC). */
void rccUsart2ClockDisable(RccRegisters_t *rcc);

/** @brief Enable the WWDG's clock; needed before any WWDG access.
 *  @param rcc RCC block (e.g. RCC). */
void rccWwdgClockEnable(RccRegisters_t *rcc);

/** @brief Disable the WWDG's clock. Once the WWDG is started, only a reset stops it.
 *  @param rcc RCC block (e.g. RCC). */
void rccWwdgClockDisable(RccRegisters_t *rcc);

/** @brief Enable SYSCFG's clock; needed before any SYSCFG access (e.g. EXTI routing).
 *  @param rcc RCC block (e.g. RCC). */
void rccSyscfgClockEnable(RccRegisters_t *rcc);

/** @brief Disable SYSCFG's clock.
 *  @param rcc RCC block (e.g. RCC). */
void rccSyscfgClockDisable(RccRegisters_t *rcc);

/** @brief Enable PWR's clock; needed before any PWR access.
 *  @param rcc RCC block (e.g. RCC). */
void rccPwrClockEnable(RccRegisters_t *rcc);

/** @brief Disable PWR's clock.
 *  @param rcc RCC block (e.g. RCC). */
void rccPwrClockDisable(RccRegisters_t *rcc);

/** @} */

/** @name Reset cause
 *  @{ */

/** Every CSR reset-cause flag; rccResetFlags() returns a subset of these. */
#define RCC_RESET_FLAGS_Msk                                                                        \
    (RCC_CSR_BORRSTF | RCC_CSR_PINRSTF | RCC_CSR_PORRSTF | RCC_CSR_SFTRSTF | RCC_CSR_IWDGRSTF      \
     | RCC_CSR_WWDGRSTF | RCC_CSR_LPWRRSTF)

/**
 * @brief The reset-cause flags set since they were last cleared.
 *
 * Several can be set at once: a power-on reset also sets PINRSTF and
 * BORRSTF, and every internal reset also drives the NRST pin, so PINRSTF
 * accompanies IWDGRSTF, WWDGRSTF and SFTRSTF. Test the specific flag.
 *
 * @param rcc RCC block (e.g. RCC).
 * @return CSR masked to ::RCC_RESET_FLAGS_Msk, such as ::RCC_CSR_IWDGRSTF.
 */
uint32_t rccResetFlags(const RccRegisters_t *rcc);

/**
 * @brief Clear every reset-cause flag (CSR.RMVF).
 *
 * Read them first with rccResetFlags(): they accumulate across resets until
 * cleared.
 *
 * @param rcc RCC block (e.g. RCC).
 */
void rccClearResetFlags(RccRegisters_t *rcc);

/** @} */

/** @name Oscillators
 *  @{ */

/**
 * @brief Turn on the 16 MHz internal oscillator (HSI) and wait until ready.
 *
 * HSI is the reset clock, so this normally returns at once.
 *
 * @param rcc RCC block (e.g. RCC).
 * @retval DRIVER_STATUS_OK          HSIRDY is set.
 * @retval DRIVER_STATUS_ERR_TIMEOUT HSIRDY did not set; HSION stays set.
 */
DRIVER_MUST_CHECK DriverStatus_e rccHsiEnable(RccRegisters_t *rcc);

/**
 * @brief Turn off HSI and wait until HSIRDY clears.
 * @param rcc RCC block (e.g. RCC).
 * @retval DRIVER_STATUS_OK          HSI is off.
 * @retval DRIVER_STATUS_ERR_BUSY    HSI feeds SYSCLK, directly or through the PLL; nothing
 *                                   written.
 * @retval DRIVER_STATUS_ERR_TIMEOUT HSIRDY stayed set; HSION is cleared.
 */
DRIVER_MUST_CHECK DriverStatus_e rccHsiDisable(RccRegisters_t *rcc);

/**
 * @brief Turn on the external oscillator (HSE) and wait until ready.
 * @param rcc    RCC block (e.g. RCC).
 * @param bypass ::RCC_CR_HSEBYP for an external clock on OSC_IN (the NUCLEO's
 *               ST-LINK MCO), or 0 for a crystal.
 * @retval DRIVER_STATUS_OK                HSERDY is set.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p bypass is neither value.
 * @retval DRIVER_STATUS_ERR_BUSY          HSE is already on; call rccHseDisable() first.
 * @retval DRIVER_STATUS_ERR_TIMEOUT       HSERDY did not set; HSEON stays set.
 */
DRIVER_MUST_CHECK DriverStatus_e rccHseEnable(RccRegisters_t *rcc, uint32_t bypass);

/**
 * @brief Turn off HSE and wait until HSERDY clears.
 * @param rcc RCC block (e.g. RCC).
 * @retval DRIVER_STATUS_OK          HSE is off.
 * @retval DRIVER_STATUS_ERR_BUSY    HSE feeds SYSCLK, directly or through the PLL; nothing
 *                                   written.
 * @retval DRIVER_STATUS_ERR_TIMEOUT HSERDY stayed set; HSEON is cleared.
 * @warning A PLL that is on but not feeding SYSCLK loses its input. Disable it first.
 */
DRIVER_MUST_CHECK DriverStatus_e rccHseDisable(RccRegisters_t *rcc);

/** @} */

/** @name Main PLL
 *  @{ */

/**
 * @brief Set the PLL's input source and M, N and P.
 *
 * Checks the register ranges, not the VCO frequency limits. PLLQ is left
 * unchanged.
 *
 * @param rcc    RCC block (e.g. RCC).
 * @param source ::RCC_PLLCFGR_PLLSRC_HSI or ::RCC_PLLCFGR_PLLSRC_HSE.
 * @param m      Input divider, 2-63. VCO input = source / m, must be 1-2 MHz.
 * @param n      Multiplier, 50-432. VCO output = VCO input x n, must be 100-432 MHz.
 * @param p      A `RCC_PLLCFGR_PLLP_DIV*` value. SYSCLK = VCO output / p.
 * @retval DRIVER_STATUS_OK                PLLCFGR written.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM A parameter is out of range.
 * @retval DRIVER_STATUS_ERR_BUSY          The PLL is on; PLLCFGR is writable only while it is off.
 */
DRIVER_MUST_CHECK DriverStatus_e rccPllConfig(RccRegisters_t *rcc, uint32_t source, uint32_t m,
                                              uint32_t n, uint32_t p);

/**
 * @brief Turn on the PLL and wait for lock.
 * @param rcc RCC block (e.g. RCC).
 * @pre rccPllConfig() has run and its source oscillator is ready.
 * @retval DRIVER_STATUS_OK          PLLRDY is set.
 * @retval DRIVER_STATUS_ERR_TIMEOUT PLLRDY did not set, usually because the source is not
 *                                   ready. PLLON stays set; call rccPllDisable() to retry.
 */
DRIVER_MUST_CHECK DriverStatus_e rccPllEnable(RccRegisters_t *rcc);

/**
 * @brief Turn off the PLL and wait until PLLRDY clears.
 * @param rcc RCC block (e.g. RCC).
 * @retval DRIVER_STATUS_OK          The PLL is off.
 * @retval DRIVER_STATUS_ERR_BUSY    The PLL is the SYSCLK source; nothing written.
 * @retval DRIVER_STATUS_ERR_TIMEOUT PLLRDY stayed set; PLLON is cleared.
 */
DRIVER_MUST_CHECK DriverStatus_e rccPllDisable(RccRegisters_t *rcc);

/** @} */

/** @name Bus prescalers and SYSCLK
 *  @{ */

/**
 * @brief Set the AHB, APB1 and APB2 prescalers.
 *
 * The new division takes effect 1 to 16 AHB cycles after the write. Does not
 * check the bus maximums: APB1 at most 45 MHz, APB2 at most 90 MHz.
 *
 * @param rcc   RCC block (e.g. RCC).
 * @param hpre  A `RCC_CFGR_HPRE_DIV*` value.
 * @param ppre1 A `RCC_CFGR_PPRE_DIV*` value for APB1.
 * @param ppre2 A `RCC_CFGR_PPRE_DIV*` value for APB2.
 * @retval DRIVER_STATUS_OK                CFGR written.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM A value is not one of its macros.
 */
DRIVER_MUST_CHECK DriverStatus_e rccBusPrescalerConfig(RccRegisters_t *rcc, uint32_t hpre,
                                                       uint32_t ppre1, uint32_t ppre2);

/**
 * @brief Switch SYSCLK to a ready source and set the flash latency it needs.
 *
 * A higher latency is written before the switch; a lower one only after
 * CFGR.SWS confirms the switch, so flash is never read with too few wait
 * states.
 *
 * @param rcc     RCC block (e.g. RCC).
 * @param flash   Flash interface block (e.g. FLASH).
 * @param source  ::RCC_CFGR_SYSCLK_HSI, ::RCC_CFGR_SYSCLK_HSE or ::RCC_CFGR_SYSCLK_PLL.
 * @param latency Wait states for the new SYSCLK (RM0390 Table 5); see flashSetLatency().
 * @pre For the PLL, steps 1 to 8 of the bring-up order above have run.
 * @retval DRIVER_STATUS_OK                  Switched, and the latency is set.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM   @p source or @p latency is invalid.
 * @retval DRIVER_STATUS_ERR_NOT_INITIALIZED @p source is not ready.
 * @retval DRIVER_STATUS_ERR_TIMEOUT         SWS never reported @p source.
 * @retval DRIVER_STATUS_ERR_HW_FAULT        ACR.LATENCY did not read back as written.
 * @note On NOT_INITIALIZED or TIMEOUT a raised latency stays written. That is
 *       safe: extra wait states only slow flash down. A lowered latency is never
 *       written unless the switch completed.
 */
DRIVER_MUST_CHECK DriverStatus_e rccSysclkSwitch(RccRegisters_t *rcc, FlashRegisters_t *flash,
                                                 uint32_t source, uint32_t latency);

/** @} */

/** @} */

#endif /* RCC_H */
