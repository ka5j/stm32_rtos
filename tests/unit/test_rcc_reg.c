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

/** Each GPIO port's clock-enable bit at its own documented position
 *  (RM0390 RCC_AHB1ENR: GPIOAEN bit 0 ... GPIOHEN bit 7). The no-overlap
 *  test above only proves the eight bits fill 0-7 in some order, and the
 *  port-to-bit mapping test in test_rcc.c builds both sides from these same
 *  macros, so a swap of two ports would pass both. */
void test_rcc_ahb1enr_gpio_enable_masks_are_pinned_individually(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x01U, RCC_AHB1ENR_GPIOAEN);
    TEST_ASSERT_EQUAL_HEX32(0x02U, RCC_AHB1ENR_GPIOBEN);
    TEST_ASSERT_EQUAL_HEX32(0x04U, RCC_AHB1ENR_GPIOCEN);
    TEST_ASSERT_EQUAL_HEX32(0x08U, RCC_AHB1ENR_GPIODEN);
    TEST_ASSERT_EQUAL_HEX32(0x10U, RCC_AHB1ENR_GPIOEEN);
    TEST_ASSERT_EQUAL_HEX32(0x20U, RCC_AHB1ENR_GPIOFEN);
    TEST_ASSERT_EQUAL_HEX32(0x40U, RCC_AHB1ENR_GPIOGEN);
    TEST_ASSERT_EQUAL_HEX32(0x80U, RCC_AHB1ENR_GPIOHEN);
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

/** Literal sizes, offsets, masks and encodings for the RCC registers the drivers use, written out
 *  (from ST's SVD and HAL) rather than built from the header's own macros. RCC has seven RESERVED
 *  gaps, so every member after one is pinned: a shifted member would otherwise pass. */
