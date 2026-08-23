/**
 * @file test_rcc_reg.c
 * @brief Host-side sanity checks for device/inc/rcc_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "rcc_reg.h"
#include "unity.h"

/** RCC_AHB1ENR_GPIOxEN bit positions must be 0..7, one bit each, no overlap. */
void
test_rcc_ahb1enr_gpio_enable_bits_do_not_overlap(void)
{
  uint32_t bits = RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN
                  | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN
                  | RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN;

  TEST_ASSERT_EQUAL_HEX32(0xFFU, bits);
  TEST_ASSERT_EQUAL_UINT32(0U, RCC_AHB1ENR_GPIOAEN_Pos);
  TEST_ASSERT_EQUAL_UINT32(7U, RCC_AHB1ENR_GPIOHEN_Pos);
}
