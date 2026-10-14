/**
 * @file test_exti.c
 * @brief Host-side tests for drivers/src/exti.c, on in-memory
 *        ExtiRegisters_t and SyscfgRegisters_t. They check what the driver
 *        writes; that a pin edge or SWIER really sets PR is a target check
 *        (tests/target/smoke.c).
 */
#include "exti.h"
#include "unity.h"

#include <stddef.h>
#include <string.h>

/** Each port lands in the right EXTICR word and field, with its selector. */
void test_exti_driver_select_port_writes_each_port_selector(void)
{
    const GpioRegisters_t *ports[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG};
    const uint32_t selectors[] = {SYSCFG_EXTICR_PA, SYSCFG_EXTICR_PB, SYSCFG_EXTICR_PC,
                                  SYSCFG_EXTICR_PD, SYSCFG_EXTICR_PE, SYSCFG_EXTICR_PF,
                                  SYSCFG_EXTICR_PG};

    for (size_t i = 0U; i < (sizeof ports / sizeof ports[0]); i++)
    {
        SyscfgRegisters_t syscfg = {0};

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiSelectPort(&syscfg, 13U, ports[i]));
        /* Line 13: EXTICR[3], bits 7:4. */
        TEST_ASSERT_EQUAL_HEX32(selectors[i] << 4U, syscfg.EXTICR[3]);
    }
}

/** Lines 0, 5 and 15 map to EXTICR[0] bits 3:0, EXTICR[1] bits 7:4 and EXTICR[3] bits 15:12. */
void test_exti_driver_select_port_places_field_by_line(void)
{
    SyscfgRegisters_t syscfg = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiSelectPort(&syscfg, 0U, GPIOG));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiSelectPort(&syscfg, 5U, GPIOC));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiSelectPort(&syscfg, 15U, GPIOB));

    TEST_ASSERT_EQUAL_HEX32(SYSCFG_EXTICR_PG, syscfg.EXTICR[0]);
    TEST_ASSERT_EQUAL_HEX32(SYSCFG_EXTICR_PC << 4U, syscfg.EXTICR[1]);
    TEST_ASSERT_EQUAL_HEX32(0U, syscfg.EXTICR[2]);
    TEST_ASSERT_EQUAL_HEX32(SYSCFG_EXTICR_PB << 12U, syscfg.EXTICR[3]);
}

/** Re-routing a line replaces only its own 4-bit field. */
void test_exti_driver_select_port_preserves_other_fields(void)
{
    SyscfgRegisters_t syscfg = {.EXTICR = {0U, 0U, 0U, 0xFFFFFFFFU}};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiSelectPort(&syscfg, 13U, GPIOA));

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFF0FU, syscfg.EXTICR[3]);
}

/** GPIOH has pins 0 and 1 only. */
void test_exti_driver_select_port_limits_gpioh_to_lines_0_and_1(void)
{
    SyscfgRegisters_t syscfg = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiSelectPort(&syscfg, 0U, GPIOH));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiSelectPort(&syscfg, 1U, GPIOH));
    TEST_ASSERT_EQUAL_HEX32((SYSCFG_EXTICR_PH << 4U) | SYSCFG_EXTICR_PH, syscfg.EXTICR[0]);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiSelectPort(&syscfg, 2U, GPIOH));
    TEST_ASSERT_EQUAL_HEX32((SYSCFG_EXTICR_PH << 4U) | SYSCFG_EXTICR_PH, syscfg.EXTICR[0]);
}

/** Lines above 15 have no GPIO source, and an unknown port is rejected; nothing is written. */
void test_exti_driver_select_port_rejects_invalid_line_or_port(void)
{
    SyscfgRegisters_t syscfg = {0};
    GpioRegisters_t not_a_port = {0};
    SyscfgRegisters_t untouched = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiSelectPort(&syscfg, 16U, GPIOA));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiSelectPort(&syscfg, 3U, &not_a_port));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiSelectPort(&syscfg, 3U, NULL));
    TEST_ASSERT_EQUAL_MEMORY(&untouched, &syscfg, sizeof syscfg);
}

/** Rising sets RTSR and clears FTSR; falling the reverse; both sets both. */
void test_exti_driver_configure_sets_edges(void)
{
    ExtiRegisters_t exti = {.RTSR = 1U << 7U, .FTSR = 1U << 7U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiConfigure(&exti, 7U, EXTI_EDGE_RISING));
    TEST_ASSERT_EQUAL_HEX32(1U << 7U, exti.RTSR);
    TEST_ASSERT_EQUAL_HEX32(0U, exti.FTSR);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiConfigure(&exti, 7U, EXTI_EDGE_FALLING));
    TEST_ASSERT_EQUAL_HEX32(0U, exti.RTSR);
    TEST_ASSERT_EQUAL_HEX32(1U << 7U, exti.FTSR);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiConfigure(&exti, 7U, EXTI_EDGE_BOTH));
    TEST_ASSERT_EQUAL_HEX32(1U << 7U, exti.RTSR);
    TEST_ASSERT_EQUAL_HEX32(1U << 7U, exti.FTSR);
}

