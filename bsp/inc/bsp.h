/**
 * @file bsp.h
 * @brief Board support package for the NUCLEO-F446RE - the one place that
 *        knows which clock configuration this board runs at. Consumes
 *        drivers/ and, once api/ exists, is consumed by it. No
 *        application-facing logic.
 *
 * Today that is a single function, bspClockInit(): the board-specific
 * sequence that takes the part from its 16 MHz HSI reset state to 180 MHz.
 * It is deliberately not a general clock framework - one caller, one
 * configuration - so there is no frequency solver and no VCO/APB limit
 * validation here or in rccPllConfig()/rccBusPrescalerConfig(). The
 * limits are instead documented against the one tested configuration
 * below, and tests/unit/test_bsp.c asserts that configuration respects
 * them. Anyone changing a BSP_* constant must re-check the limits.
 *
 * The configuration, which follows ST's own 180 MHz example for this part
 * (SystemClock_Config in its Nucleo-F446 projects), driven from the
 * 8 MHz clock the on-board ST-LINK supplies on OSC_IN (HSE bypass):
 *   - PLL: 8 MHz / M=8 = 1 MHz VCO input (the PLL's 1-2 MHz range), x
 *     N=360 = 360 MHz VCO output (100-432 MHz), / P=2 = 180 MHz SYSCLK.
 *     M and N are ST's example's own values, not an equivalent pair such
 *     as M=4/N=180, so they are the combination known to bring this part
 *     up.
 *   - Regulator scale 1 plus over-drive, which 180 MHz needs.
 *   - AHB /1 = 180 MHz HCLK; APB1 /4 = 45 MHz and APB2 /2 = 90 MHz,
 *     which are those buses' maximum rates on the F446.
 *   - 5 flash wait states, then prefetch and both caches on.
 *
 * Takes the register blocks it drives as parameters even though they are
 * hardware singletons - see CONTRIBUTING.md's error-handling contract
 * section for why every module in this project does this.
 */
#ifndef BSP_H
#define BSP_H

#include "driver_status.h"
#include "flash.h"
#include "pwr.h"
#include "rcc.h"

#include <stdint.h>

/**
 * @addtogroup board_support
 * @{
 */

#define BSP_HSE_HZ (8000000U) ///< HSE input: ST-LINK MCO on OSC_IN, bypass mode
#define BSP_PLL_M (8U)        ///< PLLM: VCO input = HSE / M = 1 MHz
#define BSP_PLL_N (360U)      ///< PLLN: VCO output = VCO input * N = 360 MHz
#define BSP_PLL_P_DIV (2U) ///< PLLP divisor value (the register encoding is RCC_PLLCFGR_PLLP_DIV2)
#define BSP_FLASH_LATENCY                                                                          \
    (5U) ///< Flash wait states at 180 MHz, scale 1 + over-drive, 2.7-3.6 V supply

/** SYSCLK after bspClockInit(): HSE / M * N / P. */
#define BSP_SYSCLK_HZ ((BSP_HSE_HZ / BSP_PLL_M) * BSP_PLL_N / BSP_PLL_P_DIV)
#define BSP_HCLK_HZ (BSP_SYSCLK_HZ)     ///< AHB clock (HPRE /1); also the SysTick/core clock
#define BSP_PCLK1_HZ (BSP_HCLK_HZ / 4U) ///< APB1 clock (PPRE1 /4); USART2's bus
#define BSP_PCLK2_HZ (BSP_HCLK_HZ / 2U) ///< APB2 clock (PPRE2 /2)

/* The limits this one configuration must respect, enforced at compile time so a
 * firmware build (not just `make test`) fails if a BSP_* constant is edited out of
 * range: PLL VCO input 1-2 MHz and output 100-432 MHz, SYSCLK 180 MHz maximum, APB1
 * 45 MHz and APB2 90 MHz maximum (rcc.h; RM0390). rccPllConfig() deliberately does
 * not check them - it does not know its source frequency. */
_Static_assert(((BSP_HSE_HZ / BSP_PLL_M) >= 1000000U) && ((BSP_HSE_HZ / BSP_PLL_M) <= 2000000U),
               "PLL VCO input must be 1-2 MHz");
_Static_assert((((BSP_HSE_HZ / BSP_PLL_M) * BSP_PLL_N) >= 100000000U)
                   && (((BSP_HSE_HZ / BSP_PLL_M) * BSP_PLL_N) <= 432000000U),
               "PLL VCO output must be 100-432 MHz");
_Static_assert(BSP_SYSCLK_HZ <= 180000000U, "SYSCLK must not exceed 180 MHz");
_Static_assert(BSP_PCLK1_HZ <= 45000000U, "APB1 must not exceed 45 MHz");
_Static_assert(BSP_PCLK2_HZ <= 90000000U, "APB2 must not exceed 90 MHz");

/**
 * @brief Bring the system clock from the HSI reset default to 180 MHz.
 *
 * Runs the ordered sequence documented in rcc.h's file-level comment:
 * PWR clock on, HSE (bypass) on, regulator scale 1 (steps 2 and 3 there
 * are independent; HSE goes first so a missing clock fails before the
 * regulator is touched), PLL configured and enabled, regulator settled,
 * over-drive on, bus prescalers set, SYSCLK switched to the PLL with 5
 * wait states, and finally the flash prefetch and caches enabled.
 *
 * Stops at the first step that fails and returns its status; nothing
 * after that step runs. There is no unwind, so a failure leaves whatever
 * earlier steps already did in place - HSE and its bypass bit, the
 * regulator scale, a running PLL, over-drive, and (if the failure is the
 * SYSCLK switch itself) the bus prescalers - while SYSCLK is still HSI,
 * which is a safe state to continue from. After an SYSCLK-switch timeout
 * CFGR.SW still requests the PLL, so the switch can complete later. The
 * function cannot simply be called again: HSE and the PLL are left on,
 * and the second call reports DRIVER_STATUS_ERR_BUSY.
 *
 * @param rcc   RCC register block (e.g. RCC).
 * @param pwr   PWR register block (e.g. PWR).
 * @param flash Flash interface register block (e.g. FLASH).
 * @pre Called once, from the reset state: SYSCLK is HSI, and HSE and the
 *      PLL are off. HSE already running is reported as
 *      DRIVER_STATUS_ERR_BUSY by rccHseEnable() because HSEBYP cannot be
 *      changed while it is on.
 * @return DRIVER_STATUS_OK once SYSCLK is confirmed running from the PLL.
 * @return DRIVER_STATUS_ERR_TIMEOUT if HSE, the PLL, the regulator scale,
 *         over-drive, or the SYSCLK switch never reported ready within its
 *         bounded wait.
 * @return DRIVER_STATUS_ERR_BUSY if HSE or the PLL was already on.
 * @return DRIVER_STATUS_ERR_HW_FAULT if the flash latency did not read
 *         back as written.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM or
 *         DRIVER_STATUS_ERR_NOT_INITIALIZED are not reachable with the
 *         BSP_* constants as they are; they would be if a constant were
 *         edited to an out-of-range PLL M/N or latency, or a clock source
 *         were not ready when the switch ran.
 */
DRIVER_MUST_CHECK DriverStatus_e bspClockInit(RccRegisters_t *rcc, PwrRegisters_t *pwr,
                                              FlashRegisters_t *flash);

/** @} */

#endif /* BSP_H */
