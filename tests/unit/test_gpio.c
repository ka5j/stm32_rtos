/**
 * @file test_gpio.c
 * @brief Host-side tests for drivers/src/gpio.c. Each test operates on a
 *        plain in-memory GpioRegisters_t standing in for a real peripheral
 *        - gpio.c's functions take the register block as a parameter
 *        rather than reaching for a hardware GPIOx macro, which is what
 *        makes this testable off-target. Compiled into
 *        tests/unit/test_runner.c's run_tests binary.
 */
#include "gpio.h"
#include "unity.h"

/* --- gpioInit: parameter validation --- */

/** mode outside MODER's 2-bit range must be rejected before any write. */
void test_gpio_driver_init_rejects_invalid_mode(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status = gpioInit(&port, GPIO_PIN_0, GPIO_MODE_ANALOG + 1U, GPIO_OTYPE_PP,
                                     GPIO_OSPEED_LOW, GPIO_PUPD_NONE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, port.MODER);
}

/** otype outside OTYPER's 1-bit range must be rejected before any write. */
void test_gpio_driver_init_rejects_invalid_otype(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status = gpioInit(&port, GPIO_PIN_0, GPIO_MODE_OUTPUT, GPIO_OTYPE_OD + 1U,
                                     GPIO_OSPEED_LOW, GPIO_PUPD_NONE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, port.OTYPER);
}

/** ospeed outside OSPEEDR's 2-bit range must be rejected before any write. */
void test_gpio_driver_init_rejects_invalid_ospeed(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status = gpioInit(&port, GPIO_PIN_0, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                                     GPIO_OSPEED_HIGH + 1U, GPIO_PUPD_NONE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, port.OSPEEDR);
}

/** pupd == 0x3 is reserved/undefined per RM0390 and must be rejected. */
void test_gpio_driver_init_rejects_reserved_pupd(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status = gpioInit(&port, GPIO_PIN_0, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                                     GPIO_OSPEED_LOW, GPIO_PUPD_DOWN + 1U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, port.PUPDR);
}

/* --- gpioInit: field placement and isolation --- */

/** A valid call sets MODER/OTYPER/OSPEEDR/PUPDR only in pin 5's field,
 *  leaving every other pin's bits in those same registers untouched. */
void test_gpio_driver_init_sets_fields_for_specified_pin_only(void)
{
    GpioRegisters_t port = {
        .MODER = 0xFFFFFFFFU, .OTYPER = 0xFFFFFFFFU, .OSPEEDR = 0xFFFFFFFFU, .PUPDR = 0xFFFFFFFFU};

    DriverStatus_e status = gpioInit(&port, GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                                     GPIO_OSPEED_LOW, GPIO_PUPD_NONE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);

    /* Pin 5's field now holds the requested value... */
    TEST_ASSERT_EQUAL_HEX32(GPIO_MODE_OUTPUT, (port.MODER >> 10) & 0x3U);
    TEST_ASSERT_EQUAL_HEX32(GPIO_OTYPE_PP, (port.OTYPER >> 5) & 0x1U);
    TEST_ASSERT_EQUAL_HEX32(GPIO_OSPEED_LOW, (port.OSPEEDR >> 10) & 0x3U);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PUPD_NONE, (port.PUPDR >> 10) & 0x3U);

    /* ...and every other pin's bits (still all-1s from setup) are intact. */
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.MODER | (0x3U << 10));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.OTYPER | (0x1U << 5));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.OSPEEDR | (0x3U << 10));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.PUPDR | (0x3U << 10));
}

/** gpioInit never touches AFRL/AFRH - AF selection is gpioSetAlternateFunction's
 *  job, called separately before gpioInit per its documented @pre. */
void test_gpio_driver_init_does_not_touch_afr(void)
{
    GpioRegisters_t port = {.AFRL = 0xDEADBEEFU, .AFRH = 0xCAFEF00DU};

    DriverStatus_e status =
        gpioInit(&port, GPIO_PIN_5, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, GPIO_PUPD_NONE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEFU, port.AFRL);
    TEST_ASSERT_EQUAL_HEX32(0xCAFEF00DU, port.AFRH);
}

/* --- gpioDeinit --- */

/** gpioDeinit clears MODER/OTYPER/OSPEEDR/PUPDR only in the specified
 *  pin's field, leaving every other pin's bits untouched. */
void test_gpio_driver_deinit_clears_fields_for_specified_pin_only(void)
{
    GpioRegisters_t port = {
        .MODER = 0xFFFFFFFFU, .OTYPER = 0xFFFFFFFFU, .OSPEEDR = 0xFFFFFFFFU, .PUPDR = 0xFFFFFFFFU};

    gpioDeinit(&port, GPIO_PIN_5);

    TEST_ASSERT_EQUAL_HEX32(0U, (port.MODER >> 10) & 0x3U);
    TEST_ASSERT_EQUAL_HEX32(0U, (port.OTYPER >> 5) & 0x1U);
    TEST_ASSERT_EQUAL_HEX32(0U, (port.OSPEEDR >> 10) & 0x3U);
    TEST_ASSERT_EQUAL_HEX32(0U, (port.PUPDR >> 10) & 0x3U);

    /* Every other pin's bits (still all-1s from setup) are intact. */
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~(0x3U << 10), port.MODER);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~(0x1U << 5), port.OTYPER);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~(0x3U << 10), port.OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~(0x3U << 10), port.PUPDR);
}