void test_rcc_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40023800UL, RCC_BASE);
    TEST_ASSERT_EQUAL_HEX32(RCC_BASE, (uintptr_t)RCC);
    TEST_ASSERT_EQUAL_UINT(0x98, sizeof(RccRegisters_t));
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(RccRegisters_t, PLLCFGR));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(RccRegisters_t, CFGR));
    TEST_ASSERT_EQUAL_HEX32(0x10, offsetof(RccRegisters_t, AHB1RSTR));
    TEST_ASSERT_EQUAL_HEX32(0x20, offsetof(RccRegisters_t, APB1RSTR));
    TEST_ASSERT_EQUAL_HEX32(0x24, offsetof(RccRegisters_t, APB2RSTR));
    TEST_ASSERT_EQUAL_HEX32(0x30, offsetof(RccRegisters_t, AHB1ENR));
    TEST_ASSERT_EQUAL_HEX32(0x40, offsetof(RccRegisters_t, APB1ENR));
    TEST_ASSERT_EQUAL_HEX32(0x44, offsetof(RccRegisters_t, APB2ENR));
    TEST_ASSERT_EQUAL_HEX32(0x50, offsetof(RccRegisters_t, AHB1LPENR));
    TEST_ASSERT_EQUAL_HEX32(0x60, offsetof(RccRegisters_t, APB1LPENR));
    TEST_ASSERT_EQUAL_HEX32(0x70, offsetof(RccRegisters_t, BDCR));
    TEST_ASSERT_EQUAL_HEX32(0x74, offsetof(RccRegisters_t, CSR));
    TEST_ASSERT_EQUAL_HEX32(0x80, offsetof(RccRegisters_t, SSCGR));
    TEST_ASSERT_EQUAL_HEX32(0x94, offsetof(RccRegisters_t, DCKCFGR2));

    TEST_ASSERT_EQUAL_HEX32(0x00000001U, RCC_CR_HSION);
    TEST_ASSERT_EQUAL_HEX32(0x00000002U, RCC_CR_HSIRDY);
    TEST_ASSERT_EQUAL_HEX32(0x00010000U, RCC_CR_HSEON);
    TEST_ASSERT_EQUAL_HEX32(0x00020000U, RCC_CR_HSERDY);
    TEST_ASSERT_EQUAL_HEX32(0x00040000U, RCC_CR_HSEBYP);
    TEST_ASSERT_EQUAL_HEX32(0x01000000U, RCC_CR_PLLON);
    TEST_ASSERT_EQUAL_HEX32(0x02000000U, RCC_CR_PLLRDY);

    TEST_ASSERT_EQUAL_HEX32(0x0000003FU, RCC_PLLCFGR_PLLM_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00007FC0U, RCC_PLLCFGR_PLLN_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00030000U, RCC_PLLCFGR_PLLP_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x0U, RCC_PLLCFGR_PLLSRC_HSI);
    TEST_ASSERT_EQUAL_HEX32(0x00400000U, RCC_PLLCFGR_PLLSRC_HSE);
    TEST_ASSERT_EQUAL_HEX32(0x0U, RCC_PLLCFGR_PLLP_DIV2);
    TEST_ASSERT_EQUAL_HEX32(0x1U, RCC_PLLCFGR_PLLP_DIV4);
    TEST_ASSERT_EQUAL_HEX32(0x2U, RCC_PLLCFGR_PLLP_DIV6);
    TEST_ASSERT_EQUAL_HEX32(0x3U, RCC_PLLCFGR_PLLP_DIV8);

    TEST_ASSERT_EQUAL_HEX32(0x00000003U, RCC_CFGR_SW_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x0000000CU, RCC_CFGR_SWS_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x000000F0U, RCC_CFGR_HPRE_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00001C00U, RCC_CFGR_PPRE1_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x0000E000U, RCC_CFGR_PPRE2_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x0U, RCC_CFGR_SYSCLK_HSI);
    TEST_ASSERT_EQUAL_HEX32(0x1U, RCC_CFGR_SYSCLK_HSE);
    TEST_ASSERT_EQUAL_HEX32(0x2U, RCC_CFGR_SYSCLK_PLL);

    TEST_ASSERT_EQUAL_HEX32(0x0U, RCC_CFGR_HPRE_DIV1);
    TEST_ASSERT_EQUAL_HEX32(0x8U, RCC_CFGR_HPRE_DIV2);
    TEST_ASSERT_EQUAL_HEX32(0x9U, RCC_CFGR_HPRE_DIV4);
    TEST_ASSERT_EQUAL_HEX32(0xAU, RCC_CFGR_HPRE_DIV8);
    TEST_ASSERT_EQUAL_HEX32(0xBU, RCC_CFGR_HPRE_DIV16);
    TEST_ASSERT_EQUAL_HEX32(0xCU, RCC_CFGR_HPRE_DIV64);
    TEST_ASSERT_EQUAL_HEX32(0xDU, RCC_CFGR_HPRE_DIV128);
    TEST_ASSERT_EQUAL_HEX32(0xEU, RCC_CFGR_HPRE_DIV256);
    TEST_ASSERT_EQUAL_HEX32(0xFU, RCC_CFGR_HPRE_DIV512);
    TEST_ASSERT_EQUAL_HEX32(0x0U, RCC_CFGR_PPRE_DIV1);
    TEST_ASSERT_EQUAL_HEX32(0x4U, RCC_CFGR_PPRE_DIV2);
    TEST_ASSERT_EQUAL_HEX32(0x5U, RCC_CFGR_PPRE_DIV4);
    TEST_ASSERT_EQUAL_HEX32(0x6U, RCC_CFGR_PPRE_DIV8);
    TEST_ASSERT_EQUAL_HEX32(0x7U, RCC_CFGR_PPRE_DIV16);

    TEST_ASSERT_EQUAL_HEX32(0x00000001U, RCC_AHB1ENR_GPIOAEN);
    TEST_ASSERT_EQUAL_HEX32(0x00000080U, RCC_AHB1ENR_GPIOHEN);
    TEST_ASSERT_EQUAL_HEX32(0x00020000U, RCC_APB1ENR_USART2EN);
    TEST_ASSERT_EQUAL_HEX32(0x10000000U, RCC_APB1ENR_PWREN);
    TEST_ASSERT_EQUAL_HEX32(0x00004000U, RCC_APB2ENR_SYSCFGEN);
}
