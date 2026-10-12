/**
 * @file test_exti_reg.c
 * @brief Host-side sanity checks for device/inc/exti_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "exti_reg.h"
#include "unity.h"

#include <stddef.h>

/** ExtiRegisters_t must be exactly 0x18 bytes (IMR..PR, RM0390 12.3). */
void test_exti_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x18, sizeof(ExtiRegisters_t));
}

/** Fixed EXTI line numbers must be distinct and outside the GPIO 0-15 range. */
void test_exti_line_numbers_are_distinct_and_above_gpio_range(void)
{
    uint32_t lines[] = {EXTI_LINE_PVD,         EXTI_LINE_RTC_ALARM,  EXTI_LINE_OTG_FS_WKUP,
                        EXTI_LINE_OTG_HS_WKUP, EXTI_LINE_TAMP_STAMP, EXTI_LINE_RTC_WKUP};

    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++)
    {
        TEST_ASSERT_GREATER_THAN_UINT32(15U, lines[i]);
        for (size_t j = i + 1; j < sizeof(lines) / sizeof(lines[0]); j++)
        {
            TEST_ASSERT_NOT_EQUAL_UINT32(lines[i], lines[j]);
        }
    }
}

/** Literal base and offsets (ST's SVD), and the fixed EXTI line numbers 16-22 as the RM documents
 * them (not in the SVD or ST's headers, so these are regression pins, not a second source). */
void test_exti_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40013C00UL, EXTI_BASE);
    TEST_ASSERT_EQUAL_HEX32(EXTI_BASE, (uintptr_t)EXTI);
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(ExtiRegisters_t, EMR));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(ExtiRegisters_t, RTSR));
    TEST_ASSERT_EQUAL_HEX32(0x0C, offsetof(ExtiRegisters_t, FTSR));
    TEST_ASSERT_EQUAL_HEX32(0x10, offsetof(ExtiRegisters_t, SWIER));
    TEST_ASSERT_EQUAL_HEX32(0x14, offsetof(ExtiRegisters_t, PR));
    TEST_ASSERT_EQUAL_UINT(16U, EXTI_LINE_PVD);
    TEST_ASSERT_EQUAL_UINT(17U, EXTI_LINE_RTC_ALARM);
    TEST_ASSERT_EQUAL_UINT(18U, EXTI_LINE_OTG_FS_WKUP);
    TEST_ASSERT_EQUAL_UINT(20U, EXTI_LINE_OTG_HS_WKUP);
    TEST_ASSERT_EQUAL_UINT(21U, EXTI_LINE_TAMP_STAMP);
    TEST_ASSERT_EQUAL_UINT(22U, EXTI_LINE_RTC_WKUP);
}
