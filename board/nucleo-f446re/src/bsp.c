/**
 * @file bsp.c
 * @brief Board support implementation (API in bsp.h).
 */
#include "bsp.h"

/**
 * @addtogroup board_support
 * @{
 */

DriverStatus_e bspClockInit(RccRegisters_t *rcc, PwrRegisters_t *pwr, FlashRegisters_t *flash)
{
    DriverStatus_e status;

    rccPwrClockEnable(rcc);

    /* HSE first: it is the step most likely to fail on a board (no ST-LINK
     * MCO, solder bridges), and failing here leaves the regulator untouched. */
    status = rccHseEnable(rcc, RCC_CR_HSEBYP);

    if (status == DRIVER_STATUS_OK)
    {
        status = pwrSetVoltageScale(pwr, PWR_CR_VOS_SCALE1);
    }

    if (status == DRIVER_STATUS_OK)
    {
        uint32_t pll_source = RCC_PLLCFGR_PLLSRC_HSE;

        status = rccPllConfig(rcc, pll_source, BSP_PLL_M, BSP_PLL_N, RCC_PLLCFGR_PLLP_DIV2);
    }

    if (status == DRIVER_STATUS_OK)
    {
        status = rccPllEnable(rcc);
    }

    if (status == DRIVER_STATUS_OK)
    {
        status = pwrWaitVoltageScaleReady(pwr);
    }

    if (status == DRIVER_STATUS_OK)
    {
        status = pwrEnableOverdrive(pwr);
    }

    if (status == DRIVER_STATUS_OK)
    {
        status =
            rccBusPrescalerConfig(rcc, RCC_CFGR_HPRE_DIV1, RCC_CFGR_PPRE_DIV4, RCC_CFGR_PPRE_DIV2);
    }

    if (status == DRIVER_STATUS_OK)
    {
        status = rccSysclkSwitch(rcc, flash, RCC_CFGR_SYSCLK_PLL, BSP_FLASH_LATENCY);
    }

    if (status == DRIVER_STATUS_OK)
    {
        flashEnableCaches(flash);
    }

    return status;
}

/** @} */
