/**
 * @file test_wwdg_reg.c
 * @brief Host-side sanity checks for device/inc/wwdg_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "unity.h"

#include "wwdg_reg.h"
#include <stddef.h>

/** WwdgRegisters_t must be exactly 0x0C bytes (CR, CFR, SR, RM0390 20.6). */
void test_wwdg_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x0C, sizeof(WwdgRegisters_t));
}

/** Literal base and offsets (ST's SVD). */
void test_wwdg_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40002C00UL, WWDG_BASE);
    TEST_ASSERT_EQUAL_HEX32(WWDG_BASE, (uintptr_t)WWDG);
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(WwdgRegisters_t, CFR));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(WwdgRegisters_t, SR));
}
