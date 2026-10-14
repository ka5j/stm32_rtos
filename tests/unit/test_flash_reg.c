/**
 * @file test_flash_reg.c
 * @brief Host-side sanity checks for device/inc/flash_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "flash_reg.h"
#include "unity.h"

#include <stddef.h>

/** FlashRegisters_t must be exactly 0x1C bytes (ACR..OPTCR1, RM0390 3.8). */
void test_flash_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0x1C, sizeof(FlashRegisters_t));
}

/** Literal base, offsets, ACR masks and bit positions, written out (from ST's SVD and HAL). */
void test_flash_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40023C00UL, FLASH_BASE);
    TEST_ASSERT_EQUAL_HEX32(FLASH_BASE, (uintptr_t)FLASH);
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(FlashRegisters_t, KEYR));
    TEST_ASSERT_EQUAL_HEX32(0x0C, offsetof(FlashRegisters_t, SR));
    TEST_ASSERT_EQUAL_HEX32(0x10, offsetof(FlashRegisters_t, CR));
    TEST_ASSERT_EQUAL_HEX32(0x14, offsetof(FlashRegisters_t, OPTCR));
    TEST_ASSERT_EQUAL_HEX32(0x18, offsetof(FlashRegisters_t, OPTCR1));

    TEST_ASSERT_EQUAL_HEX32(0x0000000FU, FLASH_ACR_LATENCY_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00000100U, FLASH_ACR_PRFTEN);
    TEST_ASSERT_EQUAL_HEX32(0x00000200U, FLASH_ACR_ICEN);
    TEST_ASSERT_EQUAL_HEX32(0x00000400U, FLASH_ACR_DCEN);
    TEST_ASSERT_EQUAL_HEX32(0x00000800U, FLASH_ACR_ICRST);
    TEST_ASSERT_EQUAL_HEX32(0x00001000U, FLASH_ACR_DCRST);
}
