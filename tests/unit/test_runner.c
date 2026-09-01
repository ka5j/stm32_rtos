/**
 * @file test_runner.c
 * @brief Unity entry point aggregating every test_*_reg.c suite in this
 *        directory into one run_tests binary.
 *
 * Each test_<peripheral>_reg.c file owns only its test functions, with
 * no main() and no setUp/tearDown, since Unity requires exactly one
 * definition of each per binary. This file supplies both, and invokes
 * RUN_TEST() for every test function via an extern declaration rather
 * than an #include, so each test file compiles, and can be reasoned
 * about, as an independent translation unit.
 */
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

extern void test_gpio_base_addresses_are_evenly_spaced(void);
extern void test_gpio_register_block_size(void);
extern void test_gpio_driver_init_rejects_invalid_mode(void);
extern void test_gpio_driver_init_rejects_invalid_otype(void);
extern void test_gpio_driver_init_rejects_invalid_ospeed(void);
extern void test_gpio_driver_init_rejects_reserved_pupd(void);
extern void test_gpio_driver_init_sets_fields_for_specified_pin_only(void);
extern void test_gpio_driver_init_does_not_touch_afr(void);
extern void test_gpio_driver_deinit_clears_fields_for_specified_pin_only(void);
extern void test_gpio_driver_deinit_clears_afrl_for_low_pins(void);
extern void test_gpio_driver_deinit_clears_afrh_for_high_pins(void);
extern void test_gpio_driver_set_alternate_function_rejects_invalid_af(void);
extern void test_gpio_driver_set_alternate_function_writes_afrl_for_low_pins(void);
extern void test_gpio_driver_set_alternate_function_writes_afrh_for_high_pins(void);
extern void test_gpio_driver_write_pin_set_uses_bsrr_set_half(void);
extern void test_gpio_driver_write_pin_reset_uses_bsrr_reset_half(void);
extern void test_gpio_driver_read_pin_reports_set_when_idr_bit_high(void);
extern void test_gpio_driver_read_pin_reports_reset_when_idr_bit_low(void);
extern void test_gpio_driver_toggle_pin_drives_high_when_odr_bit_low(void);
extern void test_gpio_driver_toggle_pin_drives_low_when_odr_bit_high(void);
extern void test_rcc_ahb1enr_gpio_enable_bits_do_not_overlap(void);
extern void test_rcc_pwren_and_syscfgen_bit_positions(void);
extern void test_rcc_cfgr_sysclk_values_are_distinct_and_in_range(void);
extern void test_rcc_cfgr_hpre_values_are_distinct_and_in_range(void);
extern void test_rcc_cfgr_ppre_values_are_distinct_and_in_range(void);
extern void test_rcc_pllcfgr_pllp_values_are_distinct_and_in_range(void);
extern void test_systick_base_address(void);
extern void test_uart_register_block_size(void);
extern void test_usart_sr_flag_bits_do_not_overlap(void);
extern void test_usart2_base_address(void);
extern void test_nvic_register_block_size_and_offsets(void);
extern void test_scb_register_block_size_and_offset(void);
extern void test_fpu_register_block_size(void);
extern void test_scb_cfsr_fault_status_byte_fields_do_not_overlap(void);
extern void test_mpu_register_block_size(void);
extern void test_mpu_rasr_ap_values_are_distinct_and_in_range(void);
extern void test_flash_register_block_size(void);
extern void test_pwr_register_block_size(void);
extern void test_pwr_cr_vos_scale_values_are_distinct(void);
extern void test_iwdg_register_block_size(void);
extern void test_iwdg_key_values_are_distinct(void);
extern void test_wwdg_register_block_size(void);
extern void test_exti_register_block_size(void);
extern void test_exti_line_numbers_are_distinct_and_above_gpio_range(void);
extern void test_syscfg_register_block_size(void);
extern void test_syscfg_exticr_port_values_are_distinct_and_in_range(void);

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_gpio_base_addresses_are_evenly_spaced);
    RUN_TEST(test_gpio_register_block_size);
    RUN_TEST(test_gpio_driver_init_rejects_invalid_mode);
    RUN_TEST(test_gpio_driver_init_rejects_invalid_otype);
    RUN_TEST(test_gpio_driver_init_rejects_invalid_ospeed);
    RUN_TEST(test_gpio_driver_init_rejects_reserved_pupd);
    RUN_TEST(test_gpio_driver_init_sets_fields_for_specified_pin_only);
    RUN_TEST(test_gpio_driver_init_does_not_touch_afr);
    RUN_TEST(test_gpio_driver_deinit_clears_fields_for_specified_pin_only);
    RUN_TEST(test_gpio_driver_deinit_clears_afrl_for_low_pins);
    RUN_TEST(test_gpio_driver_deinit_clears_afrh_for_high_pins);
    RUN_TEST(test_gpio_driver_set_alternate_function_rejects_invalid_af);
    RUN_TEST(test_gpio_driver_set_alternate_function_writes_afrl_for_low_pins);
    RUN_TEST(test_gpio_driver_set_alternate_function_writes_afrh_for_high_pins);
    RUN_TEST(test_gpio_driver_write_pin_set_uses_bsrr_set_half);
    RUN_TEST(test_gpio_driver_write_pin_reset_uses_bsrr_reset_half);
    RUN_TEST(test_gpio_driver_read_pin_reports_set_when_idr_bit_high);
    RUN_TEST(test_gpio_driver_read_pin_reports_reset_when_idr_bit_low);
    RUN_TEST(test_gpio_driver_toggle_pin_drives_high_when_odr_bit_low);
    RUN_TEST(test_gpio_driver_toggle_pin_drives_low_when_odr_bit_high);
    RUN_TEST(test_rcc_ahb1enr_gpio_enable_bits_do_not_overlap);
    RUN_TEST(test_rcc_pwren_and_syscfgen_bit_positions);
    RUN_TEST(test_rcc_cfgr_sysclk_values_are_distinct_and_in_range);
    RUN_TEST(test_rcc_cfgr_hpre_values_are_distinct_and_in_range);
    RUN_TEST(test_rcc_cfgr_ppre_values_are_distinct_and_in_range);
    RUN_TEST(test_rcc_pllcfgr_pllp_values_are_distinct_and_in_range);
    RUN_TEST(test_systick_base_address);
    RUN_TEST(test_uart_register_block_size);
    RUN_TEST(test_usart_sr_flag_bits_do_not_overlap);
    RUN_TEST(test_usart2_base_address);
    RUN_TEST(test_nvic_register_block_size_and_offsets);
    RUN_TEST(test_scb_register_block_size_and_offset);
    RUN_TEST(test_fpu_register_block_size);
    RUN_TEST(test_scb_cfsr_fault_status_byte_fields_do_not_overlap);
    RUN_TEST(test_mpu_register_block_size);
    RUN_TEST(test_mpu_rasr_ap_values_are_distinct_and_in_range);
    RUN_TEST(test_flash_register_block_size);
    RUN_TEST(test_pwr_register_block_size);
    RUN_TEST(test_pwr_cr_vos_scale_values_are_distinct);
    RUN_TEST(test_iwdg_register_block_size);
    RUN_TEST(test_iwdg_key_values_are_distinct);
    RUN_TEST(test_wwdg_register_block_size);
    RUN_TEST(test_exti_register_block_size);
    RUN_TEST(test_exti_line_numbers_are_distinct_and_above_gpio_range);
    RUN_TEST(test_syscfg_register_block_size);
    RUN_TEST(test_syscfg_exticr_port_values_are_distinct_and_in_range);
    return UNITY_END();
}
