/**
 * @file test_iwdg_reg.c
 * @brief Host-side sanity checks for device/inc/iwdg_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "iwdg_reg.h"
#include "unity.h"

/** IwdgRegisters_t must be exactly 0x14 bytes (KR..WINR, RM0390 21.4). */
void
test_iwdg_register_block_size(void)
{
  TEST_ASSERT_EQUAL_UINT(0x14, sizeof(IwdgRegisters_t));
}

/** IWDG_KEY_* values must be distinct 16-bit key codes. */
void
test_iwdg_key_values_are_distinct(void)
{
  TEST_ASSERT_NOT_EQUAL_UINT32(IWDG_KEY_RELOAD, IWDG_KEY_ENABLE_ACCESS);
  TEST_ASSERT_NOT_EQUAL_UINT32(IWDG_KEY_ENABLE_ACCESS, IWDG_KEY_START);
  TEST_ASSERT_NOT_EQUAL_UINT32(IWDG_KEY_RELOAD, IWDG_KEY_START);
}
