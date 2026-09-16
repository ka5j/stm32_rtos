/**
 * @file test_rcc.c
 * @brief Host-side tests for drivers/src/rcc.c. Each test operates on a
 *        plain in-memory RccRegisters_t standing in for a real peripheral
 *        - rcc.c's functions take the register block as a parameter
 *        rather than reaching for the hardware RCC macro, which is what
 *        makes this testable off-target. GPIOA..GPIOH (gpio_reg.h) are
 *        used only as pointer-identity values here, never dereferenced,
 *        so they're safe to pass on the host build too. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "rcc.h"
#include "unity.h"

#include <stddef.h>

/* --- rccGpioClockEnable / rccGpioClockDisable: port identity --- */

/** Every GPIOA..GPIOH port enables its own documented RCC_AHB1ENR bit
 *  (device/inc/rcc_reg.h) and no other. */
void test_rcc_driver_gpio_clock_enable_maps_every_port_to_its_documented_bit(void)
{
    static const GpioRegisters_t *const port[] = {GPIOA, GPIOB, GPIOC, GPIOD,
                                                  GPIOE, GPIOF, GPIOG, GPIOH};
    static const uint32_t expected_bit[] = {
        RCC_AHB1ENR_GPIOAEN, RCC_AHB1ENR_GPIOBEN, RCC_AHB1ENR_GPIOCEN, RCC_AHB1ENR_GPIODEN,
        RCC_AHB1ENR_GPIOEEN, RCC_AHB1ENR_GPIOFEN, RCC_AHB1ENR_GPIOGEN, RCC_AHB1ENR_GPIOHEN};

    for (size_t i = 0; i < sizeof(port) / sizeof(port[0]); i++)
    {
        RccRegisters_t rcc = {0};

        DriverStatus_e status = rccGpioClockEnable(&rcc, port[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(expected_bit[i], rcc.AHB1ENR);
    }
}

/** Every GPIOA..GPIOH port disables its own documented RCC_AHB1ENR bit
 *  and no other. */
void test_rcc_driver_gpio_clock_disable_maps_every_port_to_its_documented_bit(void)
{
    static const GpioRegisters_t *const port[] = {GPIOA, GPIOB, GPIOC, GPIOD,
                                                  GPIOE, GPIOF, GPIOG, GPIOH};
    static const uint32_t enabled_bit[] = {
        RCC_AHB1ENR_GPIOAEN, RCC_AHB1ENR_GPIOBEN, RCC_AHB1ENR_GPIOCEN, RCC_AHB1ENR_GPIODEN,
        RCC_AHB1ENR_GPIOEEN, RCC_AHB1ENR_GPIOFEN, RCC_AHB1ENR_GPIOGEN, RCC_AHB1ENR_GPIOHEN};

    for (size_t i = 0; i < sizeof(port) / sizeof(port[0]); i++)
    {
        RccRegisters_t rcc = {.AHB1ENR = 0xFFFFFFFFU};

        DriverStatus_e status = rccGpioClockDisable(&rcc, port[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~enabled_bit[i], rcc.AHB1ENR);
    }
}

/** A pointer that isn't one of GPIOA..GPIOH must be rejected before any
 *  write - including a NULL pointer, which is never dereferenced. */
void test_rcc_driver_gpio_clock_enable_rejects_unrecognized_port(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status = rccGpioClockEnable(&rcc, NULL);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.AHB1ENR);
}

/** Same rejection on the disable path. */
void test_rcc_driver_gpio_clock_disable_rejects_unrecognized_port(void)
{
    RccRegisters_t rcc = {.AHB1ENR = 0xFFFFFFFFU};

    DriverStatus_e status = rccGpioClockDisable(&rcc, NULL);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, rcc.AHB1ENR);
}

/* --- rccGpioClockEnable / rccGpioClockDisable: bit isolation --- */

/** Enabling one port's clock must not disturb any other AHB1ENR bit. */
void test_rcc_driver_gpio_clock_enable_preserves_other_bits(void)
{
    RccRegisters_t rcc = {.AHB1ENR = 0xFFFFFFFFU & ~RCC_AHB1ENR_GPIOCEN};

    DriverStatus_e status = rccGpioClockEnable(&rcc, GPIOC);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, rcc.AHB1ENR);
}

