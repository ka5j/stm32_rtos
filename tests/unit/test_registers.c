/**
 * @file test_registers.c
 * @brief Host-side sanity checks for the register-layer headers (core/inc,
 *        device/inc). Pure data validation - no hardware, no driver logic.
 *        Runs natively via `make test`, not cross-compiled.
 */
#include "gpio_reg.h"
#include "rcc_reg.h"
#include "systick_reg.h"
#include "uart_reg.h"
#include "unity.h"

void
setUp(void)
{
}
void
tearDown(void)
{
}

/** GPIOA..GPIOH base addresses must be 0x400 apart, matching RM0390. */
void
test_gpio_base_addresses_are_evenly_spaced(void)
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
void
test_gpio_register_block_size(void)
{
  TEST_ASSERT_EQUAL_UINT(0x28, sizeof(GpioRegisters_t));
}

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

/** SysTick base address is architecturally fixed by Armv7-M, not vendor-specific. */
void
test_systick_base_address(void)
{
  TEST_ASSERT_EQUAL_HEX32(0xE000E010UL, SYSTICK_BASE);
}

/** UartRegisters_t must be exactly 0x1C bytes (SR..GTPR, RM0390 19.6). */
void
test_uart_register_block_size(void)
{
  TEST_ASSERT_EQUAL_UINT(0x1C, sizeof(UartRegisters_t));
}

/** USART_SR field bit positions must be 0..7, one bit each, no overlap. */
void
test_usart_sr_flag_bits_do_not_overlap(void)
{
  uint32_t bits = USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE | USART_SR_IDLE
                  | USART_SR_RXNE | USART_SR_TC | USART_SR_TXE;

  TEST_ASSERT_EQUAL_HEX32(0xFFU, bits);
}

/** USART2 (Nucleo ST-LINK VCP) sits on APB1, 0x400 below USART3. */
void
test_usart2_base_address(void)
{
  TEST_ASSERT_EQUAL_HEX32(0x40004400UL, USART2_BASE);
  TEST_ASSERT_EQUAL_HEX32(0x400, USART3_BASE - USART2_BASE);
}

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
  return UNITY_END();
}
