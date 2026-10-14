/**
 * @file nvic.c
 * @brief NVIC driver implementation (API in nvic.h).
 */
#include "nvic.h"

#include <stddef.h>

/**
 * @addtogroup nvic_driver
 * @{
 */

/**
 * @brief Accept only a real device interrupt.
 *
 * Negative values (system exceptions) and values past ::FMPI2C1_Error_IRQn
 * would index outside the implemented banks. Reserved positions have no
 * vector, so pending one would jump through address 0.
 *
 * @param irqn Candidate interrupt.
 * @retval DRIVER_STATUS_OK                0 to ::FMPI2C1_Error_IRQn and not reserved.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM Otherwise.
 */
static DriverStatus_e nvicValidateIrq(IRQn_e irqn)
{
    /* One bit per reserved position (61-62, 79-80, 82-83, 85-86, 88-90), by
     * ISER word. tools/check_vector_table.awk cannot see this mask: keep it in
     * step with the vector table by hand (test_nvic.c checks every position). */
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

        if ((reserved_mask[word] & (1U << ((uint32_t)irqn & 0x1FU))) != 0U)
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    return status;
}

/**
 * @brief Compiler barrier: stops stores being reordered across the ISER write
 *        (as CMSIS `__NVIC_EnableIRQ()` does). Emits no instruction.
 */
static inline void nvicCompilerBarrier(void)
{
    __asm__ volatile("" ::: "memory");
}

/**
 * @brief DSB then ISB on the target (a compiler barrier on the host), so a
 *        disabled interrupt cannot be taken after nvicDisableIrq() returns.
 */
static inline void nvicSyncBarrier(void)
{
#if defined(__arm__)
    __asm__ volatile("dsb 0xF\n\tisb 0xF" ::: "memory");
#else
    nvicCompilerBarrier();
#endif
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
    return 1U << ((uint32_t)irqn & 0x1FU);
}

DriverStatus_e nvicEnableIrq(NvicRegisters_t *nvic, IRQn_e irqn)
{
    DriverStatus_e status = nvicValidateIrq(irqn);

    if (status == DRIVER_STATUS_OK)
    {
        nvicCompilerBarrier();
        nvic->ISER[nvicIrqWord(irqn)] = nvicIrqBit(irqn);
        nvicCompilerBarrier();
    }

    return status;
}

DriverStatus_e nvicDisableIrq(NvicRegisters_t *nvic, IRQn_e irqn)
{
    DriverStatus_e status = nvicValidateIrq(irqn);

    if (status == DRIVER_STATUS_OK)
    {
        nvic->ICER[nvicIrqWord(irqn)] = nvicIrqBit(irqn);
        nvicSyncBarrier();
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

    /* Pick the SHPR word and byte by name rather than computing an address. */
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
            msk = SCB_SHPR1_PRI_5_Msk;
            break;
        case UsageFault_IRQn:
            shpr = &scb->SHPR1;
            pos = SCB_SHPR1_PRI_6_Pos;
            msk = SCB_SHPR1_PRI_6_Msk;
            break;
        case SVCall_IRQn:
            shpr = &scb->SHPR2;
            pos = SCB_SHPR2_PRI_11_Pos;
            msk = SCB_SHPR2_PRI_11_Msk;
            break;
        case PendSV_IRQn:
            shpr = &scb->SHPR3;
            pos = SCB_SHPR3_PRI_14_Pos;
            msk = SCB_SHPR3_PRI_14_Msk;
            break;
        case SysTick_IRQn:
            shpr = &scb->SHPR3;
            pos = SCB_SHPR3_PRI_15_Pos;
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
        *shpr = (*shpr & ~msk) | ((priority << NVIC_PRIO_Pos) << pos);
    }

    return status;
}

/** @} */
