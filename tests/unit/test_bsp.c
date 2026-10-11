/**
 * @file test_bsp.c
 * @brief Host-side tests for bsp/src/bsp.c. Each test operates on plain
 *        in-memory RccRegisters_t / PwrRegisters_t / FlashRegisters_t
 *        standing in for the real peripherals - bspClockInit() takes the
 *        register blocks as parameters rather than reaching for the
 *        hardware macros, which is what makes it testable off-target.
 *        Every hardware-ready flag it waits on (HSERDY, PLLRDY, VOSRDY,
 *        ODRDY, ODSWRDY, CFGR.SWS) is a field distinct from what the
 *        drivers write, so a test pre-sets the flags the simulated
 *        hardware would raise and withholds one to exercise that step's
 *        timeout. Compiled into tests/unit/test_runner.c's run_tests
 *        binary.
 */
#include "bsp.h"
#include "unity.h"

/** Flags a healthy part would have raised by the time each wait runs. */
#define ALL_RCC_READY (RCC_CR_HSERDY | RCC_CR_PLLRDY)
#define ALL_PWR_READY (PWR_CSR_VOSRDY | PWR_CSR_ODRDY | PWR_CSR_ODSWRDY)
#define SWS_PLL (RCC_CFGR_SYSCLK_PLL << RCC_CFGR_SWS_Pos)

/* --- the configuration constants stay inside the documented limits --- */

/** The one configuration is 180 MHz SYSCLK with APB1 at 45 MHz and APB2 at 90 MHz. */
void test_bsp_clock_configuration_frequencies(void)
{
    TEST_ASSERT_EQUAL_UINT32(180000000U, BSP_SYSCLK_HZ);
    TEST_ASSERT_EQUAL_UINT32(180000000U, BSP_HCLK_HZ);
    TEST_ASSERT_EQUAL_UINT32(45000000U, BSP_PCLK1_HZ);
    TEST_ASSERT_EQUAL_UINT32(90000000U, BSP_PCLK2_HZ);
}

/** The PLL constants keep the VCO in range: 1-2 MHz in, 100-432 MHz out. */
void test_bsp_clock_configuration_respects_pll_limits(void)
{
    uint32_t vco_in = BSP_HSE_HZ / BSP_PLL_M;
    uint32_t vco_out = vco_in * BSP_PLL_N;

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1000000U, vco_in);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(2000000U, vco_in);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(100000000U, vco_out);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(432000000U, vco_out);
    TEST_ASSERT_EQUAL_UINT32(2U, BSP_PLL_P_DIV); /* matches RCC_PLLCFGR_PLLP_DIV2 */
}

/* --- bspClockInit --- */

/** With every ready flag raised, the whole sequence runs and leaves the documented register state.
 */
void test_bsp_driver_clock_init_configures_180mhz(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY, .CFGR = SWS_PLL};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    DriverStatus_e status = bspClockInit(&rcc, &pwr, &flash);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);

    TEST_ASSERT_BITS_HIGH(RCC_APB1ENR_PWREN, rcc.APB1ENR);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_VOS_SCALE1, (pwr.CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos);
    TEST_ASSERT_BITS_HIGH(PWR_CR_ODEN | PWR_CR_ODSWEN, pwr.CR);

    TEST_ASSERT_BITS_HIGH(RCC_CR_HSEON | RCC_CR_HSEBYP | RCC_CR_PLLON, rcc.CR);
    TEST_ASSERT_EQUAL_UINT32(BSP_PLL_M,
                             (rcc.PLLCFGR & RCC_PLLCFGR_PLLM_Msk) >> RCC_PLLCFGR_PLLM_Pos);
    TEST_ASSERT_EQUAL_UINT32(BSP_PLL_N,
                             (rcc.PLLCFGR & RCC_PLLCFGR_PLLN_Msk) >> RCC_PLLCFGR_PLLN_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_PLLCFGR_PLLP_DIV2,
                            (rcc.PLLCFGR & RCC_PLLCFGR_PLLP_Msk) >> RCC_PLLCFGR_PLLP_Pos);
    TEST_ASSERT_BITS_HIGH(RCC_PLLCFGR_PLLSRC, rcc.PLLCFGR);

    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_HPRE_DIV1,
                            (rcc.CFGR & RCC_CFGR_HPRE_Msk) >> RCC_CFGR_HPRE_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_PPRE_DIV4,
                            (rcc.CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_PPRE_DIV2,
                            (rcc.CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_SYSCLK_PLL, (rcc.CFGR & RCC_CFGR_SW_Msk) >> RCC_CFGR_SW_Pos);

    TEST_ASSERT_EQUAL_UINT32(5U, flash.ACR & FLASH_ACR_LATENCY_Msk);
    TEST_ASSERT_BITS_HIGH(FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN, flash.ACR);
}

