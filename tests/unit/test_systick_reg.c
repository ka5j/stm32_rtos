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
