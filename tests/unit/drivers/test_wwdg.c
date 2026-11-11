/**
 * @file test_wwdg.c
 * @brief Host-side tests for drivers/src/wwdg.c, on in-memory
 *        WwdgRegisters_t and DbgmcuRegisters_t. That the counter, window and
 *        early-wakeup interrupt behave as written is checked on the board
 *        (tests/target/smoke.c).
 */
#include "unity.h"
#include "wwdg.h"

/** CFR gets W and WDGTB; CR gets WDGA and T. */
void test_wwdg_driver_start_writes_cfr_then_cr(void)
{
    WwdgRegisters_t wwdg = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, wwdgStart(&wwdg, WWDG_CFR_WDGTB_DIV8, 0x5FU, 0x7FU));

    TEST_ASSERT_EQUAL_HEX32(0x5FU | (WWDG_CFR_WDGTB_DIV8 << WWDG_CFR_WDGTB_Pos), wwdg.CFR);
    TEST_ASSERT_EQUAL_HEX32(WWDG_CR_WDGA | 0x7FU, wwdg.CR);
}

/** A stale EWIF, set by the counter free-running before the start, is cleared. Found on the
 *  board: left set, the early wakeup fired at once and its refresh came above the window. */
void test_wwdg_driver_start_clears_stale_ewif(void)
{
    WwdgRegisters_t wwdg = {.SR = WWDG_SR_EWIF};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, wwdgStart(&wwdg, WWDG_CFR_WDGTB_DIV8, 0x5FU, 0x7FU));

    TEST_ASSERT_EQUAL_HEX32(0U, wwdg.SR);
}

/** A rejected start leaves a set EWIF alone. */
void test_wwdg_driver_rejected_start_leaves_sr(void)
{
    WwdgRegisters_t wwdg = {.SR = WWDG_SR_EWIF};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM,
                      wwdgStart(&wwdg, WWDG_CFR_WDGTB_DIV8, 0x5FU, 0x80U));

    TEST_ASSERT_EQUAL_HEX32(WWDG_SR_EWIF, wwdg.SR);
}

/** An already-set EWI survives a restart (it is set-only on the chip). */
void test_wwdg_driver_start_keeps_ewi(void)
{
    WwdgRegisters_t wwdg = {.CFR = WWDG_CFR_EWI | 0x7FU};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, wwdgStart(&wwdg, WWDG_CFR_WDGTB_DIV1, 0x40U, 0x40U));

    TEST_ASSERT_EQUAL_HEX32(WWDG_CFR_EWI | 0x40U, wwdg.CFR);
    TEST_ASSERT_EQUAL_HEX32(WWDG_CR_WDGA | 0x40U, wwdg.CR);
}

/** Window or counter outside 0x40-0x7F, or a timer base past /8, is rejected; nothing written. */
void test_wwdg_driver_start_rejects_out_of_range(void)
{
    WwdgRegisters_t wwdg = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM,
                      wwdgStart(&wwdg, WWDG_CFR_WDGTB_DIV8, 0x3FU, 0x7FU));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM,
                      wwdgStart(&wwdg, WWDG_CFR_WDGTB_DIV8, 0x80U, 0x7FU));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM,
                      wwdgStart(&wwdg, WWDG_CFR_WDGTB_DIV8, 0x5FU, 0x3FU));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM,
                      wwdgStart(&wwdg, WWDG_CFR_WDGTB_DIV8, 0x5FU, 0x80U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, wwdgStart(&wwdg, 4U, 0x5FU, 0x7FU));
    TEST_ASSERT_EQUAL_HEX32(0U, wwdg.CFR);
    TEST_ASSERT_EQUAL_HEX32(0U, wwdg.CR);
}

/** Refresh stores WDGA | counter, and leaves CFR alone. */
void test_wwdg_driver_refresh_stores_wdga_and_counter(void)
{
    WwdgRegisters_t wwdg = {.CR = WWDG_CR_WDGA | 0x41U, .CFR = 0x5FU};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, wwdgRefresh(&wwdg, 0x7FU));

    TEST_ASSERT_EQUAL_HEX32(WWDG_CR_WDGA | 0x7FU, wwdg.CR);
    TEST_ASSERT_EQUAL_HEX32(0x5FU, wwdg.CFR);
}

/** A refresh value outside 0x40-0x7F is rejected and CR is left alone. */
void test_wwdg_driver_refresh_rejects_out_of_range(void)
{
    WwdgRegisters_t wwdg = {.CR = WWDG_CR_WDGA | 0x50U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, wwdgRefresh(&wwdg, 0x3FU));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, wwdgRefresh(&wwdg, 0x80U));
    TEST_ASSERT_EQUAL_HEX32(WWDG_CR_WDGA | 0x50U, wwdg.CR);
}

/** Early wakeup sets CFR.EWI and keeps W and WDGTB. */
void test_wwdg_driver_enable_early_wakeup_sets_ewi(void)
{
    WwdgRegisters_t wwdg = {.CFR = 0x5FU | (WWDG_CFR_WDGTB_DIV8 << WWDG_CFR_WDGTB_Pos)};

    wwdgEnableEarlyWakeup(&wwdg);

    TEST_ASSERT_EQUAL_HEX32(WWDG_CFR_EWI | 0x5FU | (WWDG_CFR_WDGTB_DIV8 << WWDG_CFR_WDGTB_Pos),
                            wwdg.CFR);
}

/** Clearing writes 0 to SR (EWIF is rc_w0). */
void test_wwdg_driver_clear_early_wakeup_writes_zero(void)
{
    WwdgRegisters_t wwdg = {.SR = WWDG_SR_EWIF};

    wwdgClearEarlyWakeup(&wwdg);

    TEST_ASSERT_EQUAL_HEX32(0U, wwdg.SR);
}

/** Freeze sets DBG_WWDG_STOP and keeps the other bits. */
void test_wwdg_driver_freeze_in_debug_sets_only_its_bit(void)
{
    DbgmcuRegisters_t dbgmcu = {.APB1_FZ = DBGMCU_APB1_FZ_DBG_IWDG_STOP};

    wwdgFreezeInDebug(&dbgmcu);

    TEST_ASSERT_EQUAL_HEX32(DBGMCU_APB1_FZ_DBG_IWDG_STOP | DBGMCU_APB1_FZ_DBG_WWDG_STOP,
                            dbgmcu.APB1_FZ);
}
