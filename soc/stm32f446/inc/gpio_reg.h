/**
 * @file gpio_reg.h
 * @brief GPIO registers for ports A to H (RM0390 section 7.4).
 *
 * Per-pin fields: MODER, OSPEEDR and PUPDR use 2 bits at `pin * 2`; OTYPER,
 * IDR and ODR use bit `pin`; AFRL/AFRH use 4 bits at `(pin % 8) * 4`. BSRR
 * sets pin `n` with bit `n` and resets it with bit `n + 16`, atomically.
 */
#ifndef GPIO_REG_H
#define GPIO_REG_H

#include <stdint.h>

/**
 * @addtogroup gpio_registers
 * @{
 */

/** @name Base addresses (AHB1)
 *  @{ */
#define GPIOA_BASE (0x40020000UL) ///< GPIOA
#define GPIOB_BASE (0x40020400UL) ///< GPIOB
#define GPIOC_BASE (0x40020800UL) ///< GPIOC
#define GPIOD_BASE (0x40020C00UL) ///< GPIOD
#define GPIOE_BASE (0x40021000UL) ///< GPIOE
#define GPIOF_BASE (0x40021400UL) ///< GPIOF
#define GPIOG_BASE (0x40021800UL) ///< GPIOG
#define GPIOH_BASE (0x40021C00UL) ///< GPIOH
/** @} */

/** @brief GPIO port register block. Every register is \regaccess{rw} unless noted. */
typedef struct GpioRegisters_t
{
    volatile uint32_t MODER;     ///< 0x00: mode, 2 bits per pin
    volatile uint32_t OTYPER;    ///< 0x04: output type, 1 bit per pin
    volatile uint32_t OSPEEDR;   ///< 0x08: output speed, 2 bits per pin
    volatile uint32_t PUPDR;     ///< 0x0C: pull-up/pull-down, 2 bits per pin
    volatile const uint32_t IDR; ///< 0x10: input data, \regaccess{ro}
    volatile uint32_t ODR;       ///< 0x14: output data
    volatile uint32_t BSRR;      ///< 0x18: bit set/reset, \regaccess{wo}
    volatile uint32_t LCKR;      ///< 0x1C: configuration lock
    volatile uint32_t AFRL;      ///< 0x20: alternate function, pins 0-7
    volatile uint32_t AFRH;      ///< 0x24: alternate function, pins 8-15
} GpioRegisters_t;

/** @name Port instances
 *  @{ */
#define GPIOA ((GpioRegisters_t *)GPIOA_BASE) ///< Port A
#define GPIOB ((GpioRegisters_t *)GPIOB_BASE) ///< Port B
#define GPIOC ((GpioRegisters_t *)GPIOC_BASE) ///< Port C
#define GPIOD ((GpioRegisters_t *)GPIOD_BASE) ///< Port D
#define GPIOE ((GpioRegisters_t *)GPIOE_BASE) ///< Port E
#define GPIOF ((GpioRegisters_t *)GPIOF_BASE) ///< Port F
#define GPIOG ((GpioRegisters_t *)GPIOG_BASE) ///< Port G
#define GPIOH ((GpioRegisters_t *)GPIOH_BASE) ///< Port H
/** @} */

/** @name MODER values
 *  @{ */
#define GPIO_MODE_INPUT (0x0U)  ///< Input
#define GPIO_MODE_OUTPUT (0x1U) ///< General-purpose output
#define GPIO_MODE_AF (0x2U)     ///< Alternate function
#define GPIO_MODE_ANALOG (0x3U) ///< Analog
/** @} */

/** @name OTYPER values
 *  @{ */
#define GPIO_OTYPE_PP (0x0U) ///< Push-pull
#define GPIO_OTYPE_OD (0x1U) ///< Open-drain
/** @} */

/** @name OSPEEDR values
 *  @{ */
#define GPIO_OSPEED_LOW (0x0U)    ///< Low speed
#define GPIO_OSPEED_MEDIUM (0x1U) ///< Medium speed
#define GPIO_OSPEED_FAST (0x2U)   ///< Fast speed
#define GPIO_OSPEED_HIGH (0x3U)   ///< High speed
/** @} */

/** @name PUPDR values
 *  @{ */
#define GPIO_PUPD_NONE (0x0U) ///< No pull
#define GPIO_PUPD_UP (0x1U)   ///< Pull-up
#define GPIO_PUPD_DOWN (0x2U) ///< Pull-down
/** @} */

/** @name Non-zero reset values
 *  Every other GPIO register resets to 0 (IDR excepted). The exceptions are
 *  the SWD/JTAG pins: PA13 (SWDIO), PA14 (SWCLK), PA15 (JTDI), PB3 (SWO) and
 *  PB4 (NJTRST).
 *  GPIOA_OSPEEDR is 0 on the F446, unlike the F401/F411 (0x0C000000); checked
 *  against ST's F446 SVD.
 *  @{ */
#define GPIOA_MODER_RESET (0xA8000000U)   ///< PA13-15 in AF mode
#define GPIOA_PUPDR_RESET (0x64000000U)   ///< PA13/PA15 pull-up, PA14 pull-down
#define GPIOB_MODER_RESET (0x00000280U)   ///< PB3/PB4 in AF mode
#define GPIOB_OSPEEDR_RESET (0x000000C0U) ///< PB3 high speed
#define GPIOB_PUPDR_RESET (0x00000100U)   ///< PB4 pull-up
/** @} */

/** @} */

#endif /* GPIO_REG_H */
