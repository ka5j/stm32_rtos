/**
 * @file nvic.c
 * @brief NVIC driver implementation - see nvic.h for the public API and
 *        nvic_reg.h/scb_reg.h for the register definitions this operates
 *        on.
 */
#include "nvic.h"

#include <stddef.h>

/**
 * @addtogroup driver_layer
 * @{
 */

/**
 * @brief Reject an IRQn that does not name a real device interrupt.
 *
 * IRQn_e is a signed enum, so the system exceptions are negative, and C
 * does not constrain an enum-typed parameter to its enumerators. A
 * negative value would index ISER/IP below their start, and a value above
 * the last device interrupt would index past what this chip implements
 * (IP[] holds 240 bytes but the F446 has 97 lines). The reserved
 * positions inside the range are rejected too: they have no vector (the
 * table holds a zero there), so enabling one and then pending it would
 * take an exception through address 0.
 *
 * @param irqn Candidate interrupt number.
 * @return DRIVER_STATUS_OK if irqn is in 0 to ::FMPI2C1_Error_IRQn and
 *         not a reserved position.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM otherwise.
 */
static DriverStatus_e nvicValidateIrq(IRQn_e irqn)
{
    /* Reserved vector-table positions inside 0 to FMPI2C1_Error_IRQn, one
     * bit per position, indexed by ISER word (position / 32). They are the
     * zero slots in startup's vector table (nvic_reg.h lists them): 61-62,
     * 79-80, 82-83, 85-86 and 88-90. tools/check_vector_table.awk
     * cross-checks IRQn_e against that table but cannot see this mask, so
     * a change to the reserved set must be made in both places. */
    static const uint32_t reserved_mask[4] = {
        0x00000000U, /* 0-31: none */
        0x60000000U, /* 32-63: 61, 62 */
        0x076D8000U, /* 64-95: 79, 80, 82, 83, 85, 86, 88, 89, 90 */
        0x00000000U, /* 96: none */
    };
    DriverStatus_e status = DRIVER_STATUS_OK;

    if (((int32_t)irqn < 0) || ((int32_t)irqn > (int32_t)FMPI2C1_Error_IRQn))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else
    {
        uint32_t word = (uint32_t)irqn >> 5U;

        // cppcheck-suppress misra-c2012-12.2
        if ((reserved_mask[word] & (1U << ((uint32_t)irqn & 0x1FU))) != 0U)
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    return status;
}

/**
 * @brief Word index into ISER/ICER/ISPR/ICPR for a validated IRQn.
 * @param irqn Device interrupt number, already validated.
 * @return irqn / 32.
 */
static uint32_t nvicIrqWord(IRQn_e irqn)
{
    return (uint32_t)irqn >> 5U;
}

/**
 * @brief Bit mask within that word for a validated IRQn.
 * @param irqn Device interrupt number, already validated.
 * @return A single set bit at position irqn % 32.
 */
static uint32_t nvicIrqBit(IRQn_e irqn)
{
    // cppcheck-suppress misra-c2012-12.2
    return 1U << ((uint32_t)irqn & 0x1FU);
}

DriverStatus_e nvicEnableIrq(NvicRegisters_t *nvic, IRQn_e irqn)
{
    DriverStatus_e status = nvicValidateIrq(irqn);

    if (status == DRIVER_STATUS_OK)
    {
        nvic->ISER[nvicIrqWord(irqn)] = nvicIrqBit(irqn);
    }

    return status;
}

DriverStatus_e nvicDisableIrq(NvicRegisters_t *nvic, IRQn_e irqn)
{
    DriverStatus_e status = nvicValidateIrq(irqn);

    if (status == DRIVER_STATUS_OK)
    {
        nvic->ICER[nvicIrqWord(irqn)] = nvicIrqBit(irqn);
    }

    return status;
}

DriverStatus_e nvicSetPending(NvicRegisters_t *nvic, IRQn_e irqn)
{
    DriverStatus_e status = nvicValidateIrq(irqn);

    if (status == DRIVER_STATUS_OK)
    {
        nvic->ISPR[nvicIrqWord(irqn)] = nvicIrqBit(irqn);
    }

    return status;
}

DriverStatus_e nvicClearPending(NvicRegisters_t *nvic, IRQn_e irqn)
{
    DriverStatus_e status = nvicValidateIrq(irqn);

    if (status == DRIVER_STATUS_OK)
    {
        nvic->ICPR[nvicIrqWord(irqn)] = nvicIrqBit(irqn);
    }

    return status;
}

DriverStatus_e nvicSetPriority(NvicRegisters_t *nvic, IRQn_e irqn, uint32_t priority)
{
    DriverStatus_e status = nvicValidateIrq(irqn);

    if ((status == DRIVER_STATUS_OK) && (priority > NVIC_PRIORITY_MAX))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        // cppcheck-suppress misra-c2012-12.2
        nvic->IP[(uint32_t)irqn] = (uint8_t)(priority << NVIC_PRIO_Pos);
    }

    return status;
}

