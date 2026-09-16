/**
 * @file rcc.c
 * @brief RCC peripheral driver implementation - see rcc.h for the public
 *        API and rcc_reg.h for the register definitions this operates on.
 */
#include "rcc.h"

#include "flash.h"
#include "pwr.h"

/**
 * @addtogroup driver_layer
 * @{
 */

/** Bounded retry count for CR.HSIRDY/HSERDY/PLLRDY and CFGR.SWS, not a
 *  wall-clock timeout - see rcc.h's file-level comment for why. */
#define RCC_CLOCK_READY_TIMEOUT_ITERATIONS (100000U)

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

/**
 * @brief Read CFGR.SWS - the SYSCLK source currently active in hardware,
 *        as reported back by the switch itself (as opposed to CFGR.SW,
 *        the source software last requested). Shared by every disable
 *        function's busy guard.
 *
 * @param rcc RCC register block (e.g. RCC).
 * @return One of ::RCC_CFGR_SYSCLK_HSI, ::RCC_CFGR_SYSCLK_HSE,
 *         ::RCC_CFGR_SYSCLK_PLL.
 */
static uint32_t rccActiveSysclkSource(const RccRegisters_t *rcc)
{
    return (rcc->CFGR & RCC_CFGR_SWS_Msk) >> RCC_CFGR_SWS_Pos;
}

DriverStatus_e rccHsiEnable(RccRegisters_t *rcc)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = RCC_CLOCK_READY_TIMEOUT_ITERATIONS;

    rcc->CR |= RCC_CR_HSION;

    while (((rcc->CR & RCC_CR_HSIRDY) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }

    return status;
}

DriverStatus_e rccHsiDisable(RccRegisters_t *rcc)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if (rccActiveSysclkSource(rcc) == RCC_CFGR_SYSCLK_HSI)
    {
        status = DRIVER_STATUS_ERR_BUSY;
    }
    else
    {
        rcc->CR &= ~RCC_CR_HSION;
    }

    return status;
}

DriverStatus_e rccHseEnable(RccRegisters_t *rcc, uint32_t bypass)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    /* RCC_CR_HSEBYP/_HSEON/_HSERDY are fixed, tested compile-time bit
     * positions (device/inc/rcc_reg.h), not runtime-computed shifts -
     * cppcheck's MISRA addon can't bound a shift through a macro
     * expansion like this and flags every reference to it, the same as
     * the gating functions above (rccUsart2ClockEnable()'s comment).
     * Suppressed at each reference below, not just the definition, since
     * that is where the addon (mis)attributes the finding. */
    // cppcheck-suppress misra-c2012-12.2
    if ((bypass != 0U) && (bypass != RCC_CR_HSEBYP))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    // cppcheck-suppress misra-c2012-12.2
    else if ((rcc->CR & RCC_CR_HSEON) != 0U)
    {
        status = DRIVER_STATUS_ERR_BUSY;
    }
    else
    {
        uint32_t timeout = RCC_CLOCK_READY_TIMEOUT_ITERATIONS;

        /* HSEBYP must be written while HSEON is clear (RM0390) - the BUSY
         * guard above already ensures that. */
        // cppcheck-suppress misra-c2012-12.2
        rcc->CR = (rcc->CR & ~RCC_CR_HSEBYP) | bypass;
        // cppcheck-suppress misra-c2012-12.2
        rcc->CR |= RCC_CR_HSEON;

        // cppcheck-suppress misra-c2012-12.2
        while (((rcc->CR & RCC_CR_HSERDY) == 0U) && (timeout > 0U))
        {
            timeout--;
        }

        if (timeout == 0U)
        {
            status = DRIVER_STATUS_ERR_TIMEOUT;
        }
    }

    return status;
}

DriverStatus_e rccHseDisable(RccRegisters_t *rcc)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if (rccActiveSysclkSource(rcc) == RCC_CFGR_SYSCLK_HSE)
    {
        status = DRIVER_STATUS_ERR_BUSY;
    }
    else
    {
        // cppcheck-suppress misra-c2012-12.2
        rcc->CR &= ~RCC_CR_HSEON;
    }

    return status;
}

DriverStatus_e rccPllConfig(RccRegisters_t *rcc, uint32_t source, uint32_t m, uint32_t n,
                            uint32_t p)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    // cppcheck-suppress misra-c2012-12.2
    if ((source != RCC_PLLCFGR_PLLSRC_HSI) && (source != RCC_PLLCFGR_PLLSRC_HSE))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else if ((m < 2U) || (m > 63U))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else if ((n < 50U) || (n > 432U))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    // cppcheck-suppress misra-c2012-12.2
    else if (p > (RCC_PLLCFGR_PLLP_Msk >> RCC_PLLCFGR_PLLP_Pos))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    // cppcheck-suppress misra-c2012-12.2
    else if ((rcc->CR & RCC_CR_PLLON) != 0U)
    {
        status = DRIVER_STATUS_ERR_BUSY;
    }
    else
    {
        uint32_t mask =
            // cppcheck-suppress misra-c2012-12.2
            RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk | RCC_PLLCFGR_PLLSRC;

        /* PLLQ and any reserved bits are preserved, not touched - this
         * project configures PLLM/PLLN/PLLP/PLLSRC only (rcc_reg.h's
         * file-level comment: PLLQ is out of scope, no USB/SDIO here). */
        rcc->PLLCFGR = (rcc->PLLCFGR & ~mask) | (m << RCC_PLLCFGR_PLLM_Pos)
                       | (n << RCC_PLLCFGR_PLLN_Pos) | (p << RCC_PLLCFGR_PLLP_Pos) | source;
    }

    return status;
}

