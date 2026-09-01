/**
 * @file test_rcc_reg.c
 * @brief Host-side sanity checks for device/inc/rcc_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "rcc_reg.h"
#include "unity.h"

#include <stddef.h>

/** RCC_AHB1ENR_GPIOxEN bit positions must be 0..7, one bit each, no overlap. */
void test_rcc_ahb1enr_gpio_enable_bits_do_not_overlap(void)
{
    uint32_t bits = RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN
                    | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN
                    | RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN;

    TEST_ASSERT_EQUAL_HEX32(0xFFU, bits);
    TEST_ASSERT_EQUAL_UINT32(0U, RCC_AHB1ENR_GPIOAEN_Pos);
    TEST_ASSERT_EQUAL_UINT32(7U, RCC_AHB1ENR_GPIOHEN_Pos);
}

/** PWREN (APB1) and SYSCFGEN (APB2) must sit at their documented bit
 *  positions - both gate registers (PWR, SYSCFG) other drivers depend on. */
void test_rcc_pwren_and_syscfgen_bit_positions(void)
{
    TEST_ASSERT_EQUAL_UINT32(28U, RCC_APB1ENR_PWREN_Pos);
    TEST_ASSERT_EQUAL_UINT32(28U, RCC_APB1RSTR_PWRRST_Pos);
    TEST_ASSERT_EQUAL_UINT32(14U, RCC_APB2ENR_SYSCFGEN_Pos);
    TEST_ASSERT_EQUAL_UINT32(14U, RCC_APB2RSTR_SYSCFGRST_Pos);
}

/** RCC_CFGR.SW/SWS values must be distinct and fit the 2-bit field. */
void test_rcc_cfgr_sysclk_values_are_distinct_and_in_range(void)
{
    uint32_t values[] = {RCC_CFGR_SYSCLK_HSI, RCC_CFGR_SYSCLK_HSE, RCC_CFGR_SYSCLK_PLL};

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(0x3U, values[i]);
        for (size_t j = i + 1; j < sizeof(values) / sizeof(values[0]); j++)
        {
            TEST_ASSERT_NOT_EQUAL_UINT32(values[i], values[j]);
        }
    }
}

/** RCC_CFGR.HPRE values must be distinct and fit the 4-bit field. */
void test_rcc_cfgr_hpre_values_are_distinct_and_in_range(void)
{
    uint32_t values[] = {RCC_CFGR_HPRE_DIV1,   RCC_CFGR_HPRE_DIV2,   RCC_CFGR_HPRE_DIV4,
                         RCC_CFGR_HPRE_DIV8,   RCC_CFGR_HPRE_DIV16,  RCC_CFGR_HPRE_DIV64,
                         RCC_CFGR_HPRE_DIV128, RCC_CFGR_HPRE_DIV256, RCC_CFGR_HPRE_DIV512};

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(0xFU, values[i]);
        for (size_t j = i + 1; j < sizeof(values) / sizeof(values[0]); j++)
        {
            TEST_ASSERT_NOT_EQUAL_UINT32(values[i], values[j]);
        }
    }
}

/** RCC_CFGR.PPRE1/PPRE2 values must be distinct and fit the 3-bit field. */
void test_rcc_cfgr_ppre_values_are_distinct_and_in_range(void)
{
    uint32_t values[] = {RCC_CFGR_PPRE_DIV1, RCC_CFGR_PPRE_DIV2, RCC_CFGR_PPRE_DIV4,
                         RCC_CFGR_PPRE_DIV8, RCC_CFGR_PPRE_DIV16};

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(0x7U, values[i]);
        for (size_t j = i + 1; j < sizeof(values) / sizeof(values[0]); j++)
        {
            TEST_ASSERT_NOT_EQUAL_UINT32(values[i], values[j]);
        }
    }
}

/** RCC_PLLCFGR.PLLP values must be distinct and fit the 2-bit field. */
void test_rcc_pllcfgr_pllp_values_are_distinct_and_in_range(void)
{
    uint32_t values[] = {RCC_PLLCFGR_PLLP_DIV2, RCC_PLLCFGR_PLLP_DIV4, RCC_PLLCFGR_PLLP_DIV6,
                         RCC_PLLCFGR_PLLP_DIV8};

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(0x3U, values[i]);
        for (size_t j = i + 1; j < sizeof(values) / sizeof(values[0]); j++)
        {
            TEST_ASSERT_NOT_EQUAL_UINT32(values[i], values[j]);
        }
    }
}
