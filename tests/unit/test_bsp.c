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

/** PLLP divisor and APB prescaler of a CFGR/PLLCFGR value, as the hardware
 *  reads the field: PLLP encodes 2/4/6/8 as 0..3; PPRE below 0b100 is /1 and
 *  0b100..0b111 is /2../16. */
static uint32_t pllp_divisor(uint32_t pllcfgr)
{
    return (((pllcfgr & RCC_PLLCFGR_PLLP_Msk) >> RCC_PLLCFGR_PLLP_Pos) + 1U) * 2U;
}

static uint32_t ppre_divisor(uint32_t field)
{
    return (field < RCC_CFGR_PPRE_DIV2) ? 1U : (2U << (field - RCC_CFGR_PPRE_DIV2));
}

/** The clocks the registers bspClockInit() actually programmed work out to the
 *  BSP_*_HZ constants. The _Static_asserts in bsp.h check the limits against
 *  the BSP_* constants only; the register values are separate literals in
 *  bsp.c, so this ties the two together - editing one PPRE or PLLP literal
 *  without its constant now fails here. */
void test_bsp_driver_clock_init_programs_the_documented_frequencies(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY, .CFGR = SWS_PLL};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, bspClockInit(&rcc, &pwr, &flash));

    uint32_t m = (rcc.PLLCFGR & RCC_PLLCFGR_PLLM_Msk) >> RCC_PLLCFGR_PLLM_Pos;
    uint32_t n = (rcc.PLLCFGR & RCC_PLLCFGR_PLLN_Msk) >> RCC_PLLCFGR_PLLN_Pos;
    uint32_t sysclk = (BSP_HSE_HZ / m) * n / pllp_divisor(rcc.PLLCFGR);
    uint32_t ppre1 = ppre_divisor((rcc.CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos);
    uint32_t ppre2 = ppre_divisor((rcc.CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos);

    TEST_ASSERT_EQUAL_UINT32(BSP_SYSCLK_HZ, sysclk);
    /* AHB /1, so HCLK == SYSCLK == BSP_HCLK_HZ */
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_HPRE_DIV1,
                            (rcc.CFGR & RCC_CFGR_HPRE_Msk) >> RCC_CFGR_HPRE_Pos);
    TEST_ASSERT_EQUAL_UINT32(BSP_PCLK1_HZ, sysclk / ppre1);
    TEST_ASSERT_EQUAL_UINT32(BSP_PCLK2_HZ, sysclk / ppre2);
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

/* --- review follow-ups: ordering and partial-failure state --- */

/** The PLL is enabled *before* the voltage-scale wait: with VOSRDY never set the function times out
 *  at that wait, but PLLON and the PLLCFGR dividers are already in place (the wait cannot complete
 *  with the PLL off - the ordering bug CONTRIBUTING.md says shipped once). */
void test_bsp_driver_clock_init_enables_pll_before_voltage_scale_wait(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY, .CFGR = SWS_PLL};
    PwrRegisters_t pwr = {.CSR = PWR_CSR_ODRDY | PWR_CSR_ODSWRDY};
    FlashRegisters_t flash = {0};

    DriverStatus_e status = bspClockInit(&rcc, &pwr, &flash);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_BITS_HIGH(RCC_CR_PLLON, rcc.CR);
    TEST_ASSERT_EQUAL_UINT32(BSP_PLL_M,
                             (rcc.PLLCFGR & RCC_PLLCFGR_PLLM_Msk) >> RCC_PLLCFGR_PLLM_Pos);
}

/** The PWR clock is enabled first, even when HSE then fails (PWR registers would be unreachable
 *  without it). */
void test_bsp_driver_clock_init_enables_pwr_clock_first(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_PLLRDY};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, bspClockInit(&rcc, &pwr, &flash));
    TEST_ASSERT_BITS_HIGH(RCC_APB1ENR_PWREN, rcc.APB1ENR);
}

/** A SYSCLK-switch timeout leaves what bsp.h documents: the bus prescalers set to their final
 *  values, CFGR.SW still requesting the PLL, latency already raised to 5 - and the caches off. */
void test_bsp_driver_clock_init_switch_timeout_leaves_documented_state(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY, .CFGR = 0U};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, bspClockInit(&rcc, &pwr, &flash));

    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_HPRE_DIV1,
                            (rcc.CFGR & RCC_CFGR_HPRE_Msk) >> RCC_CFGR_HPRE_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_PPRE_DIV4,
                            (rcc.CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_PPRE_DIV2,
                            (rcc.CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_SYSCLK_PLL, (rcc.CFGR & RCC_CFGR_SW_Msk) >> RCC_CFGR_SW_Pos);
    TEST_ASSERT_EQUAL_UINT32(5U, flash.ACR & FLASH_ACR_LATENCY_Msk);
}

/** A second call after a timed-out first one cannot simply be retried: HSE is left on, so
 *  rccHseEnable() reports BUSY (bsp.h documents this). */
void test_bsp_driver_clock_init_cannot_be_retried_after_a_late_failure(void)
{
    RccRegisters_t rcc = {.CR = ALL_RCC_READY, .CFGR = 0U};
    PwrRegisters_t pwr = {.CSR = ALL_PWR_READY};
    FlashRegisters_t flash = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, bspClockInit(&rcc, &pwr, &flash));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_BUSY, bspClockInit(&rcc, &pwr, &flash));
}
