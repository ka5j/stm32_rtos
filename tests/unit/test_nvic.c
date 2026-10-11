/**
 * @file test_nvic.c
 * @brief Host-side tests for drivers/src/nvic.c. Each test operates on a
 *        plain in-memory NvicRegisters_t / ScbRegisters_t standing in for
 *        the real peripheral - nvic.c's functions take the register
 *        blocks as parameters rather than reaching for the hardware NVIC
 *        and SCB macros, which is what makes this testable off-target.
 *        The in-memory struct has no write-1-to-set semantics, so a test
 *        asserts on what each register *received*: ISER/ICER/ISPR/ICPR
 *        hold exactly the one bit the driver wrote, because the driver
 *        never read-modify-writes them. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "nvic.h"
#include "unity.h"

#include <stddef.h>

/** Device IRQs spanning the first word, a word boundary, and the last implemented line. */
static const IRQn_e valid_irqs[] = {WWDG_IRQn,  EXTI0_IRQn, (IRQn_e)31, (IRQn_e)32,
                                    (IRQn_e)63, (IRQn_e)64, (IRQn_e)95, FMPI2C1_Error_IRQn};

/** IRQns that must never index the NVIC arrays: system exceptions and past the last line. */
static const IRQn_e invalid_irqs[] = {SysTick_IRQn, PendSV_IRQn, MemoryManagement_IRQn,
                                      (IRQn_e)-14,  (IRQn_e)97,  (IRQn_e)239};

#define COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))

/* --- enable / disable / pending: shared shape, one bit in one word --- */

