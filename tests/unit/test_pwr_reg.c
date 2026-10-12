/**
 * @file test_pwr_reg.c
 * @brief Host-side sanity checks for device/inc/pwr_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "pwr_reg.h"
#include "unity.h"

#include <stddef.h>

/** PwrRegisters_t must be exactly 0x08 bytes (CR, CSR, RM0390 5.4). */
void test_pwr_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x08, sizeof(PwrRegisters_t));
}

/** PWR_CR.VOS scale values must be distinct and fit the 2-bit field. */
void test_pwr_cr_vos_scale_values_are_distinct(void)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(0x3U, PWR_CR_VOS_SCALE1);
    TEST_ASSERT_NOT_EQUAL_UINT32(PWR_CR_VOS_SCALE1, PWR_CR_VOS_SCALE2);
    TEST_ASSERT_NOT_EQUAL_UINT32(PWR_CR_VOS_SCALE2, PWR_CR_VOS_SCALE3);
    TEST_ASSERT_NOT_EQUAL_UINT32(PWR_CR_VOS_SCALE1, PWR_CR_VOS_SCALE3);
}

/** Literal values for the encodings and addresses the PWR driver relies on. These are written out
 *  (from ST's SVD and HAL) rather than built from the header's own macros, so a self-consistent
 *  edit - for example swapping SCALE1 and SCALE3, which would under-volt the 180 MHz bring-up -
 *  cannot pass. */
void test_pwr_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40007000UL, PWR_BASE);
    TEST_ASSERT_EQUAL_HEX32(PWR_BASE, (uintptr_t)PWR);
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(PwrRegisters_t, CSR));

    TEST_ASSERT_EQUAL_HEX32(0x1U, PWR_CR_VOS_SCALE3);
    TEST_ASSERT_EQUAL_HEX32(0x2U, PWR_CR_VOS_SCALE2);
    TEST_ASSERT_EQUAL_HEX32(0x3U, PWR_CR_VOS_SCALE1);
    TEST_ASSERT_EQUAL_HEX32(0x0000C000U, PWR_CR_VOS_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x000000E0U, PWR_CR_PLS_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00010000U, PWR_CR_ODEN);
    TEST_ASSERT_EQUAL_HEX32(0x00020000U, PWR_CR_ODSWEN);
    TEST_ASSERT_EQUAL_HEX32(0x00004000U, PWR_CSR_VOSRDY);
    TEST_ASSERT_EQUAL_HEX32(0x00010000U, PWR_CSR_ODRDY);
    TEST_ASSERT_EQUAL_HEX32(0x00020000U, PWR_CSR_ODSWRDY);
}
