/**
 * @file test_gpio_reg.c
 * @brief Host-side sanity checks for device/inc/gpio_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "gpio_reg.h"
#include "unity.h"

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

/** GpioRegisters_t must be exactly 0x28 bytes (MODER..AFRH, RM0390 8.4). */
void test_gpio_register_block_size(void) { TEST_ASSERT_EQUAL_UINT(0x28, sizeof(GpioRegisters_t)); }
