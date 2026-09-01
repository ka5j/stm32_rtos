/**
 * @file test_flash_reg.c
 * @brief Host-side sanity checks for device/inc/flash_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "flash_reg.h"
#include "unity.h"

/** FlashRegisters_t must be exactly 0x1C bytes (ACR..OPTCR1, RM0390 3.7). */
void test_flash_register_block_size(void)
{
  TEST_ASSERT_EQUAL_UINT(0x1C, sizeof(FlashRegisters_t));
}
