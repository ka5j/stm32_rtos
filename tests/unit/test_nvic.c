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

/** Reserved vector-table positions inside 0 to FMPI2C1_Error_IRQn (the zero slots in startup's
 *  vector table, listed in nvic_reg.h). IRQn_e names none of them. */
static const uint32_t reserved_irqs[] = {61U, 62U, 79U, 80U, 82U, 83U, 85U, 86U, 88U, 89U, 90U};

/* A non-zero pattern in every write-1-to-act word, so a driver that read-modify-writes
 * (`ICER |= bit`, which on silicon would write back every enabled line and disable them all)
 * leaves the pattern's bits behind and is caught; a plain `= bit` leaves exactly one bit. */
#define PATTERN (0xA5A5A5A5U)

static void fill_write1_banks(NvicRegisters_t *nvic)
{
    for (size_t w = 0; w < 8U; w++)
    {
        nvic->ISER[w] = PATTERN;
        nvic->ICER[w] = PATTERN;
        nvic->ISPR[w] = PATTERN;
        nvic->ICPR[w] = PATTERN;
    }
}

/** Assert `bank[word]` holds exactly `bit` and every other word of every bank is still PATTERN. */
static void assert_only_one_bank_word_written(const NvicRegisters_t *nvic,
                                              const volatile uint32_t *bank, uint32_t word,
                                              uint32_t bit)
{
    const volatile uint32_t *banks[] = {nvic->ISER, nvic->ICER, nvic->ISPR, nvic->ICPR};

    for (size_t b = 0; b < COUNT_OF(banks); b++)
    {
        for (uint32_t w = 0; w < 8U; w++)
        {
            if ((banks[b] == bank) && (w == word))
            {
                TEST_ASSERT_EQUAL_HEX32(bit, banks[b][w]);
            }
            else
            {
                TEST_ASSERT_EQUAL_HEX32(PATTERN, banks[b][w]);
            }
        }
    }
}

/* --- enable / disable / pending: shared shape, one bit in one word --- */

