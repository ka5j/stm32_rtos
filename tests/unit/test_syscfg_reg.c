/**
 * @file test_syscfg_reg.c
 * @brief Host-side sanity checks for device/inc/syscfg_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "syscfg_reg.h"
#include "unity.h"

#include <stddef.h>

/** SyscfgRegisters_t must be exactly 0x24 bytes (MEMRMP..CMPCR, RM0390 8.2). */
void test_syscfg_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x24, sizeof(SyscfgRegisters_t));
}

/** SYSCFG_EXTICRx port-selector values must be distinct and fit the 4-bit field. */
void test_syscfg_exticr_port_values_are_distinct_and_in_range(void)
{
    uint32_t values[] = {SYSCFG_EXTICR_PA, SYSCFG_EXTICR_PB, SYSCFG_EXTICR_PC, SYSCFG_EXTICR_PD,
                         SYSCFG_EXTICR_PE, SYSCFG_EXTICR_PF, SYSCFG_EXTICR_PG, SYSCFG_EXTICR_PH};

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(0xFU, values[i]);
        for (size_t j = i + 1; j < sizeof(values) / sizeof(values[0]); j++)
        {
            TEST_ASSERT_NOT_EQUAL_UINT32(values[i], values[j]);
        }
    }
}

/** Literal base, offsets (every member after the RESERVED gap) and EXTICR port codes (ST's SVD). */
void test_syscfg_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40013800UL, SYSCFG_BASE);
    TEST_ASSERT_EQUAL_HEX32(SYSCFG_BASE, (uintptr_t)SYSCFG);
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(SyscfgRegisters_t, PMC));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(SyscfgRegisters_t, EXTICR));
    TEST_ASSERT_EQUAL_HEX32(0x20, offsetof(SyscfgRegisters_t, CMPCR));
    TEST_ASSERT_EQUAL_HEX32(0x0U, SYSCFG_EXTICR_PA);
    TEST_ASSERT_EQUAL_HEX32(0x1U, SYSCFG_EXTICR_PB);
    TEST_ASSERT_EQUAL_HEX32(0x2U, SYSCFG_EXTICR_PC);
    TEST_ASSERT_EQUAL_HEX32(0x3U, SYSCFG_EXTICR_PD);
    TEST_ASSERT_EQUAL_HEX32(0x4U, SYSCFG_EXTICR_PE);
    TEST_ASSERT_EQUAL_HEX32(0x5U, SYSCFG_EXTICR_PF);
    TEST_ASSERT_EQUAL_HEX32(0x6U, SYSCFG_EXTICR_PG);
    TEST_ASSERT_EQUAL_HEX32(0x7U, SYSCFG_EXTICR_PH);
}