/** Disabling one port's clock must not disturb any other AHB1ENR bit. */
void test_rcc_driver_gpio_clock_disable_preserves_other_bits(void)
{
    RccRegisters_t rcc = {.AHB1ENR = 0xFFFFFFFFU};

    DriverStatus_e status = rccGpioClockDisable(&rcc, GPIOC);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~RCC_AHB1ENR_GPIOCEN, rcc.AHB1ENR);
}

/* --- rccUsart2ClockEnable / rccUsart2ClockDisable --- */

/** Sets USART2EN without touching any other APB1ENR bit. */
void test_rcc_driver_usart2_clock_enable_sets_bit_only(void)
{
    RccRegisters_t rcc = {.APB1ENR = 0xFFFFFFFFU & ~RCC_APB1ENR_USART2EN};

    rccUsart2ClockEnable(&rcc);

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, rcc.APB1ENR);
}

/** Clears USART2EN without touching any other APB1ENR bit. */
void test_rcc_driver_usart2_clock_disable_clears_bit_only(void)
{
    RccRegisters_t rcc = {.APB1ENR = 0xFFFFFFFFU};

    rccUsart2ClockDisable(&rcc);

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~RCC_APB1ENR_USART2EN, rcc.APB1ENR);
}

/* --- rccSyscfgClockEnable / rccSyscfgClockDisable --- */

/** Sets SYSCFGEN without touching any other APB2ENR bit. */
void test_rcc_driver_syscfg_clock_enable_sets_bit_only(void)
{
    RccRegisters_t rcc = {.APB2ENR = 0xFFFFFFFFU & ~RCC_APB2ENR_SYSCFGEN};

    rccSyscfgClockEnable(&rcc);

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, rcc.APB2ENR);
}

/** Clears SYSCFGEN without touching any other APB2ENR bit. */
void test_rcc_driver_syscfg_clock_disable_clears_bit_only(void)
{
    RccRegisters_t rcc = {.APB2ENR = 0xFFFFFFFFU};

    rccSyscfgClockDisable(&rcc);

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~RCC_APB2ENR_SYSCFGEN, rcc.APB2ENR);
}

/* --- rccPwrClockEnable / rccPwrClockDisable --- */

/** Sets PWREN without touching any other APB1ENR bit. */
void test_rcc_driver_pwr_clock_enable_sets_bit_only(void)
{
    RccRegisters_t rcc = {.APB1ENR = 0xFFFFFFFFU & ~RCC_APB1ENR_PWREN};

    rccPwrClockEnable(&rcc);

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, rcc.APB1ENR);
}

/** Clears PWREN without touching any other APB1ENR bit. */
void test_rcc_driver_pwr_clock_disable_clears_bit_only(void)
{
    RccRegisters_t rcc = {.APB1ENR = 0xFFFFFFFFU};

    rccPwrClockDisable(&rcc);

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~RCC_APB1ENR_PWREN, rcc.APB1ENR);
}

/* --- rccHsiEnable / rccHsiDisable ---
 *
 * CR.HSION (what the driver writes) and CR.HSIRDY (what it polls) are
 * separate bits in the same register - a test can set HSIRDY up front to
 * simulate "hardware already caught up" (immediate OK) or leave it clear
 * to simulate "never catches up" (the bounded retry exhausts, ERR_TIMEOUT),
 * with no real hardware or timer involved. Same pattern for HSE/PLL below.
 */

/** HSION is set and HSIRDY (pre-set here) is observed immediately -> OK. */
void test_rcc_driver_hsi_enable_reports_ok_when_ready(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSIRDY};

    DriverStatus_e status = rccHsiEnable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_HSION | RCC_CR_HSIRDY, rcc.CR);
}