/** nvicEnableIrq() writes only ISER[irq/32] = 1 << (irq%32), with no read-modify-write. */
void test_nvic_driver_enable_irq_writes_single_iser_bit(void)
{
    for (size_t i = 0; i < COUNT_OF(valid_irqs); i++)
    {
        NvicRegisters_t nvic = {0};
        uint32_t irq = (uint32_t)valid_irqs[i];
        fill_write1_banks(&nvic);

        DriverStatus_e status = nvicEnableIrq(&nvic, valid_irqs[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        assert_only_one_bank_word_written(&nvic, nvic.ISER, irq / 32U, 1U << (irq % 32U));
    }
}

/** nvicDisableIrq() writes only ICER[irq/32] = 1 << (irq%32), with no read-modify-write. */
void test_nvic_driver_disable_irq_writes_single_icer_bit(void)
{
    for (size_t i = 0; i < COUNT_OF(valid_irqs); i++)
    {
        NvicRegisters_t nvic = {0};
        uint32_t irq = (uint32_t)valid_irqs[i];
        fill_write1_banks(&nvic);

        DriverStatus_e status = nvicDisableIrq(&nvic, valid_irqs[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        assert_only_one_bank_word_written(&nvic, nvic.ICER, irq / 32U, 1U << (irq % 32U));
    }
}

/** nvicSetPending() writes only ISPR[irq/32] = 1 << (irq%32), with no read-modify-write. */
void test_nvic_driver_set_pending_writes_single_ispr_bit(void)
{
    for (size_t i = 0; i < COUNT_OF(valid_irqs); i++)
    {
        NvicRegisters_t nvic = {0};
        uint32_t irq = (uint32_t)valid_irqs[i];
        fill_write1_banks(&nvic);

        DriverStatus_e status = nvicSetPending(&nvic, valid_irqs[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        assert_only_one_bank_word_written(&nvic, nvic.ISPR, irq / 32U, 1U << (irq % 32U));
    }
}

/** nvicClearPending() writes only ICPR[irq/32] = 1 << (irq%32), with no read-modify-write. */
void test_nvic_driver_clear_pending_writes_single_icpr_bit(void)
{
    for (size_t i = 0; i < COUNT_OF(valid_irqs); i++)
    {
        NvicRegisters_t nvic = {0};
        uint32_t irq = (uint32_t)valid_irqs[i];
        fill_write1_banks(&nvic);

        DriverStatus_e status = nvicClearPending(&nvic, valid_irqs[i]);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        assert_only_one_bank_word_written(&nvic, nvic.ICPR, irq / 32U, 1U << (irq % 32U));
    }
}

/** Enable/disable/pending reject every out-of-range IRQn without writing anything. */
void test_nvic_driver_enable_disable_pending_reject_invalid_irq(void)
{
    for (size_t i = 0; i < COUNT_OF(invalid_irqs); i++)
    {
        NvicRegisters_t nvic = {0};
        NvicRegisters_t untouched = {0};

        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicEnableIrq(&nvic, invalid_irqs[i]));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicDisableIrq(&nvic, invalid_irqs[i]));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicSetPending(&nvic, invalid_irqs[i]));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM,
                          nvicClearPending(&nvic, invalid_irqs[i]));
        TEST_ASSERT_EQUAL_MEMORY(&untouched, &nvic, sizeof(nvic));
    }
}

/** Reserved positions inside the range have no vector (startup holds a zero there), so enabling
 *  and then pending one would fault; every function that takes an IRQn rejects them. */
void test_nvic_driver_rejects_reserved_irq_positions(void)
{
    for (size_t i = 0; i < COUNT_OF(reserved_irqs); i++)
    {
        NvicRegisters_t nvic = {0};
        NvicRegisters_t untouched = {0};
        IRQn_e irqn = (IRQn_e)reserved_irqs[i];
        uint32_t priority = 0U;

        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicEnableIrq(&nvic, irqn));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicDisableIrq(&nvic, irqn));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicSetPending(&nvic, irqn));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicClearPending(&nvic, irqn));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicSetPriority(&nvic, irqn, 1U));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, nvicGetPriority(&nvic, irqn, &priority));
        TEST_ASSERT_EQUAL_MEMORY(&untouched, &nvic, sizeof(nvic));
    }
}

/** Every position in 0 to FMPI2C1_Error_IRQn is accepted unless it is one of the reserved ones. */
void test_nvic_driver_accepts_every_non_reserved_position(void)
{
    for (uint32_t irq = 0U; irq <= (uint32_t)FMPI2C1_Error_IRQn; irq++)
    {
        NvicRegisters_t nvic = {0};
        int is_reserved = 0;

        for (size_t i = 0; i < COUNT_OF(reserved_irqs); i++)
        {
            if (reserved_irqs[i] == irq)
            {
                is_reserved = 1;
            }
        }

        DriverStatus_e status = nvicEnableIrq(&nvic, (IRQn_e)irq);

        TEST_ASSERT_EQUAL((is_reserved != 0) ? DRIVER_STATUS_ERR_INVALID_PARAM : DRIVER_STATUS_OK,
                          status);
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
            NvicRegisters_t nvic = {0};
            uint32_t irq = (uint32_t)valid_irqs[i];

            DriverStatus_e status = nvicSetPriority(&nvic, valid_irqs[i], prio);

            TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
            TEST_ASSERT_EQUAL_HEX8((uint8_t)(prio << 4), nvic.IP[irq]);
            for (uint32_t j = 0U; j < sizeof(nvic.IP); j++)
            {
                if (j != irq)
                {
                    TEST_ASSERT_EQUAL_HEX8(0U, nvic.IP[j]);
                }
            }
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
    NvicRegisters_t nvic = {0};

    DriverStatus_e status = nvicSetPriority(&nvic, EXTI0_IRQn, NVIC_PRIORITY_MAX + 1U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX8(0U, nvic.IP[EXTI0_IRQn]);
}

/** nvicSetPriority() rejects an invalid IRQn without writing, even with a valid priority. */
void test_nvic_driver_set_priority_rejects_invalid_irq(void)
{
    for (size_t i = 0; i < COUNT_OF(invalid_irqs); i++)
    {
        NvicRegisters_t nvic = {0};
        NvicRegisters_t untouched = {0};

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
        NvicRegisters_t nvic = {0};
        uint32_t read_back = 0xFFFFFFFFU;

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicSetPriority(&nvic, FMPI2C1_Error_IRQn, prio));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicGetPriority(&nvic, FMPI2C1_Error_IRQn, &read_back));
        TEST_ASSERT_EQUAL_UINT32(prio, read_back);
    }
}

