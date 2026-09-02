/**
 * @file test_scb_reg.c
 * @brief Host-side sanity checks for core/inc/scb_reg.h (including its
 *        FpuRegisters_t). Pure data validation - no hardware, no driver
 *        logic. Compiled into tests/unit/test_runner.c's run_tests binary.
 */
#include "scb_reg.h"
#include "unity.h"

#include <stddef.h>

/**
 * ScbRegisters_t must be exactly 0x8C bytes (CPUID..CPACR, PM0214) with
 * CPACR landing at its fixed offset past the CPU-ID register block.
 */
void test_scb_register_block_size_and_offset(void)
{
    TEST_ASSERT_EQUAL_UINT(0x8C, sizeof(ScbRegisters_t));
    TEST_ASSERT_EQUAL_HEX32(0x88, offsetof(ScbRegisters_t, CPACR));
}

/** FpuRegisters_t must be exactly 3 words (FPCCR, FPCAR, FPDSCR). */
void test_fpu_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0xC, sizeof(FpuRegisters_t));
}

/** SCB_CFSR's MMFSR/BFSR/UFSR byte fields must not overlap. */
void test_scb_cfsr_fault_status_byte_fields_do_not_overlap(void)
{
    uint32_t mmfsr_bits = SCB_CFSR_IACCVIOL | SCB_CFSR_DACCVIOL | SCB_CFSR_MUNSTKERR
                          | SCB_CFSR_MSTKERR | SCB_CFSR_MLSPERR | SCB_CFSR_MMARVALID;
    uint32_t bfsr_bits = SCB_CFSR_IBUSERR | SCB_CFSR_PRECISERR | SCB_CFSR_IMPRECISERR
                         | SCB_CFSR_UNSTKERR | SCB_CFSR_STKERR | SCB_CFSR_LSPERR
                         | SCB_CFSR_BFARVALID;

    TEST_ASSERT_EQUAL_HEX32(0x00U, mmfsr_bits & 0xFFFFFF00U);
    TEST_ASSERT_EQUAL_HEX32(0x00U, bfsr_bits & 0xFFFF00FFU);
    TEST_ASSERT_EQUAL_HEX32(0x00U, mmfsr_bits & bfsr_bits);
}
