/**
 * @file test_syscfg_reg.c
 * @brief Host-side sanity checks for device/inc/syscfg_reg.h. Pure data
 *        validation - no hardware, no driver logic. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "syscfg_reg.h"
#include "unity.h"

#include <stddef.h>

/** SyscfgRegisters_t must be exactly 0x24 bytes (MEMRMP..CMPCR, RM0390 9.2). */
void
test_syscfg_register_block_size(void)
{
  TEST_ASSERT_EQUAL_UINT(0x24, sizeof(SyscfgRegisters_t));
}

/** SYSCFG_EXTICRx port-selector values must be distinct and fit the 4-bit field. */
void
test_syscfg_exticr_port_values_are_distinct_and_in_range(void)
{
  uint32_t values[] = { SYSCFG_EXTICR_PA, SYSCFG_EXTICR_PB, SYSCFG_EXTICR_PC, SYSCFG_EXTICR_PD,
                        SYSCFG_EXTICR_PE, SYSCFG_EXTICR_PF, SYSCFG_EXTICR_PG, SYSCFG_EXTICR_PH };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(0xFU, values[i]);
    for (size_t j = i + 1; j < sizeof(values) / sizeof(values[0]); j++)
    {
      TEST_ASSERT_NOT_EQUAL_UINT32(values[i], values[j]);
    }
  }
}
