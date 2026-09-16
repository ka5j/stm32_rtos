/**
 * @file test_pwr.c
 * @brief Host-side tests for drivers/src/pwr.c. Each test operates on a
 *        plain in-memory PwrRegisters_t standing in for a real peripheral
 *        - pwr.c's functions take the register block as a parameter
 *        rather than reaching for the hardware PWR macro, which is what
 *        makes this testable off-target, including the CSR.VOSRDY poll:
 *        CR (what the driver writes) and CSR (what it polls) are
 *        separate fields, so a test can hold CSR.VOSRDY low indefinitely
 *        to exercise the timeout path with no real hardware or timer
 *        involved. Compiled into tests/unit/test_runner.c's run_tests
 *        binary.
 */
#include "pwr.h"
#include "unity.h"

#include <stddef.h>

/** vos == 0x0 (reserved per RM0390) must be rejected before any write. */
void test_pwr_driver_set_voltage_scale_rejects_reserved_zero(void)
{
    PwrRegisters_t pwr = {0};

    DriverStatus_e status = pwrSetVoltageScale(&pwr, 0x0U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, pwr.CR);
}

/** vos outside CR.VOS's 2-bit field must be rejected before any write. */
void test_pwr_driver_set_voltage_scale_rejects_out_of_range(void)
{
    PwrRegisters_t pwr = {0};

    DriverStatus_e status = pwrSetVoltageScale(&pwr, 0x4U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, pwr.CR);
}

/** Each documented scale (SCALE1/2/3) is written to CR.VOS and reports OK
 *  once CSR.VOSRDY (pre-set here to simulate the hardware already having
 *  caught up) is observed set. */
void test_pwr_driver_set_voltage_scale_writes_each_documented_scale(void)
{
    static const uint32_t scales[] = {PWR_CR_VOS_SCALE1, PWR_CR_VOS_SCALE2, PWR_CR_VOS_SCALE3};

    for (size_t i = 0; i < sizeof(scales) / sizeof(scales[0]); i++)
    {
        PwrRegisters_t pwr = {.CSR = PWR_CSR_VOSRDY};

        DriverStatus_e status = pwrSetVoltageScale(&pwr, scales[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(scales[i], (pwr.CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos);
    }
}

/** A valid call touches CR.VOS only, leaving every other CR bit
 *  (LPDS/PDDS/CWUF/CSBF/PVDE/PLS/DBP/ODEN/ODSWEN) untouched. */
void test_pwr_driver_set_voltage_scale_touches_only_vos_field(void)
{
    PwrRegisters_t pwr = {.CR = 0xFFFFFFFFU, .CSR = PWR_CSR_VOSRDY};

    DriverStatus_e status = pwrSetVoltageScale(&pwr, PWR_CR_VOS_SCALE2);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_VOS_SCALE2, (pwr.CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~PWR_CR_VOS_Msk, pwr.CR & ~PWR_CR_VOS_Msk);
}

/** CSR.VOSRDY never set (hardware never catches up) exhausts the bounded
 *  retry and reports a timeout - CR.VOS still reflects the requested
 *  write, since the write happens unconditionally before the poll. */
void test_pwr_driver_set_voltage_scale_times_out_when_vosrdy_never_sets(void)
{
    PwrRegisters_t pwr = {0};

    DriverStatus_e status = pwrSetVoltageScale(&pwr, PWR_CR_VOS_SCALE1);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_VOS_SCALE1, (pwr.CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos);
}
