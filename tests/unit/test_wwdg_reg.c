/**
 * @file test_wwdg_reg.c
 * @brief Host-side sanity checks for device/inc/wwdg_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "unity.h"
#include "wwdg_reg.h"

/** WwdgRegisters_t must be exactly 0x0C bytes (CR, CFR, SR, RM0390 20.6). */
void test_wwdg_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x0C, sizeof(WwdgRegisters_t));
}
