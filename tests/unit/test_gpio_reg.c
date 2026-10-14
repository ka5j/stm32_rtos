/**
 * @file test_gpio_reg.c
 * @brief Host-side sanity checks for device/inc/gpio_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "gpio_reg.h"
#include "unity.h"

#include <stddef.h>

/** GPIOA..GPIOH base addresses must be 0x400 apart, matching RM0390. */
void test_gpio_base_addresses_are_evenly_spaced(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x400, GPIOB_BASE - GPIOA_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x400, GPIOC_BASE - GPIOB_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x400, GPIOD_BASE - GPIOC_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x400, GPIOE_BASE - GPIOD_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x400, GPIOF_BASE - GPIOE_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x400, GPIOG_BASE - GPIOF_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x400, GPIOH_BASE - GPIOG_BASE);
}

/** GpioRegisters_t must be exactly 0x28 bytes (MODER..AFRH, RM0390 7.4). */
void test_gpio_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x28, sizeof(GpioRegisters_t));
}

/** Literal bases, offsets and field encodings, written out (from ST's SVD and HAL) rather than
 * built from the header's macros: a swapped IDR/ODR, a shifted bank, or GPIO_MODE_AF and
 * GPIO_PUPD_DOWN (both 0x2, in different registers) exchanged for each other would otherwise pass.
 */
void test_gpio_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40020000UL, GPIOA_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x40021C00UL, GPIOH_BASE);
    TEST_ASSERT_EQUAL_HEX32(GPIOA_BASE, (uintptr_t)GPIOA);
    TEST_ASSERT_EQUAL_HEX32(GPIOB_BASE, (uintptr_t)GPIOB);
    TEST_ASSERT_EQUAL_HEX32(GPIOC_BASE, (uintptr_t)GPIOC);
    TEST_ASSERT_EQUAL_HEX32(GPIOD_BASE, (uintptr_t)GPIOD);
    TEST_ASSERT_EQUAL_HEX32(GPIOE_BASE, (uintptr_t)GPIOE);
    TEST_ASSERT_EQUAL_HEX32(GPIOF_BASE, (uintptr_t)GPIOF);
    TEST_ASSERT_EQUAL_HEX32(GPIOG_BASE, (uintptr_t)GPIOG);
    TEST_ASSERT_EQUAL_HEX32(GPIOH_BASE, (uintptr_t)GPIOH);

    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(GpioRegisters_t, OTYPER));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(GpioRegisters_t, OSPEEDR));
    TEST_ASSERT_EQUAL_HEX32(0x0C, offsetof(GpioRegisters_t, PUPDR));
    TEST_ASSERT_EQUAL_HEX32(0x10, offsetof(GpioRegisters_t, IDR));
    TEST_ASSERT_EQUAL_HEX32(0x14, offsetof(GpioRegisters_t, ODR));
    TEST_ASSERT_EQUAL_HEX32(0x18, offsetof(GpioRegisters_t, BSRR));
    TEST_ASSERT_EQUAL_HEX32(0x1C, offsetof(GpioRegisters_t, LCKR));
    TEST_ASSERT_EQUAL_HEX32(0x20, offsetof(GpioRegisters_t, AFRL));
    TEST_ASSERT_EQUAL_HEX32(0x24, offsetof(GpioRegisters_t, AFRH));

    TEST_ASSERT_EQUAL_HEX32(0x0U, GPIO_MODE_INPUT);
    TEST_ASSERT_EQUAL_HEX32(0x1U, GPIO_MODE_OUTPUT);
    TEST_ASSERT_EQUAL_HEX32(0x2U, GPIO_MODE_AF);
    TEST_ASSERT_EQUAL_HEX32(0x3U, GPIO_MODE_ANALOG);
    TEST_ASSERT_EQUAL_HEX32(0x0U, GPIO_OTYPE_PP);
    TEST_ASSERT_EQUAL_HEX32(0x1U, GPIO_OTYPE_OD);
    TEST_ASSERT_EQUAL_HEX32(0x0U, GPIO_OSPEED_LOW);
    TEST_ASSERT_EQUAL_HEX32(0x1U, GPIO_OSPEED_MEDIUM);
    TEST_ASSERT_EQUAL_HEX32(0x2U, GPIO_OSPEED_FAST);
    TEST_ASSERT_EQUAL_HEX32(0x3U, GPIO_OSPEED_HIGH);
    TEST_ASSERT_EQUAL_HEX32(0x0U, GPIO_PUPD_NONE);
    TEST_ASSERT_EQUAL_HEX32(0x1U, GPIO_PUPD_UP);
    TEST_ASSERT_EQUAL_HEX32(0x2U, GPIO_PUPD_DOWN);
}
