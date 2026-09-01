/**
 * @file gpio_reg.h
 * @brief GPIO peripheral register definitions for the STM32F446xx family.
 *
 * Register layout and field values derived from RM0390 (STM32F446xx
 * reference manual), section 8: General-purpose I/Os (GPIO).
 */
#ifndef GPIO_REG_H
#define GPIO_REG_H

#include <stdint.h>

/**
 * @addtogroup gpio_registers
 * @{
 */

#define GPIOA_BASE (0x40020000UL) ///< GPIOA peripheral base address
#define GPIOB_BASE (0x40020400UL) ///< GPIOB peripheral base address
#define GPIOC_BASE (0x40020800UL) ///< GPIOC peripheral base address
#define GPIOD_BASE (0x40020C00UL) ///< GPIOD peripheral base address
#define GPIOE_BASE (0x40021000UL) ///< GPIOE peripheral base address
#define GPIOF_BASE (0x40021400UL) ///< GPIOF peripheral base address
#define GPIOG_BASE (0x40021800UL) ///< GPIOG peripheral base address
#define GPIOH_BASE (0x40021C00UL) ///< GPIOH peripheral base address

/**
 * @brief GPIO peripheral register map (RM0390 section 8.4).
 *
 * One instance per port, overlaid on the port's base address (see the
 * GPIOx_BASE defines above and the GPIOx pointer macros below).
 */
typedef struct GpioRegisters_t
{
    volatile uint32_t MODER;     ///< 0x00: mode register
    volatile uint32_t OTYPER;    ///< 0x04: output type register
    volatile uint32_t OSPEEDR;   ///< 0x08: output speed register
    volatile uint32_t PUPDR;     ///< 0x0C: pull-up/pull-down register
    volatile const uint32_t IDR; ///< 0x10: input data register (READ only)
    volatile uint32_t ODR;       ///< 0x14: output data register
    volatile uint32_t BSRR;      ///< 0x18: bit set/reset register
    volatile uint32_t LCKR;      ///< 0x1C: configuration lock register
    volatile uint32_t AFRL;      ///< 0x20: alternate function low register (pins 0-7)
    volatile uint32_t AFRH;      ///< 0x24: alternate function high register (pins 8-15)
} GpioRegisters_t;

#define GPIOA ((GpioRegisters_t *)GPIOA_BASE) ///< Pointer to the GPIOA register block
#define GPIOB ((GpioRegisters_t *)GPIOB_BASE) ///< Pointer to the GPIOB register block
#define GPIOC ((GpioRegisters_t *)GPIOC_BASE) ///< Pointer to the GPIOC register block
#define GPIOD ((GpioRegisters_t *)GPIOD_BASE) ///< Pointer to the GPIOD register block
#define GPIOE ((GpioRegisters_t *)GPIOE_BASE) ///< Pointer to the GPIOE register block
#define GPIOF ((GpioRegisters_t *)GPIOF_BASE) ///< Pointer to the GPIOF register block
#define GPIOG ((GpioRegisters_t *)GPIOG_BASE) ///< Pointer to the GPIOG register block
#define GPIOH ((GpioRegisters_t *)GPIOH_BASE) ///< Pointer to the GPIOH register block

/* --- MODER field values (2 bits per pin, shift = pin * 2) --- */
#define GPIO_MODE_INPUT (0x0U)  ///< MODER field value: input mode
#define GPIO_MODE_OUTPUT (0x1U) ///< MODER field value: output mode
#define GPIO_MODE_AF (0x2U)     ///< MODER field value: af mode
#define GPIO_MODE_ANALOG (0x3U) ///< MODER field value: analog mode

/* --- OTYPER field values (1 bit per pin) --- */
#define GPIO_OTYPE_PP (0x0U) ///< push-pull
#define GPIO_OTYPE_OD (0x1U) ///< open-drain

/* --- OSPEEDR field values (2 bits per pin, shift = pin * 2) --- */
#define GPIO_OSPEED_LOW (0x0U)    ///< OSPEEDR field value: low speed
#define GPIO_OSPEED_MEDIUM (0x1U) ///< OSPEEDR field value: medium speed
#define GPIO_OSPEED_FAST (0x2U)   ///< OSPEEDR field value: fast speed
#define GPIO_OSPEED_HIGH (0x3U)   ///< OSPEEDR field value: high speed

/* --- PUPDR field values (2 bits per pin, shift = pin * 2) --- */
#define GPIO_PUPD_NONE (0x0U) ///< PUPDR field value: none
#define GPIO_PUPD_UP (0x1U)   ///< PUPDR field value: up
#define GPIO_PUPD_DOWN (0x2U) ///< PUPDR field value: down

/** @} */

#endif /* GPIO_REG_H */
