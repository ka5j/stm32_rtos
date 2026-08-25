# Register & API Reference

This is the generated reference for a from-scratch, bare-metal preemptive
RTOS for the STM32F446RE (Cortex-M4F) — no HAL, no CMSIS device headers.
Every peripheral and core register struct here is hand-derived from the
reference manual and mapped directly to its memory address.

## Current status

Only the **register layer** is implemented so far: hand-written struct
definitions for every Cortex-M4 core peripheral this project models (NVIC,
SCB, MPU, SysTick) and every STM32F446-specific peripheral it models (GPIO,
RCC, SYSCFG, UART, EXTI, Flash interface, PWR, IWDG, WWDG). The driver/API/BSP/RTOS
layers built on top of these registers are still in progress — see the
[GitHub repository](https://github.com/ka5j/stm32_rtos) for current status,
build instructions, and contribution conventions. This site only documents
code; process docs (contributing, versioning, security) live there too.

## Where to start

The **Topics** page groups every register header into the two categories
this project currently documents:

- **Cortex-M4 Core Peripherals** — registers fixed by the Armv7-M
  architecture itself (NVIC, SCB, MPU, SysTick), derived from PM0214.
- **STM32F446 Peripherals** — registers specific to this STM32 part (GPIO,
  RCC, SYSCFG, UART, EXTI, Flash, PWR, IWDG, WWDG), derived from RM0390.

Each register struct documents its memory-mapped layout with a per-field
byte offset; each bit-definition macro documents its bit position and
access semantics — read-only, write-only, or read/write-1-to-clear.
