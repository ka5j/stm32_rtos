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