/** nvicEnableIrq() writes only ISER[irq/32] = 1 << (irq%32). */
void test_nvic_driver_enable_irq_writes_single_iser_bit(void)
{
    for (size_t i = 0; i < COUNT_OF(valid_irqs); i++)
    {
        NvicRegisters_t nvic = {{0}};
        uint32_t irq = (uint32_t)valid_irqs[i];

        DriverStatus_e status = nvicEnableIrq(&nvic, valid_irqs[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(1UL << (irq % 32U), nvic.ISER[irq / 32U]);
        for (size_t w = 0; w < 8U; w++)
        {
            if (w != (irq / 32U))
            {
                TEST_ASSERT_EQUAL_HEX32(0U, nvic.ISER[w]);
            }
            TEST_ASSERT_EQUAL_HEX32(0U, nvic.ICER[w]);
            TEST_ASSERT_EQUAL_HEX32(0U, nvic.ISPR[w]);
            TEST_ASSERT_EQUAL_HEX32(0U, nvic.ICPR[w]);
        }
    }
}

/** nvicDisableIrq() writes only ICER[irq/32] = 1 << (irq%32). */
void test_nvic_driver_disable_irq_writes_single_icer_bit(void)
{
    for (size_t i = 0; i < COUNT_OF(valid_irqs); i++)
    {
        NvicRegisters_t nvic = {{0}};
        uint32_t irq = (uint32_t)valid_irqs[i];

        DriverStatus_e status = nvicDisableIrq(&nvic, valid_irqs[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(1UL << (irq % 32U), nvic.ICER[irq / 32U]);
        for (size_t w = 0; w < 8U; w++)
        {
            TEST_ASSERT_EQUAL_HEX32(0U, nvic.ISER[w]);
        }
    }
}

/** nvicSetPending() writes only ISPR[irq/32] = 1 << (irq%32). */
void test_nvic_driver_set_pending_writes_single_ispr_bit(void)
{
    for (size_t i = 0; i < COUNT_OF(valid_irqs); i++)
    {
        NvicRegisters_t nvic = {{0}};
        uint32_t irq = (uint32_t)valid_irqs[i];

        DriverStatus_e status = nvicSetPending(&nvic, valid_irqs[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(1UL << (irq % 32U), nvic.ISPR[irq / 32U]);
        for (size_t w = 0; w < 8U; w++)
        {
            TEST_ASSERT_EQUAL_HEX32(0U, nvic.ICPR[w]);
        }
    }
}

/** nvicClearPending() writes only ICPR[irq/32] = 1 << (irq%32). */
void test_nvic_driver_clear_pending_writes_single_icpr_bit(void)
{
    for (size_t i = 0; i < COUNT_OF(valid_irqs); i++)
    {
        NvicRegisters_t nvic = {{0}};
        uint32_t irq = (uint32_t)valid_irqs[i];

        DriverStatus_e status = nvicClearPending(&nvic, valid_irqs[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(1UL << (irq % 32U), nvic.ICPR[irq / 32U]);
        for (size_t w = 0; w < 8U; w++)
        {
            TEST_ASSERT_EQUAL_HEX32(0U, nvic.ISPR[w]);
        }
    }
}

/** Enable/disable/pending reject every out-of-range IRQn without writing anything. */
void test_nvic_driver_enable_disable_pending_reject_invalid_irq(void)
{
    for (size_t i = 0; i < COUNT_OF(invalid_irqs); i++)
    {
        NvicRegisters_t nvic = {{0}};
        NvicRegisters_t untouched = {{0}};

        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicEnableIrq(&nvic, invalid_irqs[i]));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicDisableIrq(&nvic, invalid_irqs[i]));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicSetPending(&nvic, invalid_irqs[i]));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM,
                          nvicClearPending(&nvic, invalid_irqs[i]));
        TEST_ASSERT_EQUAL_MEMORY(&untouched, &nvic, sizeof(nvic));
    }
}

/* --- device interrupt priority --- */

/** nvicSetPriority() puts the 4-bit priority in the top nibble of IP[irq] and touches no other
 * byte. */
void test_nvic_driver_set_priority_shifts_into_top_nibble(void)
{
    for (size_t i = 0; i < COUNT_OF(valid_irqs); i++)
    {
        for (uint32_t prio = 0U; prio <= NVIC_PRIORITY_MAX; prio++)
        {
            NvicRegisters_t nvic = {{0}};
            uint32_t irq = (uint32_t)valid_irqs[i];

            DriverStatus_e status = nvicSetPriority(&nvic, valid_irqs[i], prio);

            TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
            TEST_ASSERT_EQUAL_HEX8((uint8_t)(prio << 4), nvic.IP[irq]);
            TEST_ASSERT_EQUAL_HEX8(0U, nvic.IP[(irq == 0U) ? 1U : 0U]);
        }
    }
}

/** NVIC_PRIORITY_MAX is 15 on this 4-bit-priority part. */
void test_nvic_driver_priority_max_is_fifteen(void)
{
    TEST_ASSERT_EQUAL_UINT(15U, NVIC_PRIORITY_MAX);
}

/** nvicSetPriority() rejects a priority above NVIC_PRIORITY_MAX without writing. */
void test_nvic_driver_set_priority_rejects_out_of_range_priority(void)
{
    NvicRegisters_t nvic = {{0}};

    DriverStatus_e status = nvicSetPriority(&nvic, EXTI0_IRQn, NVIC_PRIORITY_MAX + 1U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX8(0U, nvic.IP[EXTI0_IRQn]);
}

/** nvicSetPriority() rejects an invalid IRQn without writing, even with a valid priority. */
void test_nvic_driver_set_priority_rejects_invalid_irq(void)
{
    for (size_t i = 0; i < COUNT_OF(invalid_irqs); i++)
    {
        NvicRegisters_t nvic = {{0}};
        NvicRegisters_t untouched = {{0}};

        DriverStatus_e status = nvicSetPriority(&nvic, invalid_irqs[i], 1U);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
        TEST_ASSERT_EQUAL_MEMORY(&untouched, &nvic, sizeof(nvic));
    }
}

/** nvicGetPriority() reads back what nvicSetPriority() stored, shifted back down. */
void test_nvic_driver_get_priority_round_trips(void)
{
    for (uint32_t prio = 0U; prio <= NVIC_PRIORITY_MAX; prio++)
    {
        NvicRegisters_t nvic = {{0}};
        uint32_t read_back = 0xFFFFFFFFU;

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicSetPriority(&nvic, FMPI2C1_Error_IRQn, prio));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicGetPriority(&nvic, FMPI2C1_Error_IRQn, &read_back));
        TEST_ASSERT_EQUAL_UINT32(prio, read_back);
    }
}

/** nvicGetPriority() ignores the unimplemented low nibble of the priority byte. */
void test_nvic_driver_get_priority_ignores_low_nibble(void)
{
    NvicRegisters_t nvic = {{0}};
    uint32_t read_back = 0U;

    nvic.IP[EXTI0_IRQn] = 0xA5U;

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicGetPriority(&nvic, EXTI0_IRQn, &read_back));
    TEST_ASSERT_EQUAL_UINT32(0xAU, read_back);
}

/** nvicGetPriority() rejects an invalid IRQn and leaves the output untouched. */
void test_nvic_driver_get_priority_rejects_invalid_irq(void)
{
    NvicRegisters_t nvic = {{0}};
    uint32_t read_back = 0x1234U;

    DriverStatus_e status = nvicGetPriority(&nvic, SysTick_IRQn, &read_back);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_UINT32(0x1234U, read_back);
}

/** nvicGetPriority() rejects a NULL output pointer instead of dereferencing it. */
void test_nvic_driver_get_priority_rejects_null_output(void)
{
    NvicRegisters_t nvic = {{0}};

    DriverStatus_e status = nvicGetPriority(&nvic, EXTI0_IRQn, NULL);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
}

/* --- system handler priority --- */

/** Each system handler's priority lands in its own byte lane of the right SHPR word. */
void test_nvic_driver_set_system_handler_priority_targets_correct_lane(void)
{
    static const struct
    {
        IRQn_e irqn;
        uint32_t shpr_index; /* 1, 2 or 3 */
        uint32_t lane_shift;
    } cases[] = {
        {MemoryManagement_IRQn, 1U, 0U}, {BusFault_IRQn, 1U, 8U}, {UsageFault_IRQn, 1U, 16U},
        {SVCall_IRQn, 2U, 24U},          {PendSV_IRQn, 3U, 16U},  {SysTick_IRQn, 3U, 24U},
    };

    for (size_t i = 0; i < COUNT_OF(cases); i++)
    {
        ScbRegisters_t scb = {0};

        DriverStatus_e status = nvicSetSystemHandlerPriority(&scb, cases[i].irqn, 0x9U);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32((0x9U << 4) << cases[i].lane_shift,
                                (cases[i].shpr_index == 1U)   ? scb.SHPR1
                                : (cases[i].shpr_index == 2U) ? scb.SHPR2
                                                              : scb.SHPR3);
        TEST_ASSERT_EQUAL_HEX32(0U, (cases[i].shpr_index == 1U) ? 0U : scb.SHPR1);
        TEST_ASSERT_EQUAL_HEX32(0U, (cases[i].shpr_index == 2U) ? 0U : scb.SHPR2);
        TEST_ASSERT_EQUAL_HEX32(0U, (cases[i].shpr_index == 3U) ? 0U : scb.SHPR3);
    }
}

/** Setting one handler preserves the other handler sharing its SHPR word. */
void test_nvic_driver_set_system_handler_priority_preserves_neighbour(void)
{
    ScbRegisters_t scb = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicSetSystemHandlerPriority(&scb, SysTick_IRQn, 0xFU));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicSetSystemHandlerPriority(&scb, PendSV_IRQn, 0x3U));
    TEST_ASSERT_EQUAL_HEX32(0xF0300000U, scb.SHPR3);

    /* Rewriting overwrites only its own byte, including bits that were set. */
    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicSetSystemHandlerPriority(&scb, PendSV_IRQn, 0x0U));
    TEST_ASSERT_EQUAL_HEX32(0xF0000000U, scb.SHPR3);
}

