# Register & API Reference

This is the generated reference for a preemptive, bare-metal RTOS for the
STM32F446RE (Cortex-M4F), built from scratch without HAL or CMSIS device
headers. Every peripheral and core register structure is derived directly
from the reference manual and mapped to its memory address.

## Current status

The **register layer** and the **driver layer** are implemented and
complete, and the **board support package** has its first function (the
180 MHz clock bring-up) - what is documented here is what exists. For the
authoritative, per-layer status, see the Status section of the
[GitHub repository](https://github.com/ka5j/stm32_rtos)'s README, which
is the single source of truth for it rather than one of several copies
that drift apart. That repository also carries build instructions and the
process documentation (contributing, versioning, security); this site
documents code only.

## Where to start

The **Topics** page groups every documented header by architectural layer:

- **Register Layer** — hand-derived register structs, with one subgroup per
  peripheral: **Cortex-M4 Core Peripherals** (NVIC, SCB, FPU, MPU, SysTick,
  derived from PM0214) and **STM32F446 Peripherals** (GPIO, RCC, SYSCFG,
  UART, EXTI, Flash, PWR, IWDG, WWDG, derived from RM0390).
- **Drivers** — register-level driver logic consuming the layer above:
  the shared `DriverStatus_e` error contract and every driver
  (GPIO, RCC, Flash, PWR, UART, NVIC, SysTick).
- **Board Support Package** — Nucleo-F446RE specifics built on the drivers;
  today `bspClockInit()` and the `BSP_*_HZ` clock constants.

Each register structure documents its memory-mapped layout with a
per-field byte offset; each bit-definition macro documents its bit
position and access semantics — read-only, write-only, or
read/write-1-to-clear.

Every file page includes an include/included-by dependency graph, and
every group page in Topics includes a group graph — both generated
directly from the real `#include` graph and `@ingroup` structure, so they
can't silently drift from what the code actually does the way a hand-
maintained diagram could. Source code is browsable and cross-linked: click
any macro or struct member to jump to its definition, or scroll to the
bottom of its documentation to see every other documented place that
references it.
