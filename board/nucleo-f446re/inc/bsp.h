/**
 * @file bsp.h
 * @brief NUCLEO-F446RE board support: the 180 MHz clock configuration.
 *
 * One board, one configuration, so there is no frequency solver. The
 * configuration follows ST's own NUCLEO-F446 180 MHz example, from the 8 MHz
 * clock the ST-LINK drives on OSC_IN (see the
 * [clock tree](ARCHITECTURE.md#clock-tree)):
 *
 * | Stage     | Setting            | Result  |
 * | --------- | ------------------ | ------- |
 * | HSE       | bypass             | 8 MHz   |
 * | PLL M     | 8                  | 1 MHz   |
 * | PLL N     | 360                | 360 MHz |
 * | PLL P     | 2                  | 180 MHz SYSCLK |
 * | AHB       | /1                 | 180 MHz |
 * | APB1      | /4                 | 45 MHz  |
 * | APB2      | /2                 | 90 MHz  |
 * | Regulator | scale 1, over-drive |        |
 * | Flash     | 5 wait states, prefetch and caches on | |
 *
 * The `_Static_assert`s below check the `BSP_*` constants against the chip's
 * limits. bsp.c writes the matching register values separately, and
 * `test_bsp.c` checks the two agree, so change both together.
 */
#ifndef BSP_H
#define BSP_H

#include "driver_status.h"
#include "flash.h"
#include "pwr.h"
#include "rcc.h"

/**
 * @addtogroup board_support
 * @{
 */

/** @name Configuration
 *  @{ */
#define BSP_HSE_HZ (8000000U) ///< HSE input from the ST-LINK MCO, in bypass mode
#define BSP_PLL_M (8U)        ///< PLL M: VCO input = HSE / M = 1 MHz
#define BSP_PLL_N (360U)      ///< PLL N: VCO output = 1 MHz x N = 360 MHz
#define BSP_PLL_P_DIV (2U)    ///< PLL P divisor (register value ::RCC_PLLCFGR_PLLP_DIV2)
/** Flash wait states for 180 MHz at 2.7-3.6 V (RM0390 Table 5). */
#define BSP_FLASH_LATENCY (5U)
/** @} */

/** @name Resulting clocks
 *  Pass these to systickInit() and uartInit().
 *  @{ */
/** SYSCLK: HSE / M x N / P. */
#define BSP_SYSCLK_HZ ((BSP_HSE_HZ / BSP_PLL_M) * BSP_PLL_N / BSP_PLL_P_DIV)
#define BSP_HCLK_HZ (BSP_SYSCLK_HZ)     ///< HCLK (AHB /1): core, SysTick, GPIO
#define BSP_PCLK1_HZ (BSP_HCLK_HZ / 4U) ///< APB1 (/4): USART2, PWR
#define BSP_PCLK2_HZ (BSP_HCLK_HZ / 2U) ///< APB2 (/2): SYSCFG
/** @} */

/* Chip limits for this configuration (RM0390): VCO input 1-2 MHz, VCO output
 * 100-432 MHz, SYSCLK <= 180 MHz, APB1 <= 45 MHz, APB2 <= 90 MHz. Checked here
 * because rccPllConfig() does not know its source frequency. */
_Static_assert(((BSP_HSE_HZ / BSP_PLL_M) >= 1000000U) && ((BSP_HSE_HZ / BSP_PLL_M) <= 2000000U),
               "PLL VCO input must be 1-2 MHz");
_Static_assert((((BSP_HSE_HZ / BSP_PLL_M) * BSP_PLL_N) >= 100000000U)
                   && (((BSP_HSE_HZ / BSP_PLL_M) * BSP_PLL_N) <= 432000000U),
               "PLL VCO output must be 100-432 MHz");
_Static_assert(BSP_SYSCLK_HZ <= 180000000U, "SYSCLK must not exceed 180 MHz");
_Static_assert(BSP_PCLK1_HZ <= 45000000U, "APB1 must not exceed 45 MHz");
_Static_assert(BSP_PCLK2_HZ <= 90000000U, "APB2 must not exceed 90 MHz");

/**
 * @brief Bring SYSCLK from the 16 MHz HSI reset state to 180 MHz.
 *
 * Follows the bring-up order in rcc.h, turning HSE on before setting the
 * regulator so that a missing clock fails first. It ends by enabling
 * prefetch and the caches. It stops at the first failing step and returns
 * that step's status.
 *
 * @param rcc   RCC block (e.g. RCC).
 * @param pwr   PWR block (e.g. PWR).
 * @param flash Flash interface block (e.g. FLASH).
 * @pre Called once from reset: SYSCLK is HSI, and HSE and the PLL are off.
 * @retval DRIVER_STATUS_OK          SYSCLK runs from the PLL at 180 MHz.
 * @retval DRIVER_STATUS_ERR_TIMEOUT HSE, the PLL, the regulator, over-drive or the switch
 *                                   did not report ready.
 * @retval DRIVER_STATUS_ERR_BUSY    HSE or the PLL was already on.
 * @retval DRIVER_STATUS_ERR_HW_FAULT The flash latency did not read back.
 * @warning No unwind. After a failure the earlier steps stay in effect, such
 *          as HSE, the regulator scale or a running PLL, while SYSCLK is still
 *          HSI, which is safe to continue from. A second call reports BUSY.
 * @note INVALID_PARAM and NOT_INITIALIZED cannot occur with these constants.
 */
DRIVER_MUST_CHECK DriverStatus_e bspClockInit(RccRegisters_t *rcc, PwrRegisters_t *pwr,
                                              FlashRegisters_t *flash);

/** @} */

#endif /* BSP_H */
