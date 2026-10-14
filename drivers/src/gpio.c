/**
 * @file gpio.c
 * @brief GPIO driver implementation (API in gpio.h).
 */
#include "gpio.h"

/**
 * @addtogroup gpio_driver
 * @{
 */

/**
 * @brief Reject a pin outside 0-15, which would shift by 32 or more.
 * @param pin Candidate pin.
 * @retval DRIVER_STATUS_OK                @p pin is 0-15.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM Otherwise.
 */
static DriverStatus_e gpioValidatePin(GpioPin_e pin)
{
    DriverStatus_e status = DRIVER_STATUS_OK;

    if ((uint32_t)pin > (uint32_t)GPIO_PIN_15)
    {
        status = DRIVER_STATUS_ERR_INVALID_PARAM;
    }

    return status;
}

DriverStatus_e gpioInit(GpioRegisters_t *port, GpioPin_e pin, uint32_t mode, uint32_t otype,
                        uint32_t ospeed, uint32_t pupd)
{
    DriverStatus_e status = gpioValidatePin(pin);

    if (status == DRIVER_STATUS_OK)
    {
        if ((mode > GPIO_MODE_ANALOG) || (otype > GPIO_OTYPE_OD) || (ospeed > GPIO_OSPEED_HIGH)
            || (pupd > GPIO_PUPD_DOWN))
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    if (status == DRIVER_STATUS_OK)
    {
        uint32_t pin_pos = (uint32_t)pin;
        uint32_t field_shift = pin_pos * 2U;

        port->OTYPER = (port->OTYPER & ~(0x1U << pin_pos)) | (otype << pin_pos);
        port->OSPEEDR = (port->OSPEEDR & ~(0x3U << field_shift)) | (ospeed << field_shift);
        port->PUPDR = (port->PUPDR & ~(0x3U << field_shift)) | (pupd << field_shift);

        /* MODER last, so an AF pin never switches mode before its AFR is set. */
        port->MODER = (port->MODER & ~(0x3U << field_shift)) | (mode << field_shift);
    }

    return status;
}

GpioResetValues_t gpioPortResetValues(const GpioRegisters_t *port)
{
    GpioResetValues_t values = {0U, 0U, 0U};

    if (port == GPIOA)
    {
        values.MODER = GPIOA_MODER_RESET;
        values.PUPDR = GPIOA_PUPDR_RESET;
    }
    else if (port == GPIOB)
    {
        values.MODER = GPIOB_MODER_RESET;
        values.OSPEEDR = GPIOB_OSPEEDR_RESET;
        values.PUPDR = GPIOB_PUPDR_RESET;
    }
    else
    {
        /* Every other port resets to all zeros - already set above. */
    }

    return values;
}

void gpioDeinit(GpioRegisters_t *port, GpioPin_e pin)
{
    gpioDeinitToReset(port, pin, gpioPortResetValues(port));
}

void gpioDeinitToReset(GpioRegisters_t *port, GpioPin_e pin, GpioResetValues_t reset)
{
    uint32_t pin_pos = (uint32_t)pin;
    uint32_t field_shift = pin_pos * 2U;
    uint32_t afr_shift = (pin_pos & 0x7U) * 4U;
    uint32_t pin_field = 0x3U << field_shift;

    port->MODER = (port->MODER & ~pin_field) | (reset.MODER & pin_field);
    port->OTYPER &= ~(0x1U << pin_pos);
    port->OSPEEDR = (port->OSPEEDR & ~pin_field) | (reset.OSPEEDR & pin_field);
    port->PUPDR = (port->PUPDR & ~pin_field) | (reset.PUPDR & pin_field);

    if (pin_pos < 8U)
    {
        port->AFRL &= ~((uint32_t)0xFU << afr_shift);
    }
    else
    {
        port->AFRH &= ~((uint32_t)0xFU << afr_shift);
    }
}

DriverStatus_e gpioSetAlternateFunction(GpioRegisters_t *port, GpioPin_e pin, uint32_t af)
{
    DriverStatus_e status = gpioValidatePin(pin);

    if (status == DRIVER_STATUS_OK)
    {
        if (af > GPIO_AF_MAX)
        {
            status = DRIVER_STATUS_ERR_INVALID_PARAM;
        }
    }

    if (status == DRIVER_STATUS_OK)
    {
        uint32_t pin_pos = (uint32_t)pin;
        uint32_t afr_shift = (pin_pos & 0x7U) * 4U;

        if (pin_pos < 8U)
        {
            port->AFRL = (port->AFRL & ~((uint32_t)0xFU << afr_shift)) | (af << afr_shift);
        }
        else
        {
            port->AFRH = (port->AFRH & ~((uint32_t)0xFU << afr_shift)) | (af << afr_shift);
        }
    }

    return status;
}

void gpioWritePin(GpioRegisters_t *port, GpioPin_e pin, GpioPinState_e level)
{
    uint32_t pin_pos = (uint32_t)pin;

    if (level == GPIO_PIN_SET)
    {
        port->BSRR = (0x1U << pin_pos);
    }
    else
    {
        port->BSRR = (0x1U << (pin_pos + 16U));
    }
}

GpioPinState_e gpioReadPin(const GpioRegisters_t *port, GpioPin_e pin)
{
    uint32_t pin_pos = (uint32_t)pin;
    GpioPinState_e state = GPIO_PIN_RESET;

    if ((port->IDR & (0x1U << pin_pos)) != 0U)
    {
        state = GPIO_PIN_SET;
    }

    return state;
}

void gpioTogglePin(GpioRegisters_t *port, GpioPin_e pin)
{
    uint32_t pin_pos = (uint32_t)pin;

    if ((port->ODR & (0x1U << pin_pos)) != 0U)
    {
        port->BSRR = (0x1U << (pin_pos + 16U));
    }
    else
    {
        port->BSRR = (0x1U << pin_pos);
    }
}

/** @} */
