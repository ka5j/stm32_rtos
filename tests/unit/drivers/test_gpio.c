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

#include <stddef.h>

/* --- gpioInit: parameter validation --- */

/** A pin cast in from outside GpioPin_e's 0-15 domain must be rejected
 *  before any register is touched. Every field this driver writes is
 *  indexed by the pin number, so pin 16 would shift a uint32_t by 32 -
 *  undefined behaviour, not merely a wrong write. The enum type alone
 *  does not prevent this; C lets any integer be cast to it. */
void test_gpio_driver_init_rejects_out_of_range_pin(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status = gpioInit(&port, (GpioPin_e)16, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                                     GPIO_OSPEED_LOW, GPIO_PUPD_NONE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, port.MODER);
    TEST_ASSERT_EQUAL_HEX32(0U, port.OTYPER);
    TEST_ASSERT_EQUAL_HEX32(0U, port.OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32(0U, port.PUPDR);
}

/** A negative pin cast in wraps to a large unsigned value and must be
 *  rejected by the same check - the validation casts to uint32_t rather
 *  than comparing as a signed enum for exactly this case. */
void test_gpio_driver_init_rejects_negative_pin(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status = gpioInit(&port, (GpioPin_e)-1, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                                     GPIO_OSPEED_LOW, GPIO_PUPD_NONE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, port.MODER);
}

/** The highest valid pin (15) must still be accepted - the boundary the
 *  rejection tests above sit just past. */
void test_gpio_driver_init_accepts_highest_valid_pin(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status = gpioInit(&port, GPIO_PIN_15, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                                     GPIO_OSPEED_LOW, GPIO_PUPD_NONE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(GPIO_MODE_OUTPUT << 30U, port.MODER);
}

/** gpioSetAlternateFunction validates the pin too, before touching
 *  AFRL/AFRH. */
void test_gpio_driver_set_alternate_function_rejects_out_of_range_pin(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status = gpioSetAlternateFunction(&port, (GpioPin_e)16, 7U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, port.AFRL);
    TEST_ASSERT_EQUAL_HEX32(0U, port.AFRH);
}

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

/* --- gpioPortResetValues --- */

/** GPIOA holds PA13/PA14/PA15 (SWDIO/SWCLK/JTDI), which reset to AF mode
 *  with pulls, and nothing else is non-default (OSPEEDR is all zero). Decode the table per pin
 * rather than trusting the hex, so a transposed digit fails here. */
void test_gpio_driver_port_reset_values_gpioa_swd_pins(void)
{
    GpioResetValues_t reset = gpioPortResetValues(GPIOA);

    for (uint32_t pin = 0U; pin < 16U; pin++)
    {
        uint32_t shift = pin * 2U;
        uint32_t af = ((pin >= 13U) ? GPIO_MODE_AF : GPIO_MODE_INPUT);
        uint32_t pull = GPIO_PUPD_NONE;

        if ((pin == 13U) || (pin == 15U))
        {
            pull = GPIO_PUPD_UP;
        }
        else if (pin == 14U)
        {
            pull = GPIO_PUPD_DOWN;
        }
        else
        {
            /* no pull */
        }

        TEST_ASSERT_EQUAL_HEX32(af, (reset.MODER >> shift) & 0x3U);
        TEST_ASSERT_EQUAL_HEX32(pull, (reset.PUPDR >> shift) & 0x3U);
    }

    /* GPIOA's OSPEEDR resets to zero on the F446, unlike the F401/F411
     * (0x0C000000, PA13 very high speed). Pinned explicitly so a value
     * copied from another family's manual fails here. */
    TEST_ASSERT_EQUAL_HEX32(0U, reset.OSPEEDR);
}

/** GPIOB holds PB3/PB4 (SWO/NJTRST): AF mode, PB3 very high speed, PB4
 *  pull-up. */
void test_gpio_driver_port_reset_values_gpiob_debug_pins(void)
{
    GpioResetValues_t reset = gpioPortResetValues(GPIOB);

    for (uint32_t pin = 0U; pin < 16U; pin++)
    {
        uint32_t shift = pin * 2U;
        uint32_t af = ((pin == 3U) || (pin == 4U)) ? GPIO_MODE_AF : GPIO_MODE_INPUT;
        uint32_t speed = (pin == 3U) ? GPIO_OSPEED_HIGH : GPIO_OSPEED_LOW;
        uint32_t pull = (pin == 4U) ? GPIO_PUPD_UP : GPIO_PUPD_NONE;

        TEST_ASSERT_EQUAL_HEX32(af, (reset.MODER >> shift) & 0x3U);
        TEST_ASSERT_EQUAL_HEX32(speed, (reset.OSPEEDR >> shift) & 0x3U);
        TEST_ASSERT_EQUAL_HEX32(pull, (reset.PUPDR >> shift) & 0x3U);
    }
}

/** GPIOC..GPIOH, and an address that is no GPIO port at all, reset to
 *  all zeros. */
void test_gpio_driver_port_reset_values_other_ports_are_zero(void)
{
    const GpioRegisters_t *others[] = {GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH};
    GpioRegisters_t not_a_port = {0};

    for (size_t i = 0U; i < (sizeof(others) / sizeof(others[0])); i++)
    {
        GpioResetValues_t reset = gpioPortResetValues(others[i]);

        TEST_ASSERT_EQUAL_HEX32(0U, reset.MODER);
        TEST_ASSERT_EQUAL_HEX32(0U, reset.OSPEEDR);
        TEST_ASSERT_EQUAL_HEX32(0U, reset.PUPDR);
    }

    TEST_ASSERT_EQUAL_HEX32(0U, gpioPortResetValues(&not_a_port).MODER);
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

/* --- gpioDeinitToReset: the reset-value restore gpioDeinit() cannot show on the host --- */

/** Deinitialising PA13 (SWDIO) with GPIOA's reset values puts it back into
 *  alternate-function mode with the pull-up the debug port needs, rather
 *  than clearing it to input/no-pull - the restore that keeps the debugger
 *  connected. Only PA13's own fields change; AFR and OTYPER are cleared as
 *  for any pin. */
void test_gpio_driver_deinit_to_reset_restores_swdio_on_gpioa(void)
{
    GpioRegisters_t port = {.MODER = 0xFFFFFFFFU,
                            .OTYPER = 0xFFFFFFFFU,
                            .OSPEEDR = 0xFFFFFFFFU,
                            .PUPDR = 0xFFFFFFFFU,
                            .AFRL = 0xFFFFFFFFU,
                            .AFRH = 0xFFFFFFFFU};

    gpioDeinitToReset(&port, GPIO_PIN_13, gpioPortResetValues(GPIOA));

    TEST_ASSERT_EQUAL_HEX32(GPIO_MODE_AF, (port.MODER >> 26) & 0x3U);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PUPD_UP, (port.PUPDR >> 26) & 0x3U);
    TEST_ASSERT_EQUAL_HEX32(0U, (port.OSPEEDR >> 26) & 0x3U);
    TEST_ASSERT_EQUAL_HEX32(0U, (port.OTYPER >> 13) & 0x1U);
    TEST_ASSERT_EQUAL_HEX32(0U, (port.AFRH >> 20) & 0xFU);

    /* Every other pin's bits (still all-1s from setup) are intact. */
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~(0x3U << 26), port.OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32((0xFFFFFFFFU & ~(0x3U << 26)) | (GPIO_MODE_AF << 26), port.MODER);
    TEST_ASSERT_EQUAL_HEX32((0xFFFFFFFFU & ~(0x3U << 26)) | (GPIO_PUPD_UP << 26), port.PUPDR);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~(0xFU << 20), port.AFRH);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, port.AFRL);
}