/** HSIRDY never set -> the bounded retry exhausts and reports a timeout;
 *  HSION is still set, since the write happens before the poll. */
void test_rcc_driver_hsi_enable_times_out_when_hsirdy_never_sets(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status = rccHsiEnable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_HSION, rcc.CR & RCC_CR_HSION);
}

/** HSI is not the active SYSCLK source (SWS != HSI) -> HSION is cleared. */
void test_rcc_driver_hsi_disable_clears_hsion_when_not_active_source(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSION, .CFGR = RCC_CFGR_SYSCLK_HSE << RCC_CFGR_SWS_Pos};

    DriverStatus_e status = rccHsiDisable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CR & RCC_CR_HSION);
}

/** HSI is the active SYSCLK source (SWS == HSI) -> rejected, HSION
 *  untouched, since clearing it would stop the running clock. */
void test_rcc_driver_hsi_disable_rejects_when_active_source(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSION, .CFGR = RCC_CFGR_SYSCLK_HSI << RCC_CFGR_SWS_Pos};

    DriverStatus_e status = rccHsiDisable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_BUSY, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_HSION, rcc.CR & RCC_CR_HSION);
}

/* --- rccHseEnable / rccHseDisable --- */

/** bypass outside {0, RCC_CR_HSEBYP} must be rejected before any write. */
void test_rcc_driver_hse_enable_rejects_invalid_bypass(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status = rccHseEnable(&rcc, 0xFFU);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CR);
}

/** HSEON already set must be rejected - HSEBYP cannot be safely
 *  reconfigured without disabling HSE first. */
void test_rcc_driver_hse_enable_rejects_when_already_on(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSEON};

    DriverStatus_e status = rccHseEnable(&rcc, RCC_CR_HSEBYP);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_BUSY, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CR & RCC_CR_HSEBYP);
}

/** bypass mode sets both HSEBYP and HSEON, and reports OK once HSERDY
 *  (pre-set here) is observed. */
void test_rcc_driver_hse_enable_bypass_mode_sets_hsebyp_and_hseon(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSERDY};

    DriverStatus_e status = rccHseEnable(&rcc, RCC_CR_HSEBYP);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_HSEBYP | RCC_CR_HSEON, rcc.CR & (RCC_CR_HSEBYP | RCC_CR_HSEON));
}

/** crystal mode (bypass = 0) sets HSEON but leaves HSEBYP clear. */
void test_rcc_driver_hse_enable_crystal_mode_leaves_hsebyp_clear(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSERDY};

    DriverStatus_e status = rccHseEnable(&rcc, 0U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_HSEON, rcc.CR & (RCC_CR_HSEBYP | RCC_CR_HSEON));
}

/** HSERDY never set -> the bounded retry exhausts and reports a timeout. */
void test_rcc_driver_hse_enable_times_out_when_hserdy_never_sets(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status = rccHseEnable(&rcc, 0U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_HSEON, rcc.CR & RCC_CR_HSEON);
}

/** HSE is not the active SYSCLK source -> HSEON is cleared. */
void test_rcc_driver_hse_disable_clears_hseon_when_not_active_source(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSEON, .CFGR = RCC_CFGR_SYSCLK_HSI << RCC_CFGR_SWS_Pos};

    DriverStatus_e status = rccHseDisable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CR & RCC_CR_HSEON);
}

/** HSE is the active SYSCLK source -> rejected, HSEON untouched. */
void test_rcc_driver_hse_disable_rejects_when_active_source(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSEON, .CFGR = RCC_CFGR_SYSCLK_HSE << RCC_CFGR_SWS_Pos};

    DriverStatus_e status = rccHseDisable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_BUSY, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_HSEON, rcc.CR & RCC_CR_HSEON);
}

/* --- rccPllConfig / rccPllEnable / rccPllDisable --- */

/** source outside {PLLSRC_HSI, PLLSRC_HSE} must be rejected before any
 *  write. */
