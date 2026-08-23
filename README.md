# STM32F446RE Bare-Metal RTOS

A from-scratch preemptive RTOS built directly on register-level access — no HAL/LL, no CMSIS device headers. All peripheral and core register structs are hand-derived from the reference manual and mapped to raw addresses.

## Getting Started

**Prerequisites:**
- [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (`arm-none-eabi-gcc`) on your `PATH`
- [OpenOCD](https://openocd.org/) on your `PATH` — required for `flash`, `erase`, `debug`
- GNU Make
- `clang-format`, `cppcheck`, `doxygen` on your `PATH` — required for `make format-check`/`make lint`/`make docs`, and therefore for the pre-commit hook below to run at all
  - macOS: `brew install clang-format cppcheck doxygen`
  - Ubuntu/Debian: `sudo apt-get install clang-format cppcheck doxygen`
- Reference docs (useful, not required to build): RM0390 (F446 reference manual), PM0214 (Cortex-M4 programming manual), UM1724 (Nucleo-64 user manual)

**Setup, step by step:**

1. Clone the repository:
   ```sh
   git clone https://github.com/ka5j/stm32_rtos.git
   cd stm32_rtos
   ```

2. Enable the pre-commit hook (one-time — without this, commits skip the format/lint/docs/build checks entirely):
   ```sh
   git config core.hooksPath .githooks
   chmod +x .githooks/pre-commit
   ```

3. Build — produces `.elf`/`.bin`/`.hex` in `build/` and prints a size report:
   ```sh
   make
   ```

4. Connect the Nucleo-F446RE over USB (onboard ST-LINK/V2-1), then flash:
   ```sh
   make flash
   ```

`tools/openocd.cfg` and `linker/STM32F446RE.ld` already ship in the repo — the build fails without them, but there's nothing to configure, they're just there.

**Before writing or committing any code**, read [CONTRIBUTING.md](CONTRIBUTING.md) — it covers naming/layering conventions, the error-handling contract, and what the pre-commit hook you just enabled checks on every commit.

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
| `make format`       | Apply `.clang-format` to every tracked `.c`/`.h` file in place                                     |
| `make format-check` | Non-mutating formatting check; fails if any tracked file would be reformatted                      |
| `make lint`         | Run `cppcheck` across the project; fails on any finding                                             |
| `make docs`         | Run Doxygen; fails if any documented file has undocumented members ([details](CONTRIBUTING.md))     |
| `make test`         | Compile and run host-side unit tests (`tests/unit/`) against Unity; fails on any test failure       |

## Status

**Done:** boot pipeline (startup file, linker script, Makefile) builds and flashes successfully. Register layer for GPIO, RCC, and SysTick (`device/inc/`, `core/inc/`) is complete, documented, and covered by host-side unit tests. The full dev pipeline — build, formatting, lint, Doxygen coverage, unit tests, pre-commit hook, CI — is built and verified, including CI checks that enforce doc coverage and test coverage on every new or modified source file.

**Not started:** drivers, API, BSP, and RTOS kernel/API layers are still empty scaffolding — no driver logic exists yet. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the target layout and [CONTRIBUTING.md](CONTRIBUTING.md) for the conventions that layer needs to follow as it's written.

## License

MIT — see [LICENSE](LICENSE).
