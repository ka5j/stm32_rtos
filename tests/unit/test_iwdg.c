/**
 * @file test_iwdg.c
 * @brief Host-side tests for drivers/src/iwdg.c, on in-memory
 *        IwdgRegisters_t and DbgmcuRegisters_t. An in-memory KR keeps only
 *        the last key written, so these tests see the final state; the key
 *        order (start, unlock, write, refresh) is checked on the board, where
 *        PR and RLR read back only if the unlock worked.
 */
#include "iwdg.h"
#include "unity.h"

/** Writes PR and RLR, and ends with a refresh. */
void test_iwdg_driver_start_writes_pr_rlr_and_refreshes(void)
{
    IwdgRegisters_t iwdg = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, iwdgStart(&iwdg, IWDG_PR_DIV32, 100U));

    TEST_ASSERT_EQUAL_HEX32(IWDG_PR_DIV32, iwdg.PR);
    TEST_ASSERT_EQUAL_HEX32(100U, iwdg.RLR);
    TEST_ASSERT_EQUAL_HEX32(IWDG_KEY_RELOAD, iwdg.KR);
}

/** The largest prescaler and reload are accepted. */
void test_iwdg_driver_start_accepts_maximums(void)
{
    IwdgRegisters_t iwdg = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, iwdgStart(&iwdg, IWDG_PR_DIV256, 0xFFFU));

    TEST_ASSERT_EQUAL_HEX32(IWDG_PR_DIV256, iwdg.PR);
    TEST_ASSERT_EQUAL_HEX32(0xFFFU, iwdg.RLR);
}

/** PR 7 (a second encoding of /256) and RLR above 12 bits are rejected; nothing is written. */
void test_iwdg_driver_start_rejects_out_of_range(void)
{
    IwdgRegisters_t iwdg = {0};
    IwdgRegisters_t untouched = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, iwdgStart(&iwdg, 7U, 100U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, iwdgStart(&iwdg, IWDG_PR_DIV4, 0x1000U));
    TEST_ASSERT_EQUAL_MEMORY(&untouched, &iwdg, sizeof iwdg);
}

/** A prescaler update that never ends times out before PR or RLR is written. */
void test_iwdg_driver_start_times_out_on_pvu(void)
{
    IwdgRegisters_t iwdg = {.SR = IWDG_SR_PVU};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, iwdgStart(&iwdg, IWDG_PR_DIV32, 100U));

    TEST_ASSERT_EQUAL_HEX32(0U, iwdg.PR);
    TEST_ASSERT_EQUAL_HEX32(0U, iwdg.RLR);
    /* Started and unlocked, not refreshed. */
    TEST_ASSERT_EQUAL_HEX32(IWDG_KEY_ENABLE_ACCESS, iwdg.KR);
}

/** A reload update that never ends times out too. */
void test_iwdg_driver_start_times_out_on_rvu(void)
{
    IwdgRegisters_t iwdg = {.SR = IWDG_SR_RVU};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, iwdgStart(&iwdg, IWDG_PR_DIV32, 100U));
    TEST_ASSERT_EQUAL_HEX32(IWDG_KEY_ENABLE_ACCESS, iwdg.KR);
}

/** Refresh writes the reload key and nothing else. */
void test_iwdg_driver_refresh_writes_reload_key(void)
{
    IwdgRegisters_t iwdg = {.PR = IWDG_PR_DIV64, .RLR = 0x123U};

    iwdgRefresh(&iwdg);

    TEST_ASSERT_EQUAL_HEX32(IWDG_KEY_RELOAD, iwdg.KR);
    TEST_ASSERT_EQUAL_HEX32(IWDG_PR_DIV64, iwdg.PR);
    TEST_ASSERT_EQUAL_HEX32(0x123U, iwdg.RLR);
}

/** Freeze sets DBG_IWDG_STOP and keeps the other freeze bits. */
void test_iwdg_driver_freeze_in_debug_sets_only_its_bit(void)
{
    DbgmcuRegisters_t dbgmcu = {.APB1_FZ = 0x00000801U, .APB2_FZ = 0x5U};

    iwdgFreezeInDebug(&dbgmcu);

    TEST_ASSERT_EQUAL_HEX32(0x00000801U | DBGMCU_APB1_FZ_DBG_IWDG_STOP, dbgmcu.APB1_FZ);
    TEST_ASSERT_EQUAL_HEX32(0x5U, dbgmcu.APB2_FZ);
}
