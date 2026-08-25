# Register & API Reference

This is the generated reference for a preemptive, bare-metal RTOS for the
STM32F446RE (Cortex-M4F), built from scratch without HAL or CMSIS device
headers. Every peripheral and core register structure is derived directly
from the reference manual and mapped to its memory address.

## Current status

Only the **register layer** is implemented at this stage: hand-written
structure definitions for every Cortex-M4 core peripheral this project
models (NVIC, SCB, MPU, SysTick) and every STM32F446-specific peripheral it
models (GPIO, RCC, SYSCFG, UART, EXTI, Flash interface, PWR, IWDG, WWDG).
The driver, API, BSP, and RTOS layers built on top of these registers
remain in progress; see the
[GitHub repository](https://github.com/ka5j/stm32_rtos) for current status,
build instructions, and contribution conventions. This site documents code
only; process documentation (contributing, versioning, security) resides
there as well.

## Where to start

The **Topics** page groups every register header into the two categories
this project currently documents:

- **Cortex-M4 Core Peripherals** — registers fixed by the Armv7-M
  architecture itself (NVIC, SCB, MPU, SysTick), derived from PM0214.
- **STM32F446 Peripherals** — registers specific to this STM32 part (GPIO,
  RCC, SYSCFG, UART, EXTI, Flash, PWR, IWDG, WWDG), derived from RM0390.

Each register structure documents its memory-mapped layout with a
per-field byte offset; each bit-definition macro documents its bit
position and access semantics — read-only, write-only, or
read/write-1-to-clear.