DriverStatus_e nvicGetPriority(const NvicRegisters_t *nvic, IRQn_e irqn, uint32_t *priority)
{
    DriverStatus_e status = nvicValidateIrq(irqn);

    if ((status == DRIVER_STATUS_OK) && (priority == NULL))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        *priority = (uint32_t)nvic->IP[(uint32_t)irqn] >> NVIC_PRIO_Pos;
    }

    return status;
}

DriverStatus_e nvicSetSystemHandlerPriority(ScbRegisters_t *scb, IRQn_e irqn, uint32_t priority)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    volatile uint32_t *shpr = &scb->SHPR1;
    uint32_t pos = 0U;
    uint32_t msk = 0U;

    /* Each handler's priority is one byte of SHPR1-3; picking the word and
     * byte lane by switch keeps the register access to named struct members
     * and the lane to the SCB_SHPRx_PRI_n macros, rather than computing an
     * address from the exception number. The misra-12.2 suppressions below
     * are on the *_Msk macros, which are `0xFFU << n` - cppcheck's MISRA
     * addon can't bound a shift through a macro expansion (see pwr.c's
     * matching comment on CR.VOS). */
    switch (irqn)
    {
        case MemoryManagement_IRQn:
            shpr = &scb->SHPR1;
            pos = SCB_SHPR1_PRI_4_Pos;
            msk = SCB_SHPR1_PRI_4_Msk;
            break;
        case BusFault_IRQn:
            shpr = &scb->SHPR1;
            pos = SCB_SHPR1_PRI_5_Pos;
            // cppcheck-suppress misra-c2012-12.2
            msk = SCB_SHPR1_PRI_5_Msk;
            break;
        case UsageFault_IRQn:
            shpr = &scb->SHPR1;
            pos = SCB_SHPR1_PRI_6_Pos;
            // cppcheck-suppress misra-c2012-12.2
            msk = SCB_SHPR1_PRI_6_Msk;
            break;
        case SVCall_IRQn:
            shpr = &scb->SHPR2;
            pos = SCB_SHPR2_PRI_11_Pos;
            // cppcheck-suppress misra-c2012-12.2
            msk = SCB_SHPR2_PRI_11_Msk;
            break;
        case PendSV_IRQn:
            shpr = &scb->SHPR3;
            pos = SCB_SHPR3_PRI_14_Pos;
            // cppcheck-suppress misra-c2012-12.2
            msk = SCB_SHPR3_PRI_14_Msk;
            break;
        case SysTick_IRQn:
            shpr = &scb->SHPR3;
            pos = SCB_SHPR3_PRI_15_Pos;
            // cppcheck-suppress misra-c2012-12.2
            msk = SCB_SHPR3_PRI_15_Msk;
            break;
        default:
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
            break;
    }

    if ((status == DRIVER_STATUS_OK) && (priority > NVIC_PRIORITY_MAX))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        // cppcheck-suppress misra-c2012-12.2
        *shpr = (*shpr & ~msk) | ((priority << NVIC_PRIO_Pos) << pos);
    }

    return status;
}

/** @} */
