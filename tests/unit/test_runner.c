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
extern void test_uart_register_block_size(void);
extern void test_usart_sr_flag_bits_do_not_overlap(void);
extern void test_usart2_base_address(void);
extern void test_nvic_register_block_size_and_offsets(void);
extern void test_nvic_irqn_values_match_startup_vector_table(void);
extern void test_scb_register_block_size_and_offset(void);
extern void test_fpu_register_block_size(void);
extern void test_scb_cfsr_fault_status_byte_fields_do_not_overlap(void);
extern void test_mpu_register_block_size(void);
extern void test_mpu_rasr_ap_values_are_distinct_and_in_range(void);
extern void test_flash_register_block_size(void);

int
main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_gpio_base_addresses_are_evenly_spaced);
  RUN_TEST(test_gpio_register_block_size);
  RUN_TEST(test_rcc_ahb1enr_gpio_enable_bits_do_not_overlap);
  RUN_TEST(test_systick_base_address);
  RUN_TEST(test_uart_register_block_size);
  RUN_TEST(test_usart_sr_flag_bits_do_not_overlap);
  RUN_TEST(test_usart2_base_address);
  RUN_TEST(test_nvic_register_block_size_and_offsets);
  RUN_TEST(test_nvic_irqn_values_match_startup_vector_table);
  RUN_TEST(test_scb_register_block_size_and_offset);
  RUN_TEST(test_fpu_register_block_size);
  RUN_TEST(test_scb_cfsr_fault_status_byte_fields_do_not_overlap);
  RUN_TEST(test_mpu_register_block_size);
  RUN_TEST(test_mpu_rasr_ap_values_are_distinct_and_in_range);
  RUN_TEST(test_flash_register_block_size);
  return UNITY_END();
}