void test_rcc_driver_pll_config_rejects_invalid_source(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status = rccPllConfig(&rcc, 0x2U, 8U, 200U, RCC_PLLCFGR_PLLP_DIV2);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.PLLCFGR);
}

/** m outside its documented 2-63 range must be rejected (0, 1, and 64
 *  are all invalid; 0 and 1 are also within PLLM's 6-bit field width,
 *  so this specifically exercises the RM0390 subrange check, not just
 *  the field-width check). */
void test_rcc_driver_pll_config_rejects_m_out_of_range(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status_low =
        rccPllConfig(&rcc, RCC_PLLCFGR_PLLSRC_HSI, 1U, 200U, RCC_PLLCFGR_PLLP_DIV2);
    DriverStatus_e status_high =
        rccPllConfig(&rcc, RCC_PLLCFGR_PLLSRC_HSI, 64U, 200U, RCC_PLLCFGR_PLLP_DIV2);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status_low);
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status_high);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.PLLCFGR);
}

/** n outside its documented 50-432 range must be rejected. */
void test_rcc_driver_pll_config_rejects_n_out_of_range(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status_low =
        rccPllConfig(&rcc, RCC_PLLCFGR_PLLSRC_HSI, 8U, 49U, RCC_PLLCFGR_PLLP_DIV2);
    DriverStatus_e status_high =
        rccPllConfig(&rcc, RCC_PLLCFGR_PLLSRC_HSI, 8U, 433U, RCC_PLLCFGR_PLLP_DIV2);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status_low);
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status_high);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.PLLCFGR);
}

/** p outside PLLP's 2-bit field must be rejected. */
void test_rcc_driver_pll_config_rejects_p_out_of_range(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status = rccPllConfig(&rcc, RCC_PLLCFGR_PLLSRC_HSI, 8U, 200U, 4U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.PLLCFGR);
}

/** PLLON already set must be rejected - RM0390 forbids writing PLLCFGR
 *  while the PLL is enabled. */
void test_rcc_driver_pll_config_rejects_when_already_on(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_PLLON};

    DriverStatus_e status =
        rccPllConfig(&rcc, RCC_PLLCFGR_PLLSRC_HSI, 8U, 200U, RCC_PLLCFGR_PLLP_DIV2);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_BUSY, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.PLLCFGR);
}

/** A valid call writes PLLM/PLLN/PLLP/PLLSRC and preserves every other
 *  PLLCFGR bit (PLLQ and reserved bits) untouched. */
void test_rcc_driver_pll_config_writes_fields_and_preserves_pllq(void)
{
    RccRegisters_t rcc = {.PLLCFGR = 0xFFFFFFFFU};

    DriverStatus_e status =
        rccPllConfig(&rcc, RCC_PLLCFGR_PLLSRC_HSE, 8U, 200U, RCC_PLLCFGR_PLLP_DIV4);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(8U, (rcc.PLLCFGR & RCC_PLLCFGR_PLLM_Msk) >> RCC_PLLCFGR_PLLM_Pos);
    TEST_ASSERT_EQUAL_HEX32(200U, (rcc.PLLCFGR & RCC_PLLCFGR_PLLN_Msk) >> RCC_PLLCFGR_PLLN_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_PLLCFGR_PLLP_DIV4,
                            (rcc.PLLCFGR & RCC_PLLCFGR_PLLP_Msk) >> RCC_PLLCFGR_PLLP_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_PLLCFGR_PLLSRC, rcc.PLLCFGR & RCC_PLLCFGR_PLLSRC);
    /* Bits outside PLLM/PLLN/PLLP/PLLSRC (e.g. PLLQ) were all 1 in setup
     * and must still be 1. */
    uint32_t owned_mask =
        RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk | RCC_PLLCFGR_PLLSRC;
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~owned_mask, rcc.PLLCFGR & ~owned_mask);
}

/** PLLRDY (pre-set here) is observed immediately -> OK. */
void test_rcc_driver_pll_enable_reports_ok_when_ready(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_PLLRDY};

    DriverStatus_e status = rccPllEnable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_PLLON, rcc.CR & RCC_CR_PLLON);
}