/** PB3 (SWO) resets to alternate-function mode at very high speed with no
 *  pull - the one SWD-related pin whose OSPEEDR is not zero. */
void test_gpio_driver_deinit_to_reset_restores_pb3_speed_and_mode(void)
{
    GpioRegisters_t port = {.MODER = 0xFFFFFFFFU, .OSPEEDR = 0xFFFFFFFFU, .PUPDR = 0xFFFFFFFFU};

    gpioDeinitToReset(&port, GPIO_PIN_3, gpioPortResetValues(GPIOB));

    TEST_ASSERT_EQUAL_HEX32((0xFFFFFFFFU & ~(0x3U << 6)) | (GPIO_MODE_AF << 6), port.MODER);
    TEST_ASSERT_EQUAL_HEX32((0xFFFFFFFFU & ~(0x3U << 6)) | (GPIO_OSPEED_HIGH << 6), port.OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~(0x3U << 6), port.PUPDR);
}

/** Each register takes its own reset value: MODER, OSPEEDR and PUPDR are
 *  given three different values for the pin, so a restore that pulled a
 *  field from the wrong reset register would show. */
void test_gpio_driver_deinit_to_reset_takes_each_field_from_its_own_register(void)
{
    GpioResetValues_t reset = {.MODER = 0x1U << 10, .OSPEEDR = 0x2U << 10, .PUPDR = 0x3U << 10};
    GpioRegisters_t port = {.MODER = 0xFFFFFFFFU, .OSPEEDR = 0xFFFFFFFFU, .PUPDR = 0xFFFFFFFFU};

    gpioDeinitToReset(&port, GPIO_PIN_5, reset);

    TEST_ASSERT_EQUAL_HEX32((0xFFFFFFFFU & ~(0x3U << 10)) | (0x1U << 10), port.MODER);
    TEST_ASSERT_EQUAL_HEX32((0xFFFFFFFFU & ~(0x3U << 10)) | (0x2U << 10), port.OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32((0xFFFFFFFFU & ~(0x3U << 10)) | (0x3U << 10), port.PUPDR);
}

/** Reset values that belong to other pins are not applied to this one:
 *  restoring pin 5 with a reset word that is non-zero only at pin 13
 *  must leave pin 5 at zero. */
void test_gpio_driver_deinit_to_reset_ignores_reset_bits_of_other_pins(void)
{
    GpioResetValues_t reset = {.MODER = 0x2U << 26, .OSPEEDR = 0x3U << 26, .PUPDR = 0x1U << 26};
    GpioRegisters_t port = {.MODER = 0xFFFFFFFFU, .OSPEEDR = 0xFFFFFFFFU, .PUPDR = 0xFFFFFFFFU};

    gpioDeinitToReset(&port, GPIO_PIN_5, reset);

    TEST_ASSERT_EQUAL_HEX32(0U, (port.MODER >> 10) & 0x3U);
    TEST_ASSERT_EQUAL_HEX32(0U, (port.OSPEEDR >> 10) & 0x3U);
    TEST_ASSERT_EQUAL_HEX32(0U, (port.PUPDR >> 10) & 0x3U);
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

/** Non-zero field values land in the right register at the right pin position. Every field of
 *  every register is distinct from its neighbours' encoding, so a swapped shift (e.g. OSPEEDR
 *  shifted by `pin` instead of `pin * 2`) or a mixed-up register is visible. */
void test_gpio_driver_init_places_nonzero_fields_in_the_right_registers(void)
{
    GpioRegisters_t port = {0};

    DriverStatus_e status =
        gpioInit(&port, GPIO_PIN_5, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH, GPIO_PUPD_DOWN);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(GPIO_MODE_AF << 10, port.MODER);
    TEST_ASSERT_EQUAL_HEX32(GPIO_OTYPE_OD << 5, port.OTYPER);
    TEST_ASSERT_EQUAL_HEX32(GPIO_OSPEED_HIGH << 10, port.OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PUPD_DOWN << 10, port.PUPDR);
}

/** Re-initialising a pin overwrites its old non-zero fields rather than OR-ing into them. */
void test_gpio_driver_init_overwrites_previous_nonzero_fields(void)
{
    GpioRegisters_t port = {
        .MODER = 3U << 10, .OTYPER = 1U << 5, .OSPEEDR = 3U << 10, .PUPDR = 3U << 10};

    DriverStatus_e status = gpioInit(&port, GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                                     GPIO_OSPEED_MEDIUM, GPIO_PUPD_UP);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(GPIO_MODE_OUTPUT << 10, port.MODER);
    TEST_ASSERT_EQUAL_HEX32(0U, port.OTYPER);
    TEST_ASSERT_EQUAL_HEX32(GPIO_OSPEED_MEDIUM << 10, port.OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PUPD_UP << 10, port.PUPDR);
}

/* --- review follow-ups: argument mix-ups, boundaries and untouched state --- */

/** Every pin places each field in the right register at the right shift: distinct non-zero values
 *  everywhere (mode 3, otype 1, ospeed 2, pupd 1 - no two registers share a field value except
 *  otype/pupd, which differ by shift on every pin but 0), so a swapped register or a wrong shift
 *  fails on some pin. Also accepts GPIO_MODE_ANALOG, the highest mode. */
void test_gpio_driver_init_places_every_pin_correctly(void)
{
    for (uint32_t pin = 0U; pin <= 15U; pin++)
    {
        GpioRegisters_t port = {0};

        DriverStatus_e status = gpioInit(&port, (GpioPin_e)pin, GPIO_MODE_ANALOG, GPIO_OTYPE_OD,
                                         GPIO_OSPEED_FAST, GPIO_PUPD_UP);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(GPIO_MODE_ANALOG << (2U * pin), port.MODER);
        TEST_ASSERT_EQUAL_HEX32(GPIO_OTYPE_OD << pin, port.OTYPER);
        TEST_ASSERT_EQUAL_HEX32(GPIO_OSPEED_FAST << (2U * pin), port.OSPEEDR);
        TEST_ASSERT_EQUAL_HEX32(GPIO_PUPD_UP << (2U * pin), port.PUPDR);
    }
}

/** A rejected gpioInit leaves all four configuration registers exactly as they were. */
void test_gpio_driver_init_rejection_leaves_every_register_untouched(void)
{
    static const struct
    {
        GpioPin_e pin;
        uint32_t mode;
        uint32_t otype;
        uint32_t ospeed;
        uint32_t pupd;
    } bad[] = {
        {(GpioPin_e)16, GPIO_MODE_OUTPUT, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH, GPIO_PUPD_UP},
        {GPIO_PIN_5, GPIO_MODE_ANALOG + 1U, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH, GPIO_PUPD_UP},
        {GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPE_OD + 1U, GPIO_OSPEED_HIGH, GPIO_PUPD_UP},
        {GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH + 1U, GPIO_PUPD_UP},
        {GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH, GPIO_PUPD_DOWN + 1U},
    };

    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
    {
        GpioRegisters_t port = {
            .MODER = 0x11111111U, .OTYPER = 0x2222U, .OSPEEDR = 0x33333333U, .PUPDR = 0x44444444U};
        GpioRegisters_t before = port;

        DriverStatus_e status =
            gpioInit(&port, bad[i].pin, bad[i].mode, bad[i].otype, bad[i].ospeed, bad[i].pupd);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
        TEST_ASSERT_EQUAL_MEMORY(&before, &port, sizeof(port));
    }
}

/** Every pin, at the boundary af of 15 and af 0: only that pin's nibble changes, in AFRL for pins
 *  0-7 and AFRH for 8-15 (pins 7 and 8 straddle the register boundary). */
void test_gpio_driver_set_alternate_function_every_pin_changes_only_its_nibble(void)
{
    for (uint32_t pin = 0U; pin <= 15U; pin++)
    {
        uint32_t shift = (pin % 8U) * 4U;
        GpioRegisters_t set = {0};
        GpioRegisters_t clear = {.AFRL = 0xFFFFFFFFU, .AFRH = 0xFFFFFFFFU};

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK,
                          gpioSetAlternateFunction(&set, (GpioPin_e)pin, GPIO_AF_MAX));
        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, gpioSetAlternateFunction(&clear, (GpioPin_e)pin, 0U));

        TEST_ASSERT_EQUAL_HEX32((pin < 8U) ? (GPIO_AF_MAX << shift) : 0U, set.AFRL);
        TEST_ASSERT_EQUAL_HEX32((pin < 8U) ? 0U : (GPIO_AF_MAX << shift), set.AFRH);
        TEST_ASSERT_EQUAL_HEX32((pin < 8U) ? ~(0xFU << shift) : 0xFFFFFFFFU, clear.AFRL);
        TEST_ASSERT_EQUAL_HEX32((pin < 8U) ? 0xFFFFFFFFU : ~(0xFU << shift), clear.AFRH);
    }
}

