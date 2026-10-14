/**
 * @file test_dbgmcu_reg.c
 * @brief Host-side sanity checks for device/inc/dbgmcu_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "dbgmcu_reg.h"
#include "unity.h"

#include <stddef.h>

/** Literal base, block size, offsets and freeze bit (ST's SVD and stm32f446xx.h). */
void test_dbgmcu_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xE0042000UL, DBGMCU_BASE);
    TEST_ASSERT_EQUAL_HEX32(DBGMCU_BASE, (uintptr_t)DBGMCU);
    TEST_ASSERT_EQUAL_UINT(0x10, sizeof(DbgmcuRegisters_t));
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(DbgmcuRegisters_t, CR));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(DbgmcuRegisters_t, APB1_FZ));
    TEST_ASSERT_EQUAL_HEX32(0x0C, offsetof(DbgmcuRegisters_t, APB2_FZ));
    TEST_ASSERT_EQUAL_HEX32(0x00001000U, DBGMCU_APB1_FZ_DBG_IWDG_STOP);
}
