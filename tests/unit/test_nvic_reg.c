/**
 * @file test_nvic_reg.c
 * @brief Host-side sanity checks for core/inc/nvic_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 *
 * IRQn_e's values are cross-checked against startup_stm32f446re.s's
 * vector table by tools/check_vector_table.awk, run as part of `make
 * test` (see the Makefile's test target) rather than as a Unity test
 * here - it validates two hand-written files against each other, not a
 * property of this header in isolation.
 */
#include "nvic_reg.h"
#include "unity.h"

#include <stddef.h>

/**
 * NvicRegisters_t must be exactly 0xE04 bytes (ISER..STIR, PM0214) and
 * each register bank must land at its fixed Armv7-M architectural offset
 * regardless of how many words this chip actually implements.
 */
void test_nvic_register_block_size_and_offsets(void)
{
    TEST_ASSERT_EQUAL_UINT(0xE04, sizeof(NvicRegisters_t));
    TEST_ASSERT_EQUAL_HEX32(0x080, offsetof(NvicRegisters_t, ICER));
    TEST_ASSERT_EQUAL_HEX32(0x100, offsetof(NvicRegisters_t, ISPR));
    TEST_ASSERT_EQUAL_HEX32(0x180, offsetof(NvicRegisters_t, ICPR));
    TEST_ASSERT_EQUAL_HEX32(0x200, offsetof(NvicRegisters_t, IABR));
    TEST_ASSERT_EQUAL_HEX32(0x300, offsetof(NvicRegisters_t, IP));
    TEST_ASSERT_EQUAL_HEX32(0xE00, offsetof(NvicRegisters_t, STIR));
}

/**
 * The system exceptions use the Armv7-M-fixed negative numbers (exception
 * number minus 16) and must stay below every device interrupt, so a
 * driver's `IRQn >= 0` check cleanly separates NVIC lines from SCB-handled ones.
 */
void test_nvic_system_exception_numbers(void)
{
    TEST_ASSERT_EQUAL_INT(-12, MemoryManagement_IRQn);
    TEST_ASSERT_EQUAL_INT(-11, BusFault_IRQn);
    TEST_ASSERT_EQUAL_INT(-10, UsageFault_IRQn);
    TEST_ASSERT_EQUAL_INT(-5, SVCall_IRQn);
    TEST_ASSERT_EQUAL_INT(-2, PendSV_IRQn);
    TEST_ASSERT_EQUAL_INT(-1, SysTick_IRQn);
    TEST_ASSERT_LESS_THAN_INT(WWDG_IRQn, SysTick_IRQn);
}

/** Literal base, pointer macro and priority shift (ARM's CMSIS: __NVIC_PRIO_BITS 4 on the F446). */
void test_nvic_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xE000E100UL, NVIC_BASE);
    TEST_ASSERT_EQUAL_HEX32(NVIC_BASE, (uintptr_t)NVIC);
    TEST_ASSERT_EQUAL_UINT(4U, NVIC_PRIO_BITS);
    TEST_ASSERT_EQUAL_UINT(4U, NVIC_PRIO_Pos);
}