/** PLLRDY never set -> the bounded retry exhausts and reports a timeout. */
void test_rcc_driver_pll_enable_times_out_when_pllrdy_never_sets(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status = rccPllEnable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_PLLON, rcc.CR & RCC_CR_PLLON);
}

/** PLL is not the active SYSCLK source -> PLLON is cleared. */
void test_rcc_driver_pll_disable_clears_pllon_when_not_active_source(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_PLLON, .CFGR = RCC_CFGR_SYSCLK_HSI << RCC_CFGR_SWS_Pos};

    DriverStatus_e status = rccPllDisable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CR & RCC_CR_PLLON);
}

/** PLL is the active SYSCLK source -> rejected, PLLON untouched. */
void test_rcc_driver_pll_disable_rejects_when_active_source(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_PLLON, .CFGR = RCC_CFGR_SYSCLK_PLL << RCC_CFGR_SWS_Pos};

    DriverStatus_e status = rccPllDisable(&rcc);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_BUSY, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CR_PLLON, rcc.CR & RCC_CR_PLLON);
}

/* --- rccBusPrescalerConfig --- */

/** hpre outside its 9 documented divisors must be rejected before any
 *  write - 0x1 is within HPRE's 4-bit field width but not one of the
 *  named values (device/inc/rcc_reg.h's "no /32 encoding" note). */
void test_rcc_driver_bus_prescaler_config_rejects_invalid_hpre(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status =
        rccBusPrescalerConfig(&rcc, 0x1U, RCC_CFGR_PPRE_DIV1, RCC_CFGR_PPRE_DIV1);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CFGR);
}

/** ppre1 outside its 5 documented divisors must be rejected. */
void test_rcc_driver_bus_prescaler_config_rejects_invalid_ppre1(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status =
        rccBusPrescalerConfig(&rcc, RCC_CFGR_HPRE_DIV1, 0x1U, RCC_CFGR_PPRE_DIV1);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CFGR);
}

/** ppre2 outside its 5 documented divisors must be rejected. */
void test_rcc_driver_bus_prescaler_config_rejects_invalid_ppre2(void)
{
    RccRegisters_t rcc = {0};

    DriverStatus_e status =
        rccBusPrescalerConfig(&rcc, RCC_CFGR_HPRE_DIV1, RCC_CFGR_PPRE_DIV1, 0x1U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CFGR);
}

/** A valid call writes HPRE/PPRE1/PPRE2 and preserves every other CFGR
 *  bit (SW/SWS and reserved bits) untouched. */
