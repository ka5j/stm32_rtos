/**
 * @file test_systick.c
 * @brief Host-side tests for drivers/src/systick.c. Each test operates on
 *        a plain in-memory SysTickRegisters_t standing in for the real
 *        peripheral - systick.c's functions take the register block as a
 *        parameter rather than reaching for the hardware SYSTICK macro,
 *        which is what makes this testable off-target. The in-memory
 *        struct cannot count down or raise COUNTFLAG, so these tests check
 *        what the driver *wrote* (reload arithmetic and the final CTRL
 *        value), not the timer's behaviour. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "systick.h"
#include "unity.h"

#define SYSTICK_RUNNING (SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE)

/** 180 MHz / 1 kHz -> LOAD 179999, running from the processor clock with the interrupt on. */
void test_systick_driver_init_180mhz_1khz(void)
{
    SysTickRegisters_t systick = {.VAL = 0x1234U};

    DriverStatus_e status = systickInit(&systick, 180000000U, 1000U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(179999U, systick.LOAD);
    TEST_ASSERT_EQUAL_HEX32(0U, systick.VAL);
    TEST_ASSERT_EQUAL_HEX32(SYSTICK_RUNNING, systick.CTRL);
}

/** 16 MHz (HSI) / 1 kHz -> LOAD 15999. */
void test_systick_driver_init_16mhz_1khz(void)
{
    SysTickRegisters_t systick = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, systickInit(&systick, 16000000U, 1000U));
    TEST_ASSERT_EQUAL_UINT32(15999U, systick.LOAD);
}

/** The shortest accepted period is 2 clocks (LOAD 1); LOAD 0 never reloads. */
void test_systick_driver_init_accepts_minimum_period(void)
{
    SysTickRegisters_t systick = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, systickInit(&systick, 2U, 1U));
    TEST_ASSERT_EQUAL_UINT32(1U, systick.LOAD);
}

/** The longest accepted period is 2^24 clocks (LOAD 0x00FFFFFF). */
void test_systick_driver_init_accepts_maximum_period(void)
{
    SysTickRegisters_t systick = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, systickInit(&systick, 0x1000000U, 1U));
    TEST_ASSERT_EQUAL_HEX32(SYSTICK_LOAD_RELOAD_Msk, systick.LOAD);
}

/** A non-exact division truncates, so the period is never longer than requested. */
void test_systick_driver_init_truncates_inexact_division(void)
{
    SysTickRegisters_t systick = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, systickInit(&systick, 10U, 3U));
    TEST_ASSERT_EQUAL_UINT32(2U, systick.LOAD);
}

/** Zero core or tick frequency is rejected with nothing written. */
void test_systick_driver_init_rejects_zero_frequency(void)
{
    SysTickRegisters_t systick = {.CTRL = 0x5U, .LOAD = 0x6U, .VAL = 0x7U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, systickInit(&systick, 0U, 1000U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, systickInit(&systick, 180000000U, 0U));
    TEST_ASSERT_EQUAL_HEX32(0x5U, systick.CTRL);
    TEST_ASSERT_EQUAL_HEX32(0x6U, systick.LOAD);
    TEST_ASSERT_EQUAL_HEX32(0x7U, systick.VAL);
}

/** A period under 2 clocks (including tick_hz > core_hz) is rejected with nothing written. */
void test_systick_driver_init_rejects_period_below_two_clocks(void)
{
    SysTickRegisters_t systick = {.CTRL = 0x5U, .LOAD = 0x6U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, systickInit(&systick, 1000U, 1000U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, systickInit(&systick, 1000U, 2000U));
    TEST_ASSERT_EQUAL_HEX32(0x5U, systick.CTRL);
    TEST_ASSERT_EQUAL_HEX32(0x6U, systick.LOAD);
}

/** A period over 2^24 clocks does not fit the 24-bit LOAD and is rejected with nothing written. */
void test_systick_driver_init_rejects_period_above_24_bits(void)
{
    SysTickRegisters_t systick = {.CTRL = 0x5U, .LOAD = 0x6U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, systickInit(&systick, 0x1000001U, 1U));
    TEST_ASSERT_EQUAL_HEX32(0x5U, systick.CTRL);
    TEST_ASSERT_EQUAL_HEX32(0x6U, systick.LOAD);
}

/** Re-initialising an already-running timer ends running with the new reload. */
void test_systick_driver_init_reconfigures_running_timer(void)
{
    SysTickRegisters_t systick = {.CTRL = SYSTICK_RUNNING, .LOAD = 15999U, .VAL = 100U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, systickInit(&systick, 180000000U, 1000U));
    TEST_ASSERT_EQUAL_UINT32(179999U, systick.LOAD);
    TEST_ASSERT_EQUAL_HEX32(0U, systick.VAL);
    TEST_ASSERT_EQUAL_HEX32(SYSTICK_RUNNING, systick.CTRL);
}

/** systickDeinit() returns CTRL, LOAD and VAL to zero. */
void test_systick_driver_deinit_restores_reset_state(void)
{
    SysTickRegisters_t systick = {.CTRL = SYSTICK_RUNNING, .LOAD = 179999U, .VAL = 42U};

    systickDeinit(&systick);

    TEST_ASSERT_EQUAL_HEX32(0U, systick.CTRL);
    TEST_ASSERT_EQUAL_HEX32(0U, systick.LOAD);
    TEST_ASSERT_EQUAL_HEX32(0U, systick.VAL);
}
