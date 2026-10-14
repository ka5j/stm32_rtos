/**
 * @file exti.c
 * @brief EXTI driver implementation (API in exti.h).
 */
#include "exti.h"

#include <stddef.h>

/**
 * @addtogroup exti_driver
 * @{
 */

/** The line with no source on the F446 (RM0390 section 10.2.5). */
#define EXTI_LINE_UNUSED (19U)

/**
 * @brief Accept a line that exists: 0 to ::EXTI_LINE_MAX, except 19.
 * @param line Candidate line.
 * @retval DRIVER_STATUS_OK                Valid.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM Otherwise.
 */
static DriverStatus_e extiValidateLine(uint32_t line)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((line > EXTI_LINE_MAX) || (line == EXTI_LINE_UNUSED))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    return status;
}

/**
 * @brief Map a GPIO port to its EXTICR selector.
 * @param port     Port to identify, compared by address.
 * @param selector Receives a `SYSCFG_EXTICR_P*` value; untouched on failure.
 * @retval DRIVER_STATUS_OK                @p port is GPIOA to GPIOH.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM Otherwise.
 */
static DriverStatus_e extiPortSelector(const GpioRegisters_t *port, uint32_t *selector)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if (port == GPIOA)
    {
        *selector = SYSCFG_EXTICR_PA;
    }
    else if (port == GPIOB)
    {
        *selector = SYSCFG_EXTICR_PB;
    }
    else if (port == GPIOC)
    {
        *selector = SYSCFG_EXTICR_PC;
    }
    else if (port == GPIOD)
    {
        *selector = SYSCFG_EXTICR_PD;
    }
    else if (port == GPIOE)
    {
        *selector = SYSCFG_EXTICR_PE;
    }
    else if (port == GPIOF)
    {
        *selector = SYSCFG_EXTICR_PF;
    }
    else if (port == GPIOG)
    {
        *selector = SYSCFG_EXTICR_PG;
    }
    else if (port == GPIOH)
    {
        *selector = SYSCFG_EXTICR_PH;
    }
    else
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    return status;
}

DriverStatus_e extiSelectPort(SyscfgRegisters_t *syscfg, uint32_t line, const GpioRegisters_t *port)
{
    uint32_t selector = 0U;
    DriverStatus_e status = DRIVER_STATUS_OK;

    if (line > EXTI_GPIO_LINE_MAX)
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else
    {
        status = extiPortSelector(port, &selector);
    }

    /* GPIOH has only pins 0 and 1. */
    if ((status == DRIVER_STATUS_OK) && (selector == SYSCFG_EXTICR_PH) && (line > 1U))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        uint32_t index = line / 4U;
        uint32_t shift = (line % 4U) * 4U;

        syscfg->EXTICR[index] =
            (syscfg->EXTICR[index] & ~((uint32_t)0xFU << shift)) | (selector << shift);
    }

    return status;
}

DriverStatus_e extiConfigure(ExtiRegisters_t *exti, uint32_t line, ExtiEdge_e edge)
{
    DriverStatus_e status = extiValidateLine(line);

    if ((status == DRIVER_STATUS_OK) && (edge != EXTI_EDGE_RISING) && (edge != EXTI_EDGE_FALLING)
        && (edge != EXTI_EDGE_BOTH))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        uint32_t bit = (uint32_t)1U << line;

        if (edge == EXTI_EDGE_FALLING)
        {
            exti->RTSR &= ~bit;
        }
        else
        {
            exti->RTSR |= bit;
        }

        if (edge == EXTI_EDGE_RISING)
        {
            exti->FTSR &= ~bit;
        }
        else
        {
            exti->FTSR |= bit;
        }
    }

    return status;
}

DriverStatus_e extiEnableInterrupt(ExtiRegisters_t *exti, uint32_t line)
{
    DriverStatus_e status = extiValidateLine(line);

    if (status == DRIVER_STATUS_OK)
    {
        exti->IMR |= (uint32_t)1U << line;
    }

    return status;
}

DriverStatus_e extiDisableInterrupt(ExtiRegisters_t *exti, uint32_t line)
{
    DriverStatus_e status = extiValidateLine(line);

    if (status == DRIVER_STATUS_OK)
    {
        exti->IMR &= ~((uint32_t)1U << line);
    }

    return status;
}

DriverStatus_e extiTriggerSoftware(ExtiRegisters_t *exti, uint32_t line)
{
    DriverStatus_e status = extiValidateLine(line);

    if (status == DRIVER_STATUS_OK)
    {
        /* Only a 0-to-1 write triggers, so setting a bit that is already set is harmless. */
        exti->SWIER |= (uint32_t)1U << line;
    }

    return status;
}

DriverStatus_e extiIsPending(const ExtiRegisters_t *exti, uint32_t line, bool *pending)
{
    DriverStatus_e status = extiValidateLine(line);

    if ((status == DRIVER_STATUS_OK) && (pending == NULL))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        *pending = ((exti->PR & ((uint32_t)1U << line)) != 0U);
    }

    return status;
}

DriverStatus_e extiClearPending(ExtiRegisters_t *exti, uint32_t line)
{
    DriverStatus_e status = extiValidateLine(line);

    if (status == DRIVER_STATUS_OK)
    {
        /* A store, not |=: PR is write-1-to-clear, and reading it back would
         * write 1 to, and so clear, every other pending line. */
        exti->PR = (uint32_t)1U << line;
    }

    return status;
}

/** @} */
