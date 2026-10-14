/**
 * @file rcc.c
 * @brief RCC driver implementation (API in rcc.h).
 */
#include "rcc.h"

#include "flash.h"

/**
 * @addtogroup rcc_driver
 * @{
 */

/** Polls allowed for each oscillator, PLL or SWS wait (see
 *  [Poll-count timeouts](DESIGN.md#poll-count-timeouts)). */
#define RCC_CLOCK_READY_TIMEOUT_ITERATIONS (100000U)

/**
 * @brief Map a GPIO port to its AHB1ENR bit.
 * @param port Port to identify, compared by address.
 * @param bit  Receives the GPIOxEN bit; untouched on failure.
 * @retval DRIVER_STATUS_OK                @p port is GPIOA to GPIOH.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM Otherwise.
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
        (void)rcc->AHB1ENR; /* read back: the clock is running before first use */
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
    rcc->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)rcc->APB1ENR; /* read back: the clock is running before first use */
}

void rccUsart2ClockDisable(RccRegisters_t *rcc)
{
    rcc->APB1ENR &= ~RCC_APB1ENR_USART2EN;
}

void rccSyscfgClockEnable(RccRegisters_t *rcc)
{
    rcc->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)rcc->APB2ENR; /* read back: the clock is running before first use */
}

void rccSyscfgClockDisable(RccRegisters_t *rcc)
{
    rcc->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;
}

void rccPwrClockEnable(RccRegisters_t *rcc)
{
    rcc->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)rcc->APB1ENR; /* read back: the clock is running before first use */
}

void rccPwrClockDisable(RccRegisters_t *rcc)
{
    rcc->APB1ENR &= ~RCC_APB1ENR_PWREN;
}

/**
 * @brief The SYSCLK source in use, from CFGR.SWS (not the requested CFGR.SW).
 * @param rcc RCC block.
 * @return ::RCC_CFGR_SYSCLK_HSI, ::RCC_CFGR_SYSCLK_HSE or ::RCC_CFGR_SYSCLK_PLL.
 */
static uint32_t rccActiveSysclkSource(const RccRegisters_t *rcc)
{
    return (rcc->CFGR & RCC_CFGR_SWS_Msk) >> RCC_CFGR_SWS_Pos;
}

/**
 * @brief Report whether an oscillator feeds SYSCLK, directly or through the PLL.
 *
 * RM0390 ignores clearing HSION/HSEON in either case, so checking SWS alone
 * would report success for a write the hardware dropped.
 *
 * @param rcc          RCC block.
 * @param sysclk_value The oscillator's SW/SWS value (::RCC_CFGR_SYSCLK_HSI or _HSE).
 * @param pllsrc_value The oscillator's PLLSRC value (::RCC_PLLCFGR_PLLSRC_HSI or _HSE).
 * @retval DRIVER_STATUS_OK       Not feeding SYSCLK.
 * @retval DRIVER_STATUS_ERR_BUSY Feeding SYSCLK.
 */
static DriverStatus_e rccOscillatorDisableGuard(const RccRegisters_t *rcc, uint32_t sysclk_value,
                                                uint32_t pllsrc_value)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t active = rccActiveSysclkSource(rcc);
    uint32_t pll_input = rcc->PLLCFGR & RCC_PLLCFGR_PLLSRC;

    if (active == sysclk_value)
    {
        status = DRIVER_STATUS_ERR_BUSY;
    }
    else if ((active == RCC_CFGR_SYSCLK_PLL) && (pll_input == pllsrc_value))
    {
        status = DRIVER_STATUS_ERR_BUSY;
    }
    else
    {
        /* Not feeding SYSCLK - safe to disable. */
    }

    return status;
}

/**
 * @brief Poll a CR ready flag until it reaches @p expected or the budget runs out.
 *
 * The flag is re-read after the loop, so a flag that settles on the last
 * poll counts as success.
 *
 * @param rcc       RCC block.
 * @param ready_bit ::RCC_CR_HSIRDY, ::RCC_CR_HSERDY or ::RCC_CR_PLLRDY.
 * @param expected  @p ready_bit to wait for set, 0 to wait for clear.
 * @retval DRIVER_STATUS_OK          The flag is in the expected state.
 * @retval DRIVER_STATUS_ERR_TIMEOUT It is not, after ::RCC_CLOCK_READY_TIMEOUT_ITERATIONS polls.
 */
static DriverStatus_e rccWaitCrFlag(const RccRegisters_t *rcc, uint32_t ready_bit,
                                    uint32_t expected)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t timeout = RCC_CLOCK_READY_TIMEOUT_ITERATIONS;

    while (((rcc->CR & ready_bit) != expected) && (timeout > 0U))
    {
        timeout--;
    }

    if ((rcc->CR & ready_bit) != expected)
    {
        status = DRIVER_STATUS_ERR_TIMEOUT;
    }

    return status;
}

DriverStatus_e rccHsiEnable(RccRegisters_t *rcc)
{
    rcc->CR |= RCC_CR_HSION;

    return rccWaitCrFlag(rcc, RCC_CR_HSIRDY, RCC_CR_HSIRDY);
}

