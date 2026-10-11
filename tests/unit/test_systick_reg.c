/**
 * @file test_systick_reg.c
 * @brief Host-side sanity checks for core/inc/systick_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "systick_reg.h"
#include "unity.h"

/** SysTick base address is architecturally fixed by Armv7-M, not vendor-specific. */
void test_systick_base_address(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xE000E010UL, SYSTICK_BASE);
}

/** SysTick's LOAD/VAL/CALIB fields are 24 bits wide, so a reload value must fit 0x00FFFFFF. */
void test_systick_24bit_fields(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x00FFFFFFU, SYSTICK_LOAD_RELOAD_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00FFFFFFU, SYSTICK_VAL_CURRENT_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00FFFFFFU, SYSTICK_CALIB_TENMS_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00U,
                            SYSTICK_CALIB_TENMS_Msk & (SYSTICK_CALIB_SKEW | SYSTICK_CALIB_NOREF));
    TEST_ASSERT_EQUAL_HEX32(1UL << 30, SYSTICK_CALIB_SKEW);
    TEST_ASSERT_EQUAL_HEX32(1UL << 31, SYSTICK_CALIB_NOREF);
}
