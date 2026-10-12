/**
 * @file test_mpu_reg.c
 * @brief Host-side sanity checks for core/inc/mpu_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "mpu_reg.h"
#include "unity.h"

#include <stddef.h>

/** MpuRegisters_t must be exactly 0x2C bytes (TYPE..RASR_A3, PM0214). */
void test_mpu_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x2C, sizeof(MpuRegisters_t));
}

/** MPU_RASR.AP values must be distinct and fit the 3-bit field. */
void test_mpu_rasr_ap_values_are_distinct_and_in_range(void)
{
    uint32_t values[] = {MPU_RASR_AP_NONE,    MPU_RASR_AP_PRIV_RW, MPU_RASR_AP_PRIV_RW_UNPRIV_RO,
                         MPU_RASR_AP_FULL_RW, MPU_RASR_AP_PRIV_RO, MPU_RASR_AP_FULL_RO};

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(0x7U, values[i]);
        for (size_t j = i + 1; j < sizeof(values) / sizeof(values[0]); j++)
        {
            TEST_ASSERT_NOT_EQUAL_UINT32(values[i], values[j]);
        }
    }
}

/** Literal base, offsets (the alias pairs are order-sensitive) and AP encodings (ARM's
 * CMSIS/PM0214). */
void test_mpu_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xE000ED90UL, MPU_BASE);
    TEST_ASSERT_EQUAL_HEX32(MPU_BASE, (uintptr_t)MPU);
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(MpuRegisters_t, CTRL));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(MpuRegisters_t, RNR));
    TEST_ASSERT_EQUAL_HEX32(0x0C, offsetof(MpuRegisters_t, RBAR));
    TEST_ASSERT_EQUAL_HEX32(0x10, offsetof(MpuRegisters_t, RASR));
    TEST_ASSERT_EQUAL_HEX32(0x14, offsetof(MpuRegisters_t, RBAR_A1));
    TEST_ASSERT_EQUAL_HEX32(0x18, offsetof(MpuRegisters_t, RASR_A1));
    TEST_ASSERT_EQUAL_HEX32(0x1C, offsetof(MpuRegisters_t, RBAR_A2));
    TEST_ASSERT_EQUAL_HEX32(0x20, offsetof(MpuRegisters_t, RASR_A2));
    TEST_ASSERT_EQUAL_HEX32(0x24, offsetof(MpuRegisters_t, RBAR_A3));
    TEST_ASSERT_EQUAL_HEX32(0x28, offsetof(MpuRegisters_t, RASR_A3));
    TEST_ASSERT_EQUAL_HEX32(0x0U, MPU_RASR_AP_NONE);
    TEST_ASSERT_EQUAL_HEX32(0x1U, MPU_RASR_AP_PRIV_RW);
    TEST_ASSERT_EQUAL_HEX32(0x2U, MPU_RASR_AP_PRIV_RW_UNPRIV_RO);
    TEST_ASSERT_EQUAL_HEX32(0x3U, MPU_RASR_AP_FULL_RW);
    TEST_ASSERT_EQUAL_HEX32(0x5U, MPU_RASR_AP_PRIV_RO);
    TEST_ASSERT_EQUAL_HEX32(0x6U, MPU_RASR_AP_FULL_RO);
}
