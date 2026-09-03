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
