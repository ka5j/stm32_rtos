/**
 * @file test_uart_reg.c
 * @brief Host-side sanity checks for device/inc/uart_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "uart_reg.h"
#include "unity.h"

/** UartRegisters_t must be exactly 0x1C bytes (SR..GTPR, RM0390 19.6). */
void test_uart_register_block_size(void) { TEST_ASSERT_EQUAL_UINT(0x1C, sizeof(UartRegisters_t)); }

/** USART_SR field bit positions must be 0..7, one bit each, no overlap. */
void test_usart_sr_flag_bits_do_not_overlap(void)
{
  uint32_t bits = USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE | USART_SR_IDLE |
                  USART_SR_RXNE | USART_SR_TC | USART_SR_TXE;

  TEST_ASSERT_EQUAL_HEX32(0xFFU, bits);
}

/** USART2 (Nucleo ST-LINK VCP) sits on APB1, 0x400 below USART3. */
void test_usart2_base_address(void)
{
  TEST_ASSERT_EQUAL_HEX32(0x40004400UL, USART2_BASE);
  TEST_ASSERT_EQUAL_HEX32(0x400, USART3_BASE - USART2_BASE);
}
