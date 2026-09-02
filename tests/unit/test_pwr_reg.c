/**
 * @file test_pwr_reg.c
 * @brief Host-side sanity checks for device/inc/pwr_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "pwr_reg.h"
#include "unity.h"

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
