/**
 * @file gpio.h
 * @brief GPIO peripheral driver - register-level configuration and digital
 *        I/O for the STM32F446xx GPIO peripheral (device/inc/gpio_reg.h).
 *        No application-facing logic; consumed by api/.
 */
#ifndef GPIO_H
#define GPIO_H

#include "driver_status.h"
#include "gpio_reg.h"

/**
 * @addtogroup driver_layer
 * @{
 */

#define GPIO_AF_MAX (15U) ///< Highest valid AFRL/AFRH alternate-function selector

/**
 * @brief GPIO pin numbers (0-15) within a GPIO port.
 */
typedef enum GpioPin_e
{
  GPIO_PIN_0 = 0,
  GPIO_PIN_1 = 1,
  GPIO_PIN_2 = 2,
  GPIO_PIN_3 = 3,
  GPIO_PIN_4 = 4,
  GPIO_PIN_5 = 5,
  GPIO_PIN_6 = 6,
  GPIO_PIN_7 = 7,
  GPIO_PIN_8 = 8,
  GPIO_PIN_9 = 9,
  GPIO_PIN_10 = 10,
  GPIO_PIN_11 = 11,
  GPIO_PIN_12 = 12,
  GPIO_PIN_13 = 13,
  GPIO_PIN_14 = 14,
  GPIO_PIN_15 = 15,
} GpioPin_e;

/**
 * @brief Digital state of a GPIO pin - used both to write a pin (gpioWritePin)
 *        and to report what gpioReadPin found.
 */
typedef enum GpioPinState_e
{
  GPIO_PIN_RESET = 0,
  GPIO_PIN_SET = 1,
} GpioPinState_e;

/**
 * @brief Configure a GPIO pin's mode, output type, speed, and pull
 *        resistor in one call.
 *
 * AFRL/AFRH is not touched here - if @p mode is ::GPIO_MODE_AF, call
 * gpioSetAlternateFunction() for this pin *before* calling gpioInit(),
 * so the alternate function is already selected before MODER switches
 * the pin into AF mode.
 *
 * @param port   GPIO port register block (e.g. GPIOA).
 * @param pin    Pin number within the port.
 * @param mode   One of the GPIO_MODE_* values (gpio_reg.h).
 * @param otype  One of the GPIO_OTYPE_* values (gpio_reg.h).
 * @param ospeed One of the GPIO_OSPEED_* values (gpio_reg.h).
 * @param pupd   One of the GPIO_PUPD_* values (gpio_reg.h).
 * @pre The port's RCC_AHB1ENR_GPIOxEN bit is already enabled.
 * @return DRIVER_STATUS_OK on success.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if mode, otype, ospeed, or pupd
 *         is not one of its documented values (e.g. pupd == 0x3, which is
 *         reserved/undefined per RM0390).
 */
DRIVER_MUST_CHECK DriverStatus_e gpioInit(GpioRegisters_t *port, GpioPin_e pin, uint32_t mode,
                                          uint32_t otype, uint32_t ospeed, uint32_t pupd);

/**
 * @brief Reset a GPIO pin's MODER/OTYPER/OSPEEDR/PUPDR/AFR fields back to
 *        their power-on-reset values (input mode, push-pull, low speed,
 *        no pull, alternate function 0).
 *
 * @param port GPIO port register block (e.g. GPIOA).
 * @param pin  Pin number within the port.
 */
void gpioDeinit(GpioRegisters_t *port, GpioPin_e pin);

/**
 * @brief Select which alternate function a pin routes to via AFRL/AFRH.
 *        Has no electrical effect until the pin's MODER field is also
 *        set to ::GPIO_MODE_AF (see gpioInit()).
 *
 * @param port GPIO port register block (e.g. GPIOA).
 * @param pin  Pin number within the port.
 * @param af   Alternate function number, 0-GPIO_AF_MAX.
 * @return DRIVER_STATUS_OK on success.
 * @return DRIVER_STATUS_ERR_INVALID_PARAM if af > GPIO_AF_MAX.
 */
DRIVER_MUST_CHECK DriverStatus_e gpioSetAlternateFunction(GpioRegisters_t *port, GpioPin_e pin,
                                                          uint32_t af);

/**
 * @brief Drive a pin high or low via a single atomic BSRR write.
 *
 * @param port  GPIO port register block (e.g. GPIOA).
 * @param pin   Pin number within the port.
 * @param level GPIO_PIN_SET to drive high, GPIO_PIN_RESET to drive low.
 */
void gpioWritePin(GpioRegisters_t *port, GpioPin_e pin, GpioPinState_e level);

/**
 * @brief Read a pin's current electrical state from IDR.
 *
 * @param port GPIO port register block (e.g. GPIOA).
 * @param pin  Pin number within the port.
 * @return GPIO_PIN_SET if the pin reads high, GPIO_PIN_RESET otherwise.
 */
GpioPinState_e gpioReadPin(const GpioRegisters_t *port, GpioPin_e pin);

/**
 * @brief Toggle a pin's driven output state.
 *
 * Reads the pin's current state from ODR (what this port is driving,
 * not IDR's electrical readback) and writes the complement via BSRR.
 * Not atomic against another context toggling the same pin between the
 * read and the write; safe as long as only one context ever drives a
 * given pin.
 *
 * @param port GPIO port register block (e.g. GPIOA).
 * @param pin  Pin number within the port.
 */
void gpioTogglePin(GpioRegisters_t *port, GpioPin_e pin);

/** @} */

#endif /* GPIO_H */