DriverStatus_e rccHsiDisable(RccRegisters_t *rcc)
{
    DriverStatus_e status =
        rccOscillatorDisableGuard(rcc, RCC_CFGR_SYSCLK_HSI, RCC_PLLCFGR_PLLSRC_HSI);

    if (status == DRIVER_STATUS_OK)
    {
        rcc->CR &= ~RCC_CR_HSION;
        status = rccWaitCrFlag(rcc, RCC_CR_HSIRDY, 0U);
    }

    return status;
}

DriverStatus_e rccHseEnable(RccRegisters_t *rcc, uint32_t bypass)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((bypass != 0U) && (bypass != RCC_CR_HSEBYP))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else if ((rcc->CR & RCC_CR_HSEON) != 0U)
    {
        status = DRIVER_STATUS_ERR_BUSY;
    }
    else
    {
        /* HSEBYP is writable only while HSEON is clear, which the guard ensures. */
        rcc->CR = (rcc->CR & ~RCC_CR_HSEBYP) | bypass;
        rcc->CR |= RCC_CR_HSEON;

        status = rccWaitCrFlag(rcc, RCC_CR_HSERDY, RCC_CR_HSERDY);
    }

    return status;
}

DriverStatus_e rccHseDisable(RccRegisters_t *rcc)
{
    uint32_t pllsrc_hse = RCC_PLLCFGR_PLLSRC_HSE;
    DriverStatus_e status = rccOscillatorDisableGuard(rcc, RCC_CFGR_SYSCLK_HSE, pllsrc_hse);

    if (status == DRIVER_STATUS_OK)
    {
        rcc->CR &= ~RCC_CR_HSEON;
        status = rccWaitCrFlag(rcc, RCC_CR_HSERDY, 0U);
    }

    return status;
}

DriverStatus_e rccPllConfig(RccRegisters_t *rcc, uint32_t source, uint32_t m, uint32_t n,
                            uint32_t p)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

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
    else if (p > (RCC_PLLCFGR_PLLP_Msk >> RCC_PLLCFGR_PLLP_Pos))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }
    else if ((rcc->CR & RCC_CR_PLLON) != 0U)
    {
        status = DRIVER_STATUS_ERR_BUSY;
    }
    else
    {
        uint32_t mask =
            RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk | RCC_PLLCFGR_PLLSRC;

        /* PLLQ and reserved bits are preserved. */
        rcc->PLLCFGR = (rcc->PLLCFGR & ~mask) | (m << RCC_PLLCFGR_PLLM_Pos)
                       | (n << RCC_PLLCFGR_PLLN_Pos) | (p << RCC_PLLCFGR_PLLP_Pos) | source;
    }

    return status;
}

DriverStatus_e rccPllEnable(RccRegisters_t *rcc)
{
    rcc->CR |= RCC_CR_PLLON;

    return rccWaitCrFlag(rcc, RCC_CR_PLLRDY, RCC_CR_PLLRDY);
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
        rcc->CR &= ~RCC_CR_PLLON;
        status = rccWaitCrFlag(rcc, RCC_CR_PLLRDY, 0U);
    }

    return status;
}

/**
 * @brief Check that @p hpre is one of the nine `RCC_CFGR_HPRE_DIV*` values.
 * @param hpre Candidate HPRE value.
 * @retval DRIVER_STATUS_OK                Valid.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM Not a defined divisor.
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
 * @brief Check that @p ppre is one of the five `RCC_CFGR_PPRE_DIV*` values.
 * @param ppre Candidate PPRE1 or PPRE2 value.
 * @retval DRIVER_STATUS_OK                Valid.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM Not a defined divisor.
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
        uint32_t mask = RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk;

        rcc->CFGR = (rcc->CFGR & ~mask) | (hpre << RCC_CFGR_HPRE_Pos)
                    | (ppre1 << RCC_CFGR_PPRE1_Pos) | (ppre2 << RCC_CFGR_PPRE2_Pos);
    }

    return status;
}

DriverStatus_e rccSysclkSwitch(RccRegisters_t *rcc, FlashRegisters_t *flash, uint32_t source,
                               uint32_t latency)
{
    DriverStatus_e status = DRIVER_STATUS_OK;
    uint32_t current_latency = (flash->ACR & FLASH_ACR_LATENCY_Msk) >> FLASH_ACR_LATENCY_Pos;

    if ((source != RCC_CFGR_SYSCLK_HSI) && (source != RCC_CFGR_SYSCLK_HSE)
        && (source != RCC_CFGR_SYSCLK_PLL))
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    /* A higher latency goes in before the switch. An out-of-range value is
     * always higher than the 4-bit current one, so flashSetLatency() rejects it
     * here and the lowering call below never sees it. */
    if ((status == DRIVER_STATUS_OK) && (latency > current_latency))
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
            ready_bit = RCC_CR_HSERDY;
        }
        else
        {
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

        /* Re-read SWS rather than test the counter (see rccWaitCrFlag()). */
        if (rccActiveSysclkSource(rcc) != source)
        {
            status = DRIVER_STATUS_ERR_TIMEOUT;
        }
    }

    /* A lower latency only once the switch has completed. */
    if ((status == DRIVER_STATUS_OK) && (latency <= current_latency))
    {
        status = flashSetLatency(flash, latency);
    }

    return status;
}

/** @} */
