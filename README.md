# STM32F446RE Bare-Metal RTOS

A from-scratch preemptive RTOS built directly on register-level access - no HAL/LL, no CMSIS device headers. All peripheral and core register structs are hand-derived from the reference manual and mapped to raw addresses.

## Overview

Starting from boot (linker script + startup file) and building up in layers:

1. **Core / device registers** — hand-written structs for Cortex-M4 core peripherals (NVIC, SysTick, SCB) and F446-specific peripherals (GPIO, RCC, UART, SPI)
2. **Drivers** — direct register manipulation (GPIO, RCC/clock config, UART, NVIC, SysTick) — no app-facing logic
3. **API** — the layer application code actually calls (`led_on()`, `debug_print()`, etc.) — hides which pin/peripheral is involved
4. **BSP** — Nucleo-F446RE board-specific pin/peripheral mapping, consumed by the API layer
5. **RTOS kernel** — scheduler, task control blocks, PendSV-based context switching
6. **RTOS API** — the syscall-style interface app code uses (`task_create()`, `task_delay()`, semaphores, queues)
7. **Application** — task code; only includes from `api/` and `rtos/api/`, never touches drivers or the kernel directly

## Hardware

- NUCLEO-F446RE (STM32F446RE, Cortex-M4F)

## Tools

- `arm-none-eabi-gcc` — cross-compiler/toolchain
- OpenOCD - flashing and debugging via onboard ST-LINK/V2-1
- GNU Make
- Reference docs: RM0390 (F446 reference manual), PM0214 (Cortex-M4 programming manual), UM1724 (Nucleo-64 user manual)

## Building

```
make
```

## Flashing / Debugging

```
openocd -f tools/openocd.cfg
```

## Status

Work in progress