/** gpioSetAlternateFunction touches nothing but AFRL/AFRH (it must not change MODER). */
void test_gpio_driver_set_alternate_function_touches_only_afr(void)
{
    GpioRegisters_t port = {.MODER = 0x11111111U,
                            .OTYPER = 0x2222U,
                            .OSPEEDR = 0x33333333U,
                            .PUPDR = 0x44444444U,
                            .ODR = 0x5555U};
    GpioRegisters_t before = port;

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, gpioSetAlternateFunction(&port, GPIO_PIN_9, 7U));

    port.AFRH = before.AFRH;
    TEST_ASSERT_EQUAL_MEMORY(&before, &port, sizeof(port));
}

/** gpioDeinit's AF clear touches only its own nibble, at the AFRL/AFRH boundary pins 7 and 8. */
void test_gpio_driver_deinit_af_clear_changes_only_its_nibble_at_the_register_boundary(void)
{
    GpioRegisters_t low = {.AFRL = 0xFFFFFFFFU, .AFRH = 0xFFFFFFFFU};
    GpioRegisters_t high = {.AFRL = 0xFFFFFFFFU, .AFRH = 0xFFFFFFFFU};

    gpioDeinit(&low, GPIO_PIN_7);
    gpioDeinit(&high, GPIO_PIN_8);

    TEST_ASSERT_EQUAL_HEX32(0x0FFFFFFFU, low.AFRL);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, low.AFRH);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, high.AFRL);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFF0U, high.AFRH);
}