DriverStatus_e rccPllEnable(RccRegisters_t *rcc)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = RCC_CLOCK_READY_TIMEOUT_ITERATIONS;

    // cppcheck-suppress misra-c2012-12.2
    rcc->CR |= RCC_CR_PLLON;

    // cppcheck-suppress misra-c2012-12.2
    while (((rcc->CR & RCC_CR_PLLRDY) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }

    return status;
}

DriverStatus_e rccPllDisable(RccRegisters_t *rcc)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if (rccActiveSysclkSource(rcc) == RCC_CFGR_SYSCLK_PLL)
    {
        status = DRIVER_STATUS_ERR_BUSY;
    }
    else
    {
        // cppcheck-suppress misra-c2012-12.2
        rcc->CR &= ~RCC_CR_PLLON;
    }

    return status;
}

/**
 * @brief Validate a CFGR.HPRE field value against its 9 documented
 *        divisors. Shared by rccBusPrescalerConfig() only, factored out
 *        so that function's validation stays flat rather than an 9-way
 *        chained condition inline.
 *
 * @param hpre Candidate CFGR.HPRE field value.
 * @return DRIVER_STATUS_OK if hpre is one of the 9 documented
 *         ::RCC_CFGR_HPRE_DIV1 .. ::RCC_CFGR_HPRE_DIV512 values.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM otherwise.
 */
static DriverStatus_e rccValidateHpre(uint32_t hpre)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((hpre != RCC_CFGR_HPRE_DIV1) && (hpre != RCC_CFGR_HPRE_DIV2) && (hpre != RCC_CFGR_HPRE_DIV4)
        && (hpre != RCC_CFGR_HPRE_DIV8) && (hpre != RCC_CFGR_HPRE_DIV16)
        && (hpre != RCC_CFGR_HPRE_DIV64) && (hpre != RCC_CFGR_HPRE_DIV128)
        && (hpre != RCC_CFGR_HPRE_DIV256) && (hpre != RCC_CFGR_HPRE_DIV512))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    return status;
}

/**
 * @brief Validate a CFGR.PPRE1/PPRE2 field value against its 5 documented
 *        divisors. Shared by rccBusPrescalerConfig() for both fields,
 *        which share one encoding (device/inc/rcc_reg.h).
 *
 * @param ppre Candidate CFGR.PPRE1 or CFGR.PPRE2 field value.
 * @return DRIVER_STATUS_OK if ppre is one of the 5 documented
 *         ::RCC_CFGR_PPRE_DIV1 .. ::RCC_CFGR_PPRE_DIV16 values.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM otherwise.
 */
static DriverStatus_e rccValidatePpre(uint32_t ppre)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((ppre != RCC_CFGR_PPRE_DIV1) && (ppre != RCC_CFGR_PPRE_DIV2) && (ppre != RCC_CFGR_PPRE_DIV4)
        && (ppre != RCC_CFGR_PPRE_DIV8) && (ppre != RCC_CFGR_PPRE_DIV16))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    return status;
}

DriverStatus_e rccBusPrescalerConfig(RccRegisters_t *rcc, uint32_t hpre, uint32_t ppre1,
                                     uint32_t ppre2)
{
    DriverStatus_e status = rccValidateHpre(hpre);

    if (status == DRIVER_STATUS_OK)
    {
        status = rccValidatePpre(ppre1);
    }

    if (status == DRIVER_STATUS_OK)
    {
        status = rccValidatePpre(ppre2);
    }

    if (status == DRIVER_STATUS_OK)
    {
        // cppcheck-suppress misra-c2012-12.2
        uint32_t mask = RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk;

        rcc->CFGR = (rcc->CFGR & ~mask) | (hpre << RCC_CFGR_HPRE_Pos)
                    | (ppre1 << RCC_CFGR_PPRE1_Pos) | (ppre2 << RCC_CFGR_PPRE2_Pos);
    }

    return status;
}

DriverStatus_e rccSysclkSwitch(RccRegisters_t *rcc, FlashRegisters_t *flash, PwrRegisters_t *pwr,
                               uint32_t source, uint32_t vos, uint32_t latency)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((source != RCC_CFGR_SYSCLK_HSI) && (source != RCC_CFGR_SYSCLK_HSE)
        && (source != RCC_CFGR_SYSCLK_PLL))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    if (status == DRIVER_STATUS_OK)
    {
        status = pwrSetVoltageScale(pwr, vos);
    }

    if (status == DRIVER_STATUS_OK)
    {
        status = flashSetLatency(flash, latency);
    }

    if (status == DRIVER_STATUS_OK)
    {
        uint32_t ready_bit;

        if (source == RCC_CFGR_SYSCLK_HSI)
        {
            ready_bit = RCC_CR_HSIRDY;
        }
        else if (source == RCC_CFGR_SYSCLK_HSE)
        {
            // cppcheck-suppress misra-c2012-12.2
            ready_bit = RCC_CR_HSERDY;
        }
        else
        {
            // cppcheck-suppress misra-c2012-12.2
            ready_bit = RCC_CR_PLLRDY;
        }

        if ((rcc->CR & ready_bit) == 0U)
        {
            status = DRIVER_STATUS_ERR_NOT_INITIALIZED;
        }
    }

    if (status == DRIVER_STATUS_OK)
    {
        uint32_t timeout = RCC_CLOCK_READY_TIMEOUT_ITERATIONS;

        rcc->CFGR = (rcc->CFGR & ~RCC_CFGR_SW_Msk) | (source << RCC_CFGR_SW_Pos);

        while ((rccActiveSysclkSource(rcc) != source) && (timeout > 0U))
        {
            timeout--;
        }

        if (timeout == 0U)
        {
            status = DRIVER_STATUS_ERR_TIMEOUT;
        }
    }

    return status;
}

/** @} */
