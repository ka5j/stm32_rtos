/**
 * @file test_flash.c
 * @brief Host-side tests for drivers/src/flash.c. Each test operates on a
 *        plain in-memory FlashRegisters_t standing in for a real
 *        peripheral - flash.c's functions take the register block as a
 *        parameter rather than reaching for the hardware FLASH macro,
 *        which is what makes this testable off-target. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 *
 * One branch in this driver is deliberately not reachable from here:
 * flashSetLatency()'s read-back check compares ACR against the value it
 * just wrote to ACR, and an in-memory struct always agrees. See that
 * function's own comment for why it is excluded from the coverage gate
 * rather than removed, and docs/DESIGN.md "Host testing and its limits"
 * for what host tests can and cannot establish.
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

/** Every encoding 0-15 round-trips through the read-back check, which
 *  must pass for a write that lands - the check exists to catch silicon
 *  refusing the write, not to reject values this driver itself accepted.
 *  Includes 0, where the write is a no-op against a zeroed field. */
void test_flash_driver_set_latency_read_back_accepts_every_encoding(void)
{
    for (uint32_t latency = 0U; latency <= 15U; latency++)
    {
        FlashRegisters_t flash = {0};

        DriverStatus_e status = flashSetLatency(&flash, latency);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(latency, flash.ACR & FLASH_ACR_LATENCY_Msk);
    }
}

/* --- flashEnableCaches --- */

/** flashEnableCaches() sets exactly PRFTEN, ICEN and DCEN, and not the cache-reset bits. */
void test_flash_driver_enable_caches_sets_prefetch_and_both_caches(void)
{
    FlashRegisters_t flash = {0};

    flashEnableCaches(&flash);

    TEST_ASSERT_EQUAL_HEX32(FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN, flash.ACR);
    TEST_ASSERT_EQUAL_HEX32(0U, flash.ACR & (FLASH_ACR_ICRST | FLASH_ACR_DCRST));
}

/** flashEnableCaches() is a read-modify-write: it keeps whatever LATENCY was programmed. */
void test_flash_driver_enable_caches_preserves_latency(void)
{
    FlashRegisters_t flash = {.ACR = 5U};

    flashEnableCaches(&flash);

    TEST_ASSERT_EQUAL_HEX32(5U, flash.ACR & FLASH_ACR_LATENCY_Msk);
    TEST_ASSERT_EQUAL_HEX32(FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | 5U, flash.ACR);
}

/** flashSetLatency() after flashEnableCaches() does not clear the cache bits (order-independent).
 */
void test_flash_driver_set_latency_preserves_cache_enable_bits(void)
{
    FlashRegisters_t flash = {0};

    flashEnableCaches(&flash);
    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, flashSetLatency(&flash, 5U));

    TEST_ASSERT_EQUAL_HEX32(FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | 5U, flash.ACR);
}

/* --- review follow-ups --- */

/** flashEnableCaches keeps every other ACR bit (including the cache-reset bits) as it was. */
void test_flash_driver_enable_caches_preserves_every_other_acr_bit(void)
{
    uint32_t others = 0xFFFFFFFFU & ~(FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    FlashRegisters_t flash = {.ACR = others};

    flashEnableCaches(&flash);

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, flash.ACR);
    TEST_ASSERT_EQUAL_HEX32(others, flash.ACR & others);
}
