/**
 * @file test_runner.c
 * @brief Unity entry point aggregating every test_*_reg.c suite in this
 *        directory into one run_tests binary.
 *
 * Each test_<peripheral>_reg.c file owns only its test functions - no
 * main(), no setUp/tearDown - since Unity requires exactly one definition
 * of each per binary. This file supplies both, and RUN_TEST()s every test
 * function via extern declaration rather than an #include, so each test
 * file compiles (and can be reasoned about) as an independent translation
 * unit.
 */
#include "unity.h"

void
setUp(void)
{
}
void
tearDown(void)
{
}

extern void test_gpio_base_addresses_are_evenly_spaced(void);
extern void test_gpio_register_block_size(void);
extern void test_rcc_ahb1enr_gpio_enable_bits_do_not_overlap(void);
extern void test_systick_base_address(void);

int
main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_gpio_base_addresses_are_evenly_spaced);
  RUN_TEST(test_gpio_register_block_size);
  RUN_TEST(test_rcc_ahb1enr_gpio_enable_bits_do_not_overlap);
  RUN_TEST(test_systick_base_address);
  return UNITY_END();
}