void test_rcc_driver_bus_prescaler_config_writes_all_three_fields(void)
{
    uint32_t owned_mask = RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk;
    RccRegisters_t rcc = {.CFGR = 0xFFFFFFFFU & ~owned_mask};

    DriverStatus_e status =
        rccBusPrescalerConfig(&rcc, RCC_CFGR_HPRE_DIV2, RCC_CFGR_PPRE_DIV4, RCC_CFGR_PPRE_DIV2);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_HPRE_DIV2,
                            (rcc.CFGR & RCC_CFGR_HPRE_Msk) >> RCC_CFGR_HPRE_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_PPRE_DIV4,
                            (rcc.CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_PPRE_DIV2,
                            (rcc.CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~owned_mask, rcc.CFGR & ~owned_mask);
}

/** Every one of HPRE's 9 documented values must independently report OK -
 *  rccValidateHpre()'s chained != comparisons short-circuit at a
 *  different clause for each value, so a single valid-value test (as
 *  above) only exercises one clause's false outcome; gcov branch
 *  coverage requires each of the 9 individually. */
void test_rcc_driver_bus_prescaler_config_accepts_every_documented_hpre(void)
{
    static const uint32_t hpre_values[] = {
        RCC_CFGR_HPRE_DIV1,   RCC_CFGR_HPRE_DIV2,   RCC_CFGR_HPRE_DIV4,
        RCC_CFGR_HPRE_DIV8,   RCC_CFGR_HPRE_DIV16,  RCC_CFGR_HPRE_DIV64,
        RCC_CFGR_HPRE_DIV128, RCC_CFGR_HPRE_DIV256, RCC_CFGR_HPRE_DIV512};

    for (size_t i = 0; i < sizeof(hpre_values) / sizeof(hpre_values[0]); i++)
    {
        RccRegisters_t rcc = {0};

        DriverStatus_e status =
            rccBusPrescalerConfig(&rcc, hpre_values[i], RCC_CFGR_PPRE_DIV1, RCC_CFGR_PPRE_DIV1);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    }
}

/** Every one of PPRE1/PPRE2's 5 documented values must independently
 *  report OK - same reasoning as the HPRE case above, applied through
 *  the ppre1 parameter (rccValidatePpre() is shared by both fields, so
 *  this also covers ppre2's identical chain). */
void test_rcc_driver_bus_prescaler_config_accepts_every_documented_ppre(void)
{
    static const uint32_t ppre_values[] = {RCC_CFGR_PPRE_DIV1, RCC_CFGR_PPRE_DIV2,
                                           RCC_CFGR_PPRE_DIV4, RCC_CFGR_PPRE_DIV8,
                                           RCC_CFGR_PPRE_DIV16};

    for (size_t i = 0; i < sizeof(ppre_values) / sizeof(ppre_values[0]); i++)
    {
        RccRegisters_t rcc = {0};

        DriverStatus_e status =
            rccBusPrescalerConfig(&rcc, RCC_CFGR_HPRE_DIV1, ppre_values[i], ppre_values[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    }
}

/* --- rccSysclkSwitch --- */

/** source outside {HSI, HSE, PLL} must be rejected before pwr/flash are
 *  ever touched. */
void test_rcc_driver_sysclk_switch_rejects_invalid_source(void)
{
    RccRegisters_t rcc = {0};
    FlashRegisters_t flash = {0};
    PwrRegisters_t pwr = {0};

    DriverStatus_e status = rccSysclkSwitch(&rcc, &flash, &pwr, 0x3U, PWR_CR_VOS_SCALE1, 5U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, flash.ACR);
    TEST_ASSERT_EQUAL_HEX32(0U, pwr.CR);
}

/** An invalid vos is propagated straight from pwrSetVoltageScale() -
 *  flash is never touched, since pwr is sequenced first. */
void test_rcc_driver_sysclk_switch_propagates_pwr_invalid_param(void)
{
    RccRegisters_t rcc = {0};
    FlashRegisters_t flash = {0};
    PwrRegisters_t pwr = {0};

    DriverStatus_e status = rccSysclkSwitch(&rcc, &flash, &pwr, RCC_CFGR_SYSCLK_PLL, 0x0U, 5U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, flash.ACR);
}

/** An invalid latency is propagated straight from flashSetLatency(),
 *  after pwr has already been applied (VOSRDY pre-set here so pwr
 *  succeeds and the sequence reaches flash). */
void test_rcc_driver_sysclk_switch_propagates_flash_invalid_param(void)
{
    RccRegisters_t rcc = {0};
    FlashRegisters_t flash = {0};
    PwrRegisters_t pwr = {.CSR = PWR_CSR_VOSRDY};

    DriverStatus_e status =
        rccSysclkSwitch(&rcc, &flash, &pwr, RCC_CFGR_SYSCLK_PLL, PWR_CR_VOS_SCALE1, 16U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CFGR);
}

/** vos/latency both succeed, but the requested source's ready bit
 *  (PLLRDY here) was never set - caller forgot to enable/wait for it. */
void test_rcc_driver_sysclk_switch_reports_not_initialized_when_source_not_ready(void)
{
    RccRegisters_t rcc = {0};
    FlashRegisters_t flash = {0};
    PwrRegisters_t pwr = {.CSR = PWR_CSR_VOSRDY};

    DriverStatus_e status =
        rccSysclkSwitch(&rcc, &flash, &pwr, RCC_CFGR_SYSCLK_PLL, PWR_CR_VOS_SCALE1, 5U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_NOT_INITIALIZED, status);
    TEST_ASSERT_EQUAL_HEX32(0U, rcc.CFGR & RCC_CFGR_SW_Msk);
}

/** vos/latency succeed, the source is ready (PLLRDY pre-set), and SWS
 *  (pre-set to already match, simulating the hardware switch completing
 *  immediately) confirms the switch -> OK, with CFGR.SW written. */
void test_rcc_driver_sysclk_switch_switches_and_reports_ok(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_PLLRDY, .CFGR = RCC_CFGR_SYSCLK_PLL << RCC_CFGR_SWS_Pos};
    FlashRegisters_t flash = {0};
    PwrRegisters_t pwr = {.CSR = PWR_CSR_VOSRDY};

    DriverStatus_e status =
        rccSysclkSwitch(&rcc, &flash, &pwr, RCC_CFGR_SYSCLK_PLL, PWR_CR_VOS_SCALE1, 5U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_SYSCLK_PLL, (rcc.CFGR & RCC_CFGR_SW_Msk) >> RCC_CFGR_SW_Pos);
    TEST_ASSERT_EQUAL_HEX32(5U, flash.ACR & FLASH_ACR_LATENCY_Msk);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_VOS_SCALE1, (pwr.CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos);
}

/** Same as above, switching to HSI instead of PLL - exercises the
 *  ready_bit == RCC_CR_HSIRDY branch, not covered by the PLL-only case
 *  above. */
void test_rcc_driver_sysclk_switch_to_hsi_switches_and_reports_ok(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSIRDY, .CFGR = RCC_CFGR_SYSCLK_HSI << RCC_CFGR_SWS_Pos};
    FlashRegisters_t flash = {0};
    PwrRegisters_t pwr = {.CSR = PWR_CSR_VOSRDY};

    DriverStatus_e status =
        rccSysclkSwitch(&rcc, &flash, &pwr, RCC_CFGR_SYSCLK_HSI, PWR_CR_VOS_SCALE3, 0U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_SYSCLK_HSI, (rcc.CFGR & RCC_CFGR_SW_Msk) >> RCC_CFGR_SW_Pos);
}

/** Same as above, switching to HSE instead of PLL - exercises the
 *  ready_bit == RCC_CR_HSERDY branch, not covered by the PLL-only case
 *  above. */
void test_rcc_driver_sysclk_switch_to_hse_switches_and_reports_ok(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_HSERDY, .CFGR = RCC_CFGR_SYSCLK_HSE << RCC_CFGR_SWS_Pos};
    FlashRegisters_t flash = {0};
    PwrRegisters_t pwr = {.CSR = PWR_CSR_VOSRDY};

    DriverStatus_e status =
        rccSysclkSwitch(&rcc, &flash, &pwr, RCC_CFGR_SYSCLK_HSE, PWR_CR_VOS_SCALE2, 2U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_SYSCLK_HSE, (rcc.CFGR & RCC_CFGR_SW_Msk) >> RCC_CFGR_SW_Pos);
}

/** The source is ready, but CFGR.SWS never reports the switch completing
 *  (left at its own reset value here, never updated after CFGR.SW is
 *  written, simulating a hardware fault) -> the bounded retry exhausts
 *  and reports a timeout. */
void test_rcc_driver_sysclk_switch_times_out_when_sws_never_matches(void)
{
    RccRegisters_t rcc = {.CR = RCC_CR_PLLRDY};
    FlashRegisters_t flash = {0};
    PwrRegisters_t pwr = {.CSR = PWR_CSR_VOSRDY};

    DriverStatus_e status =
        rccSysclkSwitch(&rcc, &flash, &pwr, RCC_CFGR_SYSCLK_PLL, PWR_CR_VOS_SCALE1, 5U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(RCC_CFGR_SYSCLK_PLL, (rcc.CFGR & RCC_CFGR_SW_Msk) >> RCC_CFGR_SW_Pos);
}