/** Priorities above NVIC_PRIORITY_MAX are rejected for a valid handler, with no write. */
void test_nvic_driver_set_system_handler_priority_rejects_out_of_range_priority(void)
{
    ScbRegisters_t scb = {0};

    DriverStatus_e status = nvicSetSystemHandlerPriority(&scb, PendSV_IRQn, NVIC_PRIORITY_MAX + 1U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, scb.SHPR3);
}

/** Device interrupts, NMI, HardFault and DebugMonitor are not configurable here. */
void test_nvic_driver_set_system_handler_priority_rejects_non_configurable(void)
{
    static const IRQn_e rejected[] = {WWDG_IRQn, FMPI2C1_Error_IRQn, (IRQn_e)-14, (IRQn_e)-13,
                                      (IRQn_e)-4};

    for (size_t i = 0; i < COUNT_OF(rejected); i++)
    {
        ScbRegisters_t scb = {0};

        DriverStatus_e status = nvicSetSystemHandlerPriority(&scb, rejected[i], 1U);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
        TEST_ASSERT_EQUAL_HEX32(0U, scb.SHPR1);
        TEST_ASSERT_EQUAL_HEX32(0U, scb.SHPR2);
        TEST_ASSERT_EQUAL_HEX32(0U, scb.SHPR3);
    }
}