/** HSE never ready: reports a timeout and the PLL is never configured or enabled. */
void test_bsp_driver_clock_init_stops_when_hse_never_ready(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_PLLRDY, .CFGR = SWS_PLL};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    DriverStatus_e status = bspClockInit(&rcc, &pwr, &flash);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(0U, pwr.CR); /* regulator untouched: HSE failed first */
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.PLLCFGR);
    TEST_ASSERT_BITS_LOW(RCC_CR_PLLON, rcc.CR);
    TEST_ASSERT_EQUAL_HEX32(0U, flash.ACR);
}

/** HSE already running (HSEON set): rccHseEnable() refuses with BUSY and nothing after it runs. */
void test_bsp_driver_clock_init_reports_busy_when_hse_already_on(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY | RCC_CR_HSEON, .CFGR = SWS_PLL};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    DriverStatus_e status = bspClockInit(&rcc, &pwr, &flash);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_BUSY, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.PLLCFGR);
}

/** PLL never locks: timeout, and over-drive, prescalers and the switch never run. */
void test_bsp_driver_clock_init_stops_when_pll_never_locks(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSERDY, .CFGR = SWS_PLL};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    DriverStatus_e status = bspClockInit(&rcc, &pwr, &flash);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    /* The scale was written before the PLL was enabled (the ordering RM0390 5.1.4 requires:
     * CR.VOS can only be written with the PLL off) - a regulator write after rccPllEnable()
     * would leave CR.VOS untouched here. */
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_VOS_SCALE1, (pwr.CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos);
    TEST_ASSERT_BITS_LOW(PWR_CR_ODEN, pwr.CR);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CFGR & (RCC_CFGR_SW_Msk | RCC_CFGR_PPRE1_Msk));
}

/** Regulator scale never settles: timeout, and over-drive is not requested. */
void test_bsp_driver_clock_init_stops_when_voltage_scale_never_settles(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY, .CFGR = SWS_PLL};
    PwrRegisters_t pwr = {.CSR = PWR_CSR_ODRDY | PWR_CSR_ODSWRDY};
    FlashRegisters_t flash = {0};

    DriverStatus_e status = bspClockInit(&rcc, &pwr, &flash);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_VOS_SCALE1, (pwr.CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos);
    TEST_ASSERT_BITS_LOW(PWR_CR_ODEN, pwr.CR);
}

/** Over-drive never ready: timeout, and SYSCLK is never switched. */
void test_bsp_driver_clock_init_stops_when_overdrive_never_ready(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY, .CFGR = SWS_PLL};
    PwrRegisters_t pwr = {.CSR = PWR_CSR_VOSRDY};
    FlashRegisters_t flash = {0};

    DriverStatus_e status = bspClockInit(&rcc, &pwr, &flash);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CFGR & RCC_CFGR_SW_Msk);
    TEST_ASSERT_EQUAL_HEX32(0U, flash.ACR);
}

/** SYSCLK never reports the PLL: timeout, and the caches are left off. */
void test_bsp_driver_clock_init_stops_when_sysclk_switch_never_completes(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY, .CFGR = 0U};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    DriverStatus_e status = bspClockInit(&rcc, &pwr, &flash);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_BITS_LOW(FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN, flash.ACR);
}

/** The PLL already running (HSE off): rccPllConfig() refuses with BUSY, and the sequence stops
 *  before over-drive, the prescalers and the switch. */
void test_bsp_driver_clock_init_reports_busy_when_pll_already_on(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY | RCC_CR_PLLON, .CFGR = SWS_PLL};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    DriverStatus_e status = bspClockInit(&rcc, &pwr, &flash);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_BUSY, status);
    TEST_ASSERT_BITS_LOW(PWR_CR_ODEN, pwr.CR);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CFGR & RCC_CFGR_SW_Msk);
}
