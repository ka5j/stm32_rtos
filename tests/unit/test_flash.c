/**
 * @file test_flash.c
 * @brief Host-side tests for drivers/src/flash.c. Each test operates on a
 *        plain in-memory FlashRegisters_t standing in for a real
 *        peripheral - flash.c's functions take the register block as a
 *        parameter rather than reaching for the hardware FLASH macro,
 *        which is what makes this testable off-target. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "flash.h"
#include "unity.h"

/** latency outside ACR.LATENCY's 4-bit field must be rejected before any
 *  write. */
void test_flash_driver_set_latency_rejects_invalid_latency(void)
{
    FlashRegisters_t flash = {0};

    DriverStatus_e status = flashSetLatency(&flash, 16U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, flash.ACR);
}

/** The full 4-bit field width (0-15) is accepted - RM0390 defines every
 *  encoding as a valid wait-state count, unlike PUPDR/PLLP's reserved
 *  codes elsewhere in this project. */
void test_flash_driver_set_latency_accepts_full_field_width(void)
{
    FlashRegisters_t flash = {0};

    DriverStatus_e status = flashSetLatency(&flash, 15U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(15U, flash.ACR & FLASH_ACR_LATENCY_Msk);
}

/** A valid call sets ACR.LATENCY only, leaving every other ACR bit
 *  (PRFTEN/ICEN/DCEN/ICRST/DCRST) untouched. */
void test_flash_driver_set_latency_touches_only_latency_field(void)
{
    FlashRegisters_t flash = {.ACR = 0xFFFFFFFFU};

    DriverStatus_e status = flashSetLatency(&flash, 5U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(5U, flash.ACR & FLASH_ACR_LATENCY_Msk);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~FLASH_ACR_LATENCY_Msk,
                            flash.ACR & ~FLASH_ACR_LATENCY_Msk);
}