/** Pins 0-7 clear their nibble in AFRL; AFRH is untouched. */
void test_gpio_driver_deinit_clears_afrl_for_low_pins(void)
{
    GpioRegisters_t port = {.AFRL = 0xFFFFFFFFU, .AFRH = 0xFFFFFFFFU};

    gpioDeinit(&port, GPIO_PIN_3);

    TEST_ASSERT_EQUAL_HEX32(0U, (port.AFRL >> 12) & 0xFU);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.AFRH);
}

/** Pins 8-15 clear their nibble in AFRH; AFRL is untouched. */
void test_gpio_driver_deinit_clears_afrh_for_high_pins(void)
{
    GpioRegisters_t port = {.AFRL = 0xFFFFFFFFU, .AFRH = 0xFFFFFFFFU};

    gpioDeinit(&port, GPIO_PIN_12);

    TEST_ASSERT_EQUAL_HEX32(0U, (port.AFRH >> 16) & 0xFU);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.AFRL);
}

/* --- gpioSetAlternateFunction --- */

/** af outside AFRL/AFRH's 4-bit field (> GPIO_AF_MAX) must be rejected
 *  before any write. */
void test_gpio_driver_set_alternate_function_rejects_invalid_af(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status = gpioSetAlternateFunction(&port, GPIO_PIN_0, GPIO_AF_MAX + 1U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, port.AFRL);
}

/** Pins 0-7 write their nibble into AFRL without disturbing neighboring
 *  nibbles or AFRH. */
void test_gpio_driver_set_alternate_function_writes_afrl_for_low_pins(void)
{
    GpioRegisters_t port = {.AFRL = 0xFFFFFFFFU, .AFRH = 0xFFFFFFFFU};

    DriverStatus_e status = gpioSetAlternateFunction(&port, GPIO_PIN_3, 7U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(7U, (port.AFRL >> 12) & 0xFU);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.AFRL | (0xFU << 12));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.AFRH);
}

/** Pins 8-15 write their nibble into AFRH without disturbing neighboring
 *  nibbles or AFRL. */
void test_gpio_driver_set_alternate_function_writes_afrh_for_high_pins(void)
{
    GpioRegisters_t port = {.AFRL = 0xFFFFFFFFU, .AFRH = 0xFFFFFFFFU};

    DriverStatus_e status = gpioSetAlternateFunction(&port, GPIO_PIN_12, 9U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(9U, (port.AFRH >> 16) & 0xFU);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.AFRH | (0xFU << 16));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.AFRL);
}

/* --- gpioWritePin --- */

/** GPIO_PIN_SET drives the pin high via BSRR's set half (bits 0-15). */
void test_gpio_driver_write_pin_set_uses_bsrr_set_half(void)
{
    GpioRegisters_t port = {0};

    gpioWritePin(&port, GPIO_PIN_5, GPIO_PIN_SET);

    TEST_ASSERT_EQUAL_HEX32(0x1U << 5, port.BSRR);
}

/** GPIO_PIN_RESET drives the pin low via BSRR's reset half (bits 16-31). */
void test_gpio_driver_write_pin_reset_uses_bsrr_reset_half(void)
{
    GpioRegisters_t port = {0};

    gpioWritePin(&port, GPIO_PIN_5, GPIO_PIN_RESET);

    TEST_ASSERT_EQUAL_HEX32(0x1U << (5U + 16U), port.BSRR);
}

/* --- gpioReadPin --- */

/** A set IDR bit reports GPIO_PIN_SET. */
void test_gpio_driver_read_pin_reports_set_when_idr_bit_high(void)
{
    const GpioRegisters_t port = {.IDR = (0x1U << 5)};

    TEST_ASSERT_EQUAL(GPIO_PIN_SET, gpioReadPin(&port, GPIO_PIN_5));
}

/** IDR bits for other pins being set must not affect this pin's read. */
void test_gpio_driver_read_pin_reports_reset_when_idr_bit_low(void)
{
    const GpioRegisters_t port = {.IDR = 0xFFFFFFFFU & ~(0x1U << 5)};

    TEST_ASSERT_EQUAL(GPIO_PIN_RESET, gpioReadPin(&port, GPIO_PIN_5));
}

/* --- gpioTogglePin --- */

/** An ODR bit currently low toggles to driving the pin high (BSRR set half). */
void test_gpio_driver_toggle_pin_drives_high_when_odr_bit_low(void)
{
    GpioRegisters_t port = {.ODR = 0U};

    gpioTogglePin(&port, GPIO_PIN_5);

    TEST_ASSERT_EQUAL_HEX32(0x1U << 5, port.BSRR);
}

/** An ODR bit currently high toggles to driving the pin low (BSRR reset half). */
void test_gpio_driver_toggle_pin_drives_low_when_odr_bit_high(void)
{
    GpioRegisters_t port = {.ODR = (0x1U << 5)};

    gpioTogglePin(&port, GPIO_PIN_5);

    TEST_ASSERT_EQUAL_HEX32(0x1U << (5U + 16U), port.BSRR);
}
