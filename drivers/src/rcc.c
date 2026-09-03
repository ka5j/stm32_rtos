/**
 * @file rcc.c
 * @brief RCC peripheral driver implementation - see rcc.h for the public
 *        API and rcc_reg.h for the register definitions this operates on.
 */
#include "rcc.h"

/**
 * @addtogroup driver_layer
 * @{
 */

/**
 * @brief Resolve a GPIO port register block to its RCC_AHB1ENR clock-gate
 *        bit. Shared by rccGpioClockEnable()/rccGpioClockDisable() so the
 *        GPIOA..GPIOH identity chain exists in exactly one place.
 *
 * @param port GPIO port register block to identify (e.g. GPIOA).
 * @param bit  Set to port's RCC_AHB1ENR_GPIOxEN bit on success; untouched
 *             on failure.
 * @return DRIVER_STATUS_OK on success.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if port is not one of
 *         GPIOA..GPIOH.
 */
static DriverStatus_e rccGpioClockBit(const GpioRegisters_t *port, uint32_t *bit)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if (port == GPIOA)
    {
        *bit = RCC_AHB1ENR_GPIOAEN;
    }
    else if (port == GPIOB)
    {
        *bit = RCC_AHB1ENR_GPIOBEN;
    }
    else if (port == GPIOC)
    {
        *bit = RCC_AHB1ENR_GPIOCEN;
    }
    else if (port == GPIOD)
    {
        *bit = RCC_AHB1ENR_GPIODEN;
    }
    else if (port == GPIOE)
    {
        *bit = RCC_AHB1ENR_GPIOEEN;
    }
    else if (port == GPIOF)
    {
        *bit = RCC_AHB1ENR_GPIOFEN;
    }
    else if (port == GPIOG)
    {
        *bit = RCC_AHB1ENR_GPIOGEN;
    }
    else if (port == GPIOH)
    {
        *bit = RCC_AHB1ENR_GPIOHEN;
    }
    else
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    return status;
}

DriverStatus_e rccGpioClockEnable(RccRegisters_t *rcc, const GpioRegisters_t *port)
{
    uint32_t bit = 0U;
    DriverStatus_e status = rccGpioClockBit(port, &bit);

    if (status == DRIVER_STATUS_OK)
    {
        rcc->AHB1ENR |= bit;
    }

    return status;
}

DriverStatus_e rccGpioClockDisable(RccRegisters_t *rcc, const GpioRegisters_t *port)
{
    uint32_t bit = 0U;
    DriverStatus_e status = rccGpioClockBit(port, &bit);

    if (status == DRIVER_STATUS_OK)
    {
        rcc->AHB1ENR &= ~bit;
    }

    return status;
}

void rccUsart2ClockEnable(RccRegisters_t *rcc)
{
    /* RCC_APB1ENR_USART2EN = (1U << 17U) (device/inc/rcc_reg.h) - a fixed,
     * tested compile-time bit position, not a runtime-computed shift
     * cppcheck's MISRA addon can't bound. */
    // cppcheck-suppress misra-c2012-12.2
    rcc->APB1ENR |= RCC_APB1ENR_USART2EN;
}

void rccUsart2ClockDisable(RccRegisters_t *rcc)
{
    // cppcheck-suppress misra-c2012-12.2
    rcc->APB1ENR &= ~RCC_APB1ENR_USART2EN;
}

void rccSyscfgClockEnable(RccRegisters_t *rcc)
{
    /* RCC_APB2ENR_SYSCFGEN = (1U << 14U) - see rccUsart2ClockEnable()'s
     * comment above. */
    // cppcheck-suppress misra-c2012-12.2
    rcc->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
}

void rccSyscfgClockDisable(RccRegisters_t *rcc)
{
    // cppcheck-suppress misra-c2012-12.2
    rcc->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;
}

void rccPwrClockEnable(RccRegisters_t *rcc)
{
    /* RCC_APB1ENR_PWREN = (1U << 28U) - see rccUsart2ClockEnable()'s
     * comment above. */
    // cppcheck-suppress misra-c2012-12.2
    rcc->APB1ENR |= RCC_APB1ENR_PWREN;
}

void rccPwrClockDisable(RccRegisters_t *rcc)
{
    // cppcheck-suppress misra-c2012-12.2
    rcc->APB1ENR &= ~RCC_APB1ENR_PWREN;
}

/** @} */
