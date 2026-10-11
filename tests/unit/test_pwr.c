/**
 * @file test_pwr.c
 * @brief Host-side tests for drivers/src/pwr.c. Each test operates on a
 *        plain in-memory PwrRegisters_t standing in for a real peripheral
 *        - pwr.c's functions take the register block as a parameter
 *        rather than reaching for the hardware PWR macro, which is what
 *        makes this testable off-target, including the CSR.VOSRDY poll:
 *        CR (what pwrSetVoltageScale() writes) and CSR (what
 *        pwrWaitVoltageScaleReady() polls) are separate fields, so a test
 *        can hold CSR.VOSRDY low indefinitely to exercise the timeout
 *        path with no real hardware or timer involved. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "pwr.h"
#include "unity.h"

#include <stddef.h>

/* --- pwrSetVoltageScale --- */

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

/** Each documented scale (SCALE1/2/3) is written to CR.VOS. */
void test_pwr_driver_set_voltage_scale_writes_each_documented_scale(void)
{
    static const uint32_t scales[] = {PWR_CR_VOS_SCALE1, PWR_CR_VOS_SCALE2, PWR_CR_VOS_SCALE3};

    for (size_t i = 0; i < sizeof(scales) / sizeof(scales[0]); i++)
    {
        PwrRegisters_t pwr = {0};

        DriverStatus_e status = pwrSetVoltageScale(&pwr, scales[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(scales[i], (pwr.CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos);
    }
}

/** A valid call touches CR.VOS only, leaving every other CR bit
 *  (LPDS/PDDS/CWUF/CSBF/PVDE/PLS/DBP/ODEN/ODSWEN) untouched. */
void test_pwr_driver_set_voltage_scale_touches_only_vos_field(void)
{
    PwrRegisters_t pwr = {.CR = 0xFFFFFFFFU};

    DriverStatus_e status = pwrSetVoltageScale(&pwr, PWR_CR_VOS_SCALE2);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_VOS_SCALE2, (pwr.CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~PWR_CR_VOS_Msk, pwr.CR & ~PWR_CR_VOS_Msk);
}

/** Selecting a scale does not poll, and in particular does not depend on
 *  CSR.VOSRDY - which cannot set until the PLL is enabled (RM0390 5.1.4),
 *  long after this call. A cleared CSR must still report OK, since an
 *  earlier revision of this driver polled here and would have timed out
 *  on every boot once the call was correctly ordered before the PLL. */
void test_pwr_driver_set_voltage_scale_does_not_poll_vosrdy(void)
{
    PwrRegisters_t pwr = {0};

    DriverStatus_e status = pwrSetVoltageScale(&pwr, PWR_CR_VOS_SCALE1);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0U, pwr.CSR);
}

/* --- pwrWaitVoltageScaleReady --- */

/** CSR.VOSRDY already set (hardware has caught up) reports OK. */
void test_pwr_driver_wait_voltage_scale_ready_reports_ok_when_vosrdy_set(void)
{
    PwrRegisters_t pwr = {.CSR = PWR_CSR_VOSRDY};

    DriverStatus_e status = pwrWaitVoltageScaleReady(&pwr);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
}

/** CSR.VOSRDY never set (hardware never catches up) exhausts the bounded
 *  retry and reports a timeout. */
void test_pwr_driver_wait_voltage_scale_ready_times_out_when_vosrdy_never_sets(void)
{
    PwrRegisters_t pwr = {0};

    DriverStatus_e status = pwrWaitVoltageScaleReady(&pwr);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
}

/** The wait only reads CSR - it must not disturb CR, which already holds
 *  the scale pwrSetVoltageScale() selected before the PLL came on. */
void test_pwr_driver_wait_voltage_scale_ready_does_not_write_cr(void)
{
    PwrRegisters_t pwr = {.CR = PWR_CR_VOS_SCALE1 << PWR_CR_VOS_Pos, .CSR = PWR_CSR_VOSRDY};

    DriverStatus_e status = pwrWaitVoltageScaleReady(&pwr);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_VOS_SCALE1 << PWR_CR_VOS_Pos, pwr.CR);
}

/* --- pwrEnableOverdrive --- */

/** With both ready flags set, overdrive enables ODEN then ODSWEN and reports OK. */
void test_pwr_driver_enable_overdrive_sets_both_enable_bits(void)
{
    PwrRegisters_t pwr = {.CSR = PWR_CSR_ODRDY | PWR_CSR_ODSWRDY};

    DriverStatus_e status = pwrEnableOverdrive(&pwr);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_ODEN | PWR_CR_ODSWEN, pwr.CR);
}

/** Overdrive is a read-modify-write: it must preserve CR.VOS and every other CR field. */
void test_pwr_driver_enable_overdrive_preserves_other_cr_fields(void)
{
    PwrRegisters_t pwr = {.CR = PWR_CR_VOS_Msk | PWR_CR_DBP,
                          .CSR = PWR_CSR_ODRDY | PWR_CSR_ODSWRDY};

    DriverStatus_e status = pwrEnableOverdrive(&pwr);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_VOS_Msk | PWR_CR_DBP | PWR_CR_ODEN | PWR_CR_ODSWEN, pwr.CR);
}

/** If ODRDY never sets, it times out and must not have written ODSWEN. */
void test_pwr_driver_enable_overdrive_times_out_without_odrdy_and_skips_switch(void)
{
    PwrRegisters_t pwr = {0};

    DriverStatus_e status = pwrEnableOverdrive(&pwr);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_ODEN, pwr.CR);
}

/** If ODSWRDY never sets after ODRDY did, it times out with both enable bits written. */
void test_pwr_driver_enable_overdrive_times_out_without_odswrdy(void)
{
    PwrRegisters_t pwr = {.CSR = PWR_CSR_ODRDY};

    DriverStatus_e status = pwrEnableOverdrive(&pwr);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(PWR_CR_ODEN | PWR_CR_ODSWEN, pwr.CR);
}
