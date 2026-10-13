/**
 * @file test_uart_reg.c
 * @brief Host-side sanity checks for device/inc/uart_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "uart_reg.h"
#include "unity.h"

#include <stddef.h>

/** UartRegisters_t must be exactly 0x1C bytes (SR..GTPR, RM0390 25.6). */
void test_uart_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x1C, sizeof(UartRegisters_t));
}

/** USART_SR field bit positions must be 0..7, one bit each, no overlap. */
void test_usart_sr_flag_bits_do_not_overlap(void)
{
    uint32_t bits = USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE | USART_SR_IDLE
                    | USART_SR_RXNE | USART_SR_TC | USART_SR_TXE;

    TEST_ASSERT_EQUAL_HEX32(0xFFU, bits);
}

/** Each USART_SR flag at its own documented bit (RM0390 USART_SR). The
 *  no-overlap test above only proves the eight flags fill bits 0-7 in some
 *  order, which would also pass with ORE and IDLE swapped - and
 *  uartReceiveByte() decides what counts as a receive fault from these. */
void test_usart_sr_flag_masks_are_pinned_individually(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x01U, USART_SR_PE);
    TEST_ASSERT_EQUAL_HEX32(0x02U, USART_SR_FE);
    TEST_ASSERT_EQUAL_HEX32(0x04U, USART_SR_NE);
    TEST_ASSERT_EQUAL_HEX32(0x08U, USART_SR_ORE);
    TEST_ASSERT_EQUAL_HEX32(0x10U, USART_SR_IDLE);
    TEST_ASSERT_EQUAL_HEX32(0x20U, USART_SR_RXNE);
    TEST_ASSERT_EQUAL_HEX32(0x40U, USART_SR_TC);
    TEST_ASSERT_EQUAL_HEX32(0x80U, USART_SR_TXE);
}

/** USART2 (Nucleo ST-LINK VCP) sits on APB1, 0x400 below USART3. */
void test_usart2_base_address(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40004400UL, USART2_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x400, USART3_BASE - USART2_BASE);
}

/** Literal bases, offsets, masks and CR1/CR2 bit positions, written out (from ST's SVD and HAL). */
void test_uart_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40011000UL, USART1_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x40004400UL, USART2_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x40004800UL, USART3_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x40004C00UL, UART4_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x40005000UL, UART5_BASE);
    TEST_ASSERT_EQUAL_HEX32(0x40011400UL, USART6_BASE);
    TEST_ASSERT_EQUAL_HEX32(USART1_BASE, (uintptr_t)USART1);
    TEST_ASSERT_EQUAL_HEX32(USART2_BASE, (uintptr_t)USART2);
    TEST_ASSERT_EQUAL_HEX32(USART3_BASE, (uintptr_t)USART3);
    TEST_ASSERT_EQUAL_HEX32(UART4_BASE, (uintptr_t)UART4);
    TEST_ASSERT_EQUAL_HEX32(UART5_BASE, (uintptr_t)UART5);
    TEST_ASSERT_EQUAL_HEX32(USART6_BASE, (uintptr_t)USART6);

    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(UartRegisters_t, DR));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(UartRegisters_t, BRR));
    TEST_ASSERT_EQUAL_HEX32(0x0C, offsetof(UartRegisters_t, CR1));
    TEST_ASSERT_EQUAL_HEX32(0x10, offsetof(UartRegisters_t, CR2));
    TEST_ASSERT_EQUAL_HEX32(0x14, offsetof(UartRegisters_t, CR3));
    TEST_ASSERT_EQUAL_HEX32(0x18, offsetof(UartRegisters_t, GTPR));

    TEST_ASSERT_EQUAL_HEX32(0x00000020U, USART_SR_RXNE);
    TEST_ASSERT_EQUAL_HEX32(0x00000040U, USART_SR_TC);
    TEST_ASSERT_EQUAL_HEX32(0x00000080U, USART_SR_TXE);
    TEST_ASSERT_EQUAL_HEX32(0x00000004U, USART_CR1_RE);
    TEST_ASSERT_EQUAL_HEX32(0x00000008U, USART_CR1_TE);
    TEST_ASSERT_EQUAL_HEX32(0x00000200U, USART_CR1_PS);
    TEST_ASSERT_EQUAL_HEX32(0x00000400U, USART_CR1_PCE);
    TEST_ASSERT_EQUAL_HEX32(0x00001000U, USART_CR1_M);
    TEST_ASSERT_EQUAL_HEX32(0x00002000U, USART_CR1_UE);
    TEST_ASSERT_EQUAL_HEX32(0x00008000U, USART_CR1_OVER8);
    TEST_ASSERT_EQUAL_HEX32(0x00003000U, USART_CR2_STOP_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x0U, USART_CR2_STOP_1);
    TEST_ASSERT_EQUAL_HEX32(0x1U, USART_CR2_STOP_0_5);
    TEST_ASSERT_EQUAL_HEX32(0x2U, USART_CR2_STOP_2);
    TEST_ASSERT_EQUAL_HEX32(0x3U, USART_CR2_STOP_1_5);
    TEST_ASSERT_EQUAL_HEX32(0x0000FFF0U, USART_BRR_DIV_MANTISSA_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x0000000FU, USART_BRR_DIV_FRACTION_Msk);
}