/** nvicGetPriority() ignores the unimplemented low nibble of the priority byte. */
void test_nvic_driver_get_priority_ignores_low_nibble(void)
{
    NvicRegisters_t nvic = {0};
    uint32_t read_back = 0U;

    nvic.IP[EXTI0_IRQn] = 0xA5U;

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicGetPriority(&nvic, EXTI0_IRQn, &read_back));
    TEST_ASSERT_EQUAL_UINT32(0xAU, read_back);
}

/** nvicGetPriority() rejects an invalid IRQn and leaves the output untouched. */
void test_nvic_driver_get_priority_rejects_invalid_irq(void)
{
    NvicRegisters_t nvic = {0};
    uint32_t read_back = 0x1234U;

    DriverStatus_e status = nvicGetPriority(&nvic, SysTick_IRQn, &read_back);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_UINT32(0x1234U, read_back);
}

/** nvicGetPriority() rejects a NULL output pointer instead of dereferencing it. */
void test_nvic_driver_get_priority_rejects_null_output(void)
{
    NvicRegisters_t nvic = {0};

    DriverStatus_e status = nvicGetPriority(&nvic, EXTI0_IRQn, NULL);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
}

/* --- system handler priority --- */

/** Each system handler's priority lands in its own byte lane of the right SHPR word, and every
 *  other bit of all three words is left exactly as it was (preloaded with a pattern). */
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
        uint32_t expected[3] = {0x11223344U, 0x55667788U, 0x99AABBCCU};

        scb.SHPR1 = expected[0];
        scb.SHPR2 = expected[1];
        scb.SHPR3 = expected[2];
        expected[cases[i].shpr_index - 1U] =
            (expected[cases[i].shpr_index - 1U] & ~(0xFFU << cases[i].lane_shift))
            | ((0x9U << 4) << cases[i].lane_shift);

        DriverStatus_e status = nvicSetSystemHandlerPriority(&scb, cases[i].irqn, 0x9U);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(expected[0], scb.SHPR1);
        TEST_ASSERT_EQUAL_HEX32(expected[1], scb.SHPR2);
        TEST_ASSERT_EQUAL_HEX32(expected[2], scb.SHPR3);
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

/* --- review follow-ups --- */

/** nvicSetPriority overwrites the old value (IP preloaded to 0xA5, result 0x30 - an OR would give
 *  0xB5) and leaves every neighbouring byte exactly as it was. */
void test_nvic_driver_set_priority_overwrites_and_leaves_neighbour_bytes(void)
{
    NvicRegisters_t nvic = {0};

    for (size_t i = 0; i < sizeof(nvic.IP); i++)
    {
        nvic.IP[i] = 0xA5U;
    }

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, nvicSetPriority(&nvic, EXTI0_IRQn, 3U));

    for (size_t i = 0; i < sizeof(nvic.IP); i++)
    {
        TEST_ASSERT_EQUAL_HEX8((i == (size_t)EXTI0_IRQn) ? 0x30U : 0xA5U, nvic.IP[i]);
    }
}
