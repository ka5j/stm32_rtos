#ifndef GPIO_REG_H
#define GPIO_REG_H

#include <stdint.h>

#define GPIOA_BASE (0x40020000UL)
#define GPIOB_BASE (0x40020400UL)
#define GPIOC_BASE (0x40020800UL)
#define GPIOD_BASE (0x40020C00UL)
#define GPIOE_BASE (0x40021000UL)
#define GPIOF_BASE (0x40021400UL)
#define GPIOG_BASE (0x40021800UL)
#define GPIOH_BASE (0x40021C00UL)

typedef struct
{
    volatile uint32_t MODER;   // 0x00: mode register
    volatile uint32_t OTYPER;  // 0x04: output type register
    volatile uint32_t OSPEEDR; // 0x08: output speed register
    volatile uint32_t PUPDR;   // 0x0C: pull-up/pull-down register
    volatile uint32_t IDR;     // 0x10: input data register
    volatile uint32_t ODR;     // 0x14: output data register
    volatile uint32_t BSRR;    // 0x18: bit set/reset register
    volatile uint32_t LCKR;    // 0x1C: configuration lock register
    volatile uint32_t AFRL;    // 0x20: alternate function low register (pins 0-7)
    volatile uint32_t AFRH;    // 0x24: alternate function high register (pins 8-15)
} GpioRegisters_t;

#define GPIOA ((GpioRegisters_t *)GPIOA_BASE)
#define GPIOB ((GpioRegisters_t *)GPIOB_BASE)
#define GPIOC ((GpioRegisters_t *)GPIOC_BASE)
#define GPIOD ((GpioRegisters_t *)GPIOD_BASE)
#define GPIOE ((GpioRegisters_t *)GPIOE_BASE)
#define GPIOF ((GpioRegisters_t *)GPIOF_BASE)
#define GPIOG ((GpioRegisters_t *)GPIOG_BASE)
#define GPIOH ((GpioRegisters_t *)GPIOH_BASE)

/* --- MODER field values (2 bits per pin, shift = pin * 2) --- */
#define GPIO_MODE_INPUT  (0x0U)
#define GPIO_MODE_OUTPUT (0x1U)
#define GPIO_MODE_AF     (0x2U)
#define GPIO_MODE_ANALOG (0x3U)

/* --- OTYPER field values (1 bit per pin) --- */
#define GPIO_OTYPE_PP (0x0U) // push-pull
#define GPIO_OTYPE_OD (0x1U) // open-drain

/* --- OSPEEDR field values (2 bits per pin, shift = pin * 2) --- */
#define GPIO_OSPEED_LOW    (0x0U)
#define GPIO_OSPEED_MEDIUM (0x1U)
#define GPIO_OSPEED_FAST   (0x2U)
#define GPIO_OSPEED_HIGH   (0x3U)

/* --- PUPDR field values (2 bits per pin, shift = pin * 2) --- */
#define GPIO_PUPD_NONE (0x0U)
#define GPIO_PUPD_UP   (0x1U)
#define GPIO_PUPD_DOWN (0x2U)

#endif /* GPIO_REG_H */
