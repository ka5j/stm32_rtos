/**
 * @file test_iwdg_reg.c
 * @brief Host-side sanity checks for device/inc/iwdg_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "iwdg_reg.h"
#include "unity.h"

#include <stddef.h>

/** IwdgRegisters_t must be exactly 0x10 bytes (KR, PR, RLR, SR - the F446 IWDG has no
 *  window register; ST's SVD and stm32f446xx.h agree). */
void test_iwdg_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x10, sizeof(IwdgRegisters_t));
    TEST_ASSERT_EQUAL_HEX32(0x0C, offsetof(IwdgRegisters_t, SR));
}

/** SR carries exactly the two update-in-progress flags the F446 has. */
void test_iwdg_sr_flags(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x1U, IWDG_SR_PVU);
    TEST_ASSERT_EQUAL_HEX32(0x2U, IWDG_SR_RVU);
}

/** IWDG_KEY_* values must be distinct 16-bit key codes. */
void test_iwdg_key_values_are_distinct(void)
{
    TEST_ASSERT_NOT_EQUAL_UINT32(IWDG_KEY_RELOAD, IWDG_KEY_ENABLE_ACCESS);
    TEST_ASSERT_NOT_EQUAL_UINT32(IWDG_KEY_ENABLE_ACCESS, IWDG_KEY_START);
    TEST_ASSERT_NOT_EQUAL_UINT32(IWDG_KEY_RELOAD, IWDG_KEY_START);
}

/** Literal base, offsets, key codes and masks (ST's SVD and HAL). */
void test_iwdg_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40003000UL, IWDG_BASE);
    TEST_ASSERT_EQUAL_HEX32(IWDG_BASE, (uintptr_t)IWDG);
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(IwdgRegisters_t, PR));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(IwdgRegisters_t, RLR));
    TEST_ASSERT_EQUAL_HEX32(0xAAAAU, IWDG_KEY_RELOAD);
    TEST_ASSERT_EQUAL_HEX32(0x5555U, IWDG_KEY_ENABLE_ACCESS);
    TEST_ASSERT_EQUAL_HEX32(0xCCCCU, IWDG_KEY_START);
    TEST_ASSERT_EQUAL_HEX32(0x00000007U, IWDG_PR_PR_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00000FFFU, IWDG_RLR_RL_Msk);
}