/** gpioWritePin writes BSRR *exactly* (a write-only register: an OR would set other bits) and
 *  leaves ODR alone, for every pin including 0 and 15. */
void test_gpio_driver_write_pin_writes_bsrr_exactly_and_leaves_odr(void)
{
    for (uint32_t pin = 0U; pin <= 15U; pin++)
    {
        GpioRegisters_t high = {.BSRR = 0xA5A5A5A5U, .ODR = 0x5A5AU};
        GpioRegisters_t low = {.BSRR = 0xA5A5A5A5U, .ODR = 0x5A5AU};

        gpioWritePin(&high, (GpioPin_e)pin, GPIO_PIN_SET);
        gpioWritePin(&low, (GpioPin_e)pin, GPIO_PIN_RESET);

        TEST_ASSERT_EQUAL_HEX32(1UL << pin, high.BSRR);
        TEST_ASSERT_EQUAL_HEX32(1UL << (pin + 16U), low.BSRR);
        TEST_ASSERT_EQUAL_HEX32(0x5A5AU, high.ODR);
        TEST_ASSERT_EQUAL_HEX32(0x5A5AU, low.ODR);
    }
}

/** gpioTogglePin decides from ODR (what the pin is being driven to), not IDR (what the pad reads):
 *  ODR low with IDR high must still set the pin. */
void test_gpio_driver_toggle_pin_reads_odr_not_idr(void)
{
    GpioRegisters_t port = {.ODR = 0U, .IDR = 1U << 5};

    gpioTogglePin(&port, GPIO_PIN_5);

    TEST_ASSERT_EQUAL_HEX32(1UL << 5, port.BSRR);
}
