/**
 * @file gpio.h
 * @brief GPIO driver: pin configuration and digital I/O.
 *
 * Typical use, an output on PA5 (the NUCLEO's LD2):
 * @code
 * if (rccGpioClockEnable(RCC, GPIOA) != DRIVER_STATUS_OK) { ... }
 * if (gpioInit(GPIOA, GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW,
 *              GPIO_PUPD_NONE) != DRIVER_STATUS_OK) { ... }
 * gpioWritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
 * @endcode
 *
 * For an alternate function, call gpioSetAlternateFunction() before
 * gpioInit() so the function is selected before MODER switches the pin.
 */
#ifndef GPIO_H
#define GPIO_H

#include "driver_status.h"
#include "gpio_reg.h"

#include <stdint.h>

/**
 * @addtogroup gpio_driver
 * @{
 */

#define GPIO_AF_MAX (15U) ///< Highest alternate-function number (AF0-AF15)

/**
 * @brief Pin number within a port.
 *
 * C does not stop a caller casting any integer in. Functions that return a
 * ::DriverStatus_e reject a pin above 15; the others state it as an `@pre`,
 * because a larger value shifts by 32 or more, which is undefined behaviour.
 */
typedef enum GpioPin_e
{
    GPIO_PIN_0 = 0,   ///< Pin 0
    GPIO_PIN_1 = 1,   ///< Pin 1
    GPIO_PIN_2 = 2,   ///< Pin 2
    GPIO_PIN_3 = 3,   ///< Pin 3
    GPIO_PIN_4 = 4,   ///< Pin 4
    GPIO_PIN_5 = 5,   ///< Pin 5
    GPIO_PIN_6 = 6,   ///< Pin 6
    GPIO_PIN_7 = 7,   ///< Pin 7
    GPIO_PIN_8 = 8,   ///< Pin 8
    GPIO_PIN_9 = 9,   ///< Pin 9
    GPIO_PIN_10 = 10, ///< Pin 10
    GPIO_PIN_11 = 11, ///< Pin 11
    GPIO_PIN_12 = 12, ///< Pin 12
    GPIO_PIN_13 = 13, ///< Pin 13
    GPIO_PIN_14 = 14, ///< Pin 14
    GPIO_PIN_15 = 15, ///< Pin 15
} GpioPin_e;

/** @brief Logic level of a pin. */
typedef enum GpioPinState_e
{
    GPIO_PIN_RESET = 0, ///< Low
    GPIO_PIN_SET = 1,   ///< High
} GpioPinState_e;

/** @brief Power-on values of the GPIO registers that are not zero on every port. */
typedef struct GpioResetValues_t
{
    uint32_t MODER;   ///< MODER reset value
    uint32_t OSPEEDR; ///< OSPEEDR reset value
    uint32_t PUPDR;   ///< PUPDR reset value
} GpioResetValues_t;

/**
 * @brief Look up a port's power-on MODER, OSPEEDR and PUPDR values.
 *
 * Only GPIOA and GPIOB are non-zero: they hold the SWD/JTAG pins. Compares
 * @p port by address and never dereferences it. Public so the host tests can
 * check the lookup.
 *
 * @param port GPIO port (e.g. GPIOA); any other address gives all zeros.
 * @return The port's reset values.
 */
GpioResetValues_t gpioPortResetValues(const GpioRegisters_t *port);

/**
 * @brief Configure a pin's mode, output type, speed and pull.
 *
 * @param port   GPIO port (e.g. GPIOA).
 * @param pin    Pin within the port.
 * @param mode   A `GPIO_MODE_*` value.
 * @param otype  A `GPIO_OTYPE_*` value.
 * @param ospeed A `GPIO_OSPEED_*` value.
 * @param pupd   A `GPIO_PUPD_*` value.
 * @pre The port's clock is enabled (rccGpioClockEnable()).
 * @pre For ::GPIO_MODE_AF, gpioSetAlternateFunction() has already run for this pin.
 * @retval DRIVER_STATUS_OK                Configured; MODER is written last.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p pin is above 15, or a value is not one of its
 *                                         macros (for example the reserved PUPDR 0b11).
 */
DRIVER_MUST_CHECK DriverStatus_e gpioInit(GpioRegisters_t *port, GpioPin_e pin, uint32_t mode,
                                          uint32_t otype, uint32_t ospeed, uint32_t pupd);

/**
 * @brief Return a pin to its power-on configuration.
 *
 * Restores MODER, OTYPER, OSPEEDR, PUPDR and the AFR field. The SWD/JTAG
 * pins return to their debug configuration, so deinitialising one does not
 * disconnect the debugger.
 *
 * @param port GPIO port (e.g. GPIOA).
 * @param pin  Pin within the port.
 * @pre @p pin is 0-15; there is no status to report a bad one.
 */
void gpioDeinit(GpioRegisters_t *port, GpioPin_e pin);

/**
 * @brief gpioDeinit() with the reset values supplied by the caller.
 *
 * Exists for the host tests: gpioDeinit() looks the values up by comparing
 * @p port with GPIOA and GPIOB, which a test's in-memory struct never
 * matches. Firmware calls gpioDeinit().
 *
 * @param port  GPIO port.
 * @param pin   Pin within the port.
 * @param reset The port's reset values, as from gpioPortResetValues().
 * @pre @p pin is 0-15.
 */
void gpioDeinitToReset(GpioRegisters_t *port, GpioPin_e pin, GpioResetValues_t reset);

/**
 * @brief Select a pin's alternate function in AFRL/AFRH.
 *
 * Has no effect on the pin until its mode is ::GPIO_MODE_AF.
 *
 * @param port GPIO port (e.g. GPIOA).
 * @param pin  Pin within the port.
 * @param af   Alternate function, 0 to ::GPIO_AF_MAX.
 * @retval DRIVER_STATUS_OK                Selected.
 * @retval DRIVER_STATUS_ERR_INVALID_PARAM @p pin is above 15 or @p af above ::GPIO_AF_MAX.
 */
DRIVER_MUST_CHECK DriverStatus_e gpioSetAlternateFunction(GpioRegisters_t *port, GpioPin_e pin,
                                                          uint32_t af);

/**
 * @brief Drive a pin high or low with one atomic BSRR write.
 * @param port  GPIO port (e.g. GPIOA).
 * @param pin   Pin within the port.
 * @param level ::GPIO_PIN_SET or ::GPIO_PIN_RESET.
 * @pre @p pin is 0-15.
 */
void gpioWritePin(GpioRegisters_t *port, GpioPin_e pin, GpioPinState_e level);

/**
 * @brief Read a pin's input level from IDR.
 * @param port GPIO port (e.g. GPIOA).
 * @param pin  Pin within the port.
 * @return ::GPIO_PIN_SET if the pin is high, ::GPIO_PIN_RESET otherwise.
 * @pre @p pin is 0-15.
 */
GpioPinState_e gpioReadPin(const GpioRegisters_t *port, GpioPin_e pin);

/**
 * @brief Invert a pin's output level.
 *
 * Reads ODR, then writes BSRR.
 *
 * @param port GPIO port (e.g. GPIOA).
 * @param pin  Pin within the port.
 * @pre @p pin is 0-15.
 * @warning Not atomic: drive each pin from only one context. See
 *          [Concurrency](DESIGN.md#concurrency).
 */
void gpioTogglePin(GpioRegisters_t *port, GpioPin_e pin);

/** @} */

#endif /* GPIO_H */
