# Register & API Reference

This is the generated reference for a preemptive, bare-metal RTOS for the
STM32F446RE (Cortex-M4F), built from scratch without HAL or CMSIS device
headers. Every peripheral and core register structure is derived directly
from the reference manual and mapped to its memory address.

## Current status

The **register layer** is implemented and complete: hand-written structure
definitions for every Cortex-M4 core peripheral this project models (NVIC,
SCB, MPU, SysTick) and every STM32F446-specific peripheral it models (GPIO,
RCC, SYSCFG, UART, EXTI, Flash interface, PWR, IWDG, WWDG).
`drivers/inc/driver_status.h`, the shared `DriverStatus_e` error contract
every driver function will use, is also implemented, ahead of any actual
driver logic. The rest of the driver layer, plus API, BSP, and RTOS, remain
in progress; see the
[GitHub repository](https://github.com/ka5j/stm32_rtos) for current status,
build instructions, and contribution conventions. This site documents code
only; process documentation (contributing, versioning, security) resides
there as well.

## Where to start

The **Topics** page groups every documented header by architectural layer:

- **Register Layer** — hand-derived register structs, with one subgroup per
  peripheral: **Cortex-M4 Core Peripherals** (NVIC, SCB, FPU, MPU, SysTick,
  derived from PM0214) and **STM32F446 Peripherals** (GPIO, RCC, SYSCFG,
  UART, EXTI, Flash, PWR, IWDG, WWDG, derived from RM0390).
- **Drivers** — register-level driver logic consuming the layer above;
  currently just the shared `DriverStatus_e` error contract.

Each register structure documents its memory-mapped layout with a
per-field byte offset; each bit-definition macro documents its bit
position and access semantics — read-only, write-only, or
read/write-1-to-clear.
