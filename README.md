# STM32F446RE Bare-Metal RTOS

A from-scratch preemptive RTOS built directly on register-level access — no HAL/LL, no CMSIS device headers. All peripheral and core register structs are hand-derived from the reference manual and mapped to raw addresses.

## Quick Start

```sh
git clone <this-repo>
cd stm32_rtos
make            # build .elf/.bin/.hex, print a size report
# connect the Nucleo-F446RE over USB (onboard ST-LINK/V2-1)
make flash      # program and reset the board
```

## Overview

Starting from boot (linker script + startup file) and building up in layers — see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full directory tree and layer diagram:

1. **Core / device registers** — hand-written structs for Cortex-M4 core peripherals (NVIC, SysTick, SCB) and F446-specific peripherals (GPIO, RCC, UART)
2. **Drivers** — direct register manipulation (GPIO, RCC/clock config, UART, NVIC, SysTick) — no app-facing logic
3. **API** (`api/`) — the layer application code actually calls (`led_on()`, `debug_print()`, etc.) — hides which pin/peripheral is involved
4. **BSP** — Nucleo-F446RE board-specific pin/peripheral mapping, consumed by the API layer
5. **RTOS kernel** — scheduler, task control blocks, PendSV-based context switching
6. **RTOS API** (`rtos/api/`) — the syscall-style interface app code uses (`task_create()`, `task_delay()`, semaphores, queues)
7. **Application** — task code; only includes from `api/` and `rtos/api/`, never touches drivers or the kernel directly

## Hardware

- NUCLEO-F446RE (STM32F446RE, Cortex-M4F, 512 KB flash / 128 KB SRAM)
- Onboard LED (LD2) on PA5, user button (B1) on PC13, ST-LINK virtual COM port on USART2 (PA2/PA3)

## Tools

- `arm-none-eabi-gcc` — cross-compiler/toolchain
- OpenOCD — flashing and debugging via onboard ST-LINK/V2-1
- GNU Make
- Reference docs: RM0390 (F446 reference manual), PM0214 (Cortex-M4 programming manual), UM1724 (Nucleo-64 user manual)

## Prerequisites

- [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (`arm-none-eabi-gcc`) on your `PATH`
- [OpenOCD](https://openocd.org/) on your `PATH` — required for `flash`, `erase`, `debug`
- GNU Make
- `tools/openocd.cfg` present in the repo, configured for the onboard ST-LINK and `stm32f4x` target
- `linker/STM32F446RE.ld` present — the build will fail without it

## Make Commands

| Command             | Description                                                                                        |
| ------------------- | -------------------------------------------------------------------------------------------------- |
| `make` / `make all` | Compile and link the project; produces `.elf`, `.bin`, `.hex` in `build/` and prints a size report |
| `make size`         | Print flash/RAM usage of the current `.elf`                                                        |
| `make flash`        | Build (if needed) and program the board over the onboard ST-LINK                                   |
| `make erase`        | Full chip mass-erase                                                                               |
| `make debug`        | Build (if needed), start OpenOCD as a GDB server, and attach GDB                                   |
| `make re`           | `clean` followed by `all`                                                                          |
| `make clean`        | Remove the `build/` directory                                                                      |

## Status

Work in progress. Boot pipeline (startup file, linker script, Makefile) builds and flashes successfully. Core/device register structs, drivers, the API/BSP layers, and the RTOS kernel/API are being implemented layer by layer — `device/inc/rcc_reg.h` is the furthest along; most other headers are currently empty scaffolding. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the target layout.

## License

MIT — see [LICENSE](LICENSE).
