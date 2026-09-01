/**
 * @file gpio.c
 * @brief GPIO peripheral driver implementation - see gpio.h for the public
 *        API and gpio_reg.h for the register definitions this operates on.
 */
#include "gpio.h"

/**
 * @addtogroup driver_layer
 * @{
 */

DriverStatus_e gpioInit(GpioRegisters_t *port, GpioPin_e pin, uint32_t mode, uint32_t otype,
                        uint32_t ospeed, uint32_t pupd)
{
  DriverStatus_e status = DRIVER_STATUS_OK;

  if ((mode > GPIO_MODE_ANALOG) || (otype > GPIO_OTYPE_OD) || (ospeed > GPIO_OSPEED_HIGH) ||
      (pupd > GPIO_PUPD_DOWN))
  {
    status = DRIVER_STATUS_ERR_INVALID_PARAM;
  }
  else
  {
    uint32_t pin_pos = (uint32_t)pin;
    uint32_t field_shift = pin_pos * 2U;

    port->OTYPER = (port->OTYPER & ~(0x1U << pin_pos)) | (otype << pin_pos);
    port->OSPEEDR = (port->OSPEEDR & ~(0x3U << field_shift)) | (ospeed << field_shift);
    port->PUPDR = (port->PUPDR & ~(0x3U << field_shift)) | (pupd << field_shift);

    /* MODER last - see gpioInit()'s @pre in gpio.h: AFR must already hold
     * the right function before this switches the pin into AF mode. */
    port->MODER = (port->MODER & ~(0x3U << field_shift)) | (mode << field_shift);
  }

  return status;
}

void gpioDeinit(GpioRegisters_t *port, GpioPin_e pin)
{
  uint32_t pin_pos = (uint32_t)pin;
  uint32_t field_shift = pin_pos * 2U;
  uint32_t afr_shift = (pin_pos & 0x7U) * 4U;

  port->MODER &= ~(0x3U << field_shift);
  port->OTYPER &= ~(0x1U << pin_pos);
  port->OSPEEDR &= ~(0x3U << field_shift);
  port->PUPDR &= ~(0x3U << field_shift);

  if (pin_pos < 8U)
  {
    /* afr_shift = (pin_pos & 0x7U) * 4U is bounded to [0,28] by the mask -
     * cppcheck's MISRA addon can't see that through this branch and flags
     * it based on 0xFU's narrow essential type instead. */
    // cppcheck-suppress misra-c2012-12.2
    port->AFRL &= ~(0xFU << afr_shift);
  }
  else
  {
    port->AFRH &= ~(0xFU << afr_shift);
  }
}

DriverStatus_e gpioSetAlternateFunction(GpioRegisters_t *port, GpioPin_e pin, uint32_t af)
{
  DriverStatus_e status = DRIVER_STATUS_OK;

  if (af > GPIO_AF_MAX)
  {
    status = DRIVER_STATUS_ERR_INVALID_PARAM;
  }
  else
  {
    uint32_t pin_pos = (uint32_t)pin;
    uint32_t afr_shift = (pin_pos & 0x7U) * 4U;

    if (pin_pos < 8U)
    {
      /* afr_shift = (pin_pos & 0x7U) * 4U is bounded to [0,28] by the mask -
       * cppcheck's MISRA addon can't see that through this branch and flags
       * it based on 0xFU/af's narrow essential type instead. */
      // cppcheck-suppress misra-c2012-12.2
      port->AFRL = (port->AFRL & ~(0xFU << afr_shift)) | (af << afr_shift);
    }
    else
    {
      port->AFRH = (port->AFRH & ~(0xFU << afr_shift)) | (af << afr_shift);
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
