/**
 * @file test_wwdg_reg.c
 * @brief Host-side sanity checks for device/inc/wwdg_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "unity.h"

#include "wwdg_reg.h"
#include <stddef.h>

/** WwdgRegisters_t must be exactly 0x0C bytes (CR, CFR, SR, RM0390 21.6). */
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

/** CR, CFR and SR fields at their RM0390 positions, and the four timer-base values. */
void test_wwdg_reg_fields_and_timer_base_values(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x0000007FU, WWDG_CR_T_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00000080U, WWDG_CR_WDGA);
    TEST_ASSERT_EQUAL_HEX32(0x0000007FU, WWDG_CFR_W_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00000180U, WWDG_CFR_WDGTB_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00000200U, WWDG_CFR_EWI);
    TEST_ASSERT_EQUAL_HEX32(0x00000001U, WWDG_SR_EWIF);
    TEST_ASSERT_EQUAL_UINT32(0U, WWDG_CFR_WDGTB_DIV1);
    TEST_ASSERT_EQUAL_UINT32(1U, WWDG_CFR_WDGTB_DIV2);
    TEST_ASSERT_EQUAL_UINT32(2U, WWDG_CFR_WDGTB_DIV4);
    TEST_ASSERT_EQUAL_UINT32(3U, WWDG_CFR_WDGTB_DIV8);
}