/** Configuring one line leaves every other line's edges alone. */
void test_exti_driver_configure_preserves_other_lines(void)
{
    ExtiRegisters_t exti = {.RTSR = 0xFFFFFFFFU, .FTSR = 0U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiConfigure(&exti, 22U, EXTI_EDGE_FALLING));

    TEST_ASSERT_EQUAL_HEX32(0xFFBFFFFFU, exti.RTSR);
    TEST_ASSERT_EQUAL_HEX32(1U << 22U, exti.FTSR);
}

/** An edge outside the enum, line 19 or a line past 22 is rejected and nothing is written. */
void test_exti_driver_configure_rejects_invalid_edge_or_line(void)
{
    ExtiRegisters_t exti = {0};
    ExtiRegisters_t untouched = {0};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiConfigure(&exti, 0U, (ExtiEdge_e)0));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiConfigure(&exti, 0U, (ExtiEdge_e)4));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiConfigure(&exti, 19U, EXTI_EDGE_RISING));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiConfigure(&exti, 23U, EXTI_EDGE_RISING));
    TEST_ASSERT_EQUAL_MEMORY(&untouched, &exti, sizeof exti);
}

/** IMR gains and loses only the line's bit. */
void test_exti_driver_enable_disable_interrupt_touch_one_imr_bit(void)
{
    ExtiRegisters_t exti = {.IMR = 0x00000005U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiEnableInterrupt(&exti, 13U));
    TEST_ASSERT_EQUAL_HEX32(0x00002005U, exti.IMR);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiDisableInterrupt(&exti, 0U));
    TEST_ASSERT_EQUAL_HEX32(0x00002004U, exti.IMR);
}

/** Line 0 and line 22 are the bounds; 19 and 23 are rejected by every line-taking function. */
void test_exti_driver_line_bounds(void)
{
    ExtiRegisters_t exti = {0};
    bool pending = false;

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiEnableInterrupt(&exti, 0U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiEnableInterrupt(&exti, 22U));
    TEST_ASSERT_EQUAL_HEX32((1U << 22U) | 1U, exti.IMR);

    exti = (ExtiRegisters_t){0};
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiEnableInterrupt(&exti, 19U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiDisableInterrupt(&exti, 23U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiTriggerSoftware(&exti, 19U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiClearPending(&exti, 23U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiIsPending(&exti, 19U, &pending));
    TEST_ASSERT_EQUAL_HEX32(0U, exti.IMR);
    TEST_ASSERT_EQUAL_HEX32(0U, exti.SWIER);
    TEST_ASSERT_EQUAL_HEX32(0U, exti.PR);
}

/** Disabling a line does not clear its pending bit. */
void test_exti_driver_disable_interrupt_leaves_pending(void)
{
    ExtiRegisters_t exti = {.IMR = 1U << 4U, .PR = 1U << 4U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiDisableInterrupt(&exti, 4U));

    TEST_ASSERT_EQUAL_HEX32(0U, exti.IMR);
    TEST_ASSERT_EQUAL_HEX32(1U << 4U, exti.PR);
}

/** SWIER gains the line's bit and keeps the others. */
void test_exti_driver_trigger_software_sets_swier_bit(void)
{
    ExtiRegisters_t exti = {.SWIER = 1U << 1U};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiTriggerSoftware(&exti, 13U));

    TEST_ASSERT_EQUAL_HEX32((1U << 13U) | (1U << 1U), exti.SWIER);
}

/** Reports the line's own PR bit, set or clear. */
void test_exti_driver_is_pending_reads_pr_bit(void)
{
    ExtiRegisters_t exti = {.PR = 1U << 13U};
    bool pending = false;

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiIsPending(&exti, 13U, &pending));
    TEST_ASSERT_TRUE(pending);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiIsPending(&exti, 12U, &pending));
    TEST_ASSERT_FALSE(pending);
}

/** A NULL output is rejected, and an invalid line leaves the output untouched. */
void test_exti_driver_is_pending_rejects_null_and_keeps_output_on_failure(void)
{
    ExtiRegisters_t exti = {.PR = 0xFFFFFFFFU};
    bool pending = false;

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiIsPending(&exti, 13U, NULL));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiIsPending(&exti, 23U, &pending));
    TEST_ASSERT_FALSE(pending);
}

/** Clearing stores exactly the line's bit: |= would write 1 to, and clear, every pending line. */
void test_exti_driver_clear_pending_stores_only_its_bit(void)
{
    ExtiRegisters_t exti = {.PR = (1U << 13U) | (1U << 2U)};

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, extiClearPending(&exti, 13U));

    TEST_ASSERT_EQUAL_HEX32(1U << 13U, exti.PR);
}

/** Rejections from the edge or line checks leave every register at its old value. */
void test_exti_driver_rejections_write_nothing(void)
{
    ExtiRegisters_t exti;
    ExtiRegisters_t before;

    (void)memset(&exti, 0x5A, sizeof exti);
    before = exti;

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiEnableInterrupt(&exti, 19U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiDisableInterrupt(&exti, 19U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiTriggerSoftware(&exti, 23U));
    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, extiClearPending(&exti, 19U));
    TEST_ASSERT_EQUAL_MEMORY(&before, &exti, sizeof exti);
}
