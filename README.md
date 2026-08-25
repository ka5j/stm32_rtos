# STM32F446RE Bare-Metal RTOS

A preemptive RTOS for the STM32F446RE, built from scratch on direct register-level access, without HAL/LL or CMSIS device headers. All peripheral and core register structures are derived directly from the reference manual and mapped to their memory addresses.

## Getting Started

**Prerequisites:**
- [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (`arm-none-eabi-gcc`) on the `PATH`
- [OpenOCD](https://openocd.org/) on the `PATH` — required for `flash`, `erase`, `debug`
- GNU Make
- `clang-format`, `cppcheck`, `doxygen` on the `PATH` — required for `make format-check`, `make lint`, and `make docs`, and consequently for the pre-commit hook to execute
  - macOS: `brew install clang-format cppcheck doxygen`
  - Ubuntu/Debian: `sudo apt-get install clang-format cppcheck doxygen`
- Reference documentation (useful, not required to build): RM0390 (F446 reference manual), PM0214 (Cortex-M4 programming manual), UM1724 (Nucleo-64 user manual)

**Setup:**

1. Clone the repository:
   ```sh
   git clone https://github.com/ka5j/stm32_rtos.git
   cd stm32_rtos
   ```

2. Enable the pre-commit hook (one-time setup; without it, commits bypass the format/lint/docs/build checks):
   ```sh
   git config core.hooksPath .githooks
   chmod +x .githooks/pre-commit
   ```

3. Build. Produces `.elf`/`.bin`/`.hex` in `build/` and prints a size report:
   ```sh
   make
   ```

4. Connect the Nucleo-F446RE over USB (onboard ST-LINK/V2-1), then flash:
   ```sh
   make flash
   ```

`tools/openocd.cfg` and `linker/STM32F446RE.ld` are included in the repository. The build fails without them; no configuration is required.

**Before writing or committing any code**, review [CONTRIBUTING.md](CONTRIBUTING.md), which documents naming and layering conventions, the error-handling contract, and the checks the pre-commit hook enforces on every commit.

## Overview

The system is structured in layers, starting from boot (linker script and startup file) and building upward. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the complete directory tree and layer diagram. Generated API documentation (Doxygen) is published at <https://ka5j.github.io/stm32_rtos/> and rebuilt on every push to `main` — see [docs/VERSIONING.md](docs/VERSIONING.md) for the release policy governing what is published there.

1. **Core / device registers** — hand-written structures for Cortex-M4 core peripherals (NVIC, SysTick, SCB) and F446-specific peripherals (GPIO, RCC, UART)
2. **Drivers** — direct register manipulation (GPIO, RCC/clock configuration, UART, NVIC, SysTick); no application-facing logic
3. **API** (`api/`) — the layer application code calls directly (`led_on()`, `debug_print()`, etc.), abstracting which pin or peripheral is involved
4. **BSP** — Nucleo-F446RE board-specific pin and peripheral mapping, consumed by the API layer
5. **RTOS kernel** — scheduler, task control blocks, PendSV-based context switching
6. **RTOS API** (`rtos/api/`) — the syscall-style interface application code uses (`task_create()`, `task_delay()`, semaphores, queues)
7. **Application** — task code; includes only from `api/` and `rtos/api/`, never from drivers or the kernel directly

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
| `make clean`        | Remove the `build/` directory and generated `docs/html/`                                          |
| `make format`       | Apply `.clang-format` to every tracked `.c`/`.h` file in place                                     |
| `make format-check` | Non-mutating formatting check; fails if any tracked file would be reformatted                      |
| `make lint`         | Run `cppcheck` (including a MISRA C:2012 subset via `--addon=misra`) across the project; fails on any finding |
| `make docs`         | Run Doxygen; fails if any documented file has undocumented members ([details](CONTRIBUTING.md))     |
| `make test`         | Compile and run host-side unit tests (`tests/unit/`) against Unity; fails on any test failure       |

## Status

**Completed:** The boot pipeline (startup file, linker script, Makefile) builds and flashes successfully. The register layer is complete, documented, and covered by host-side unit tests, comprising every Cortex-M4 core peripheral this project models (`core/inc/`: MPU, NVIC, SCB, SysTick) and every F446-specific peripheral it models (`device/inc/`: EXTI, Flash interface, GPIO, IWDG, PWR, RCC, SYSCFG, UART, WWDG), with one `test_<peripheral>_reg.c` per header aggregated by `tests/unit/test_runner.c`. The development pipeline — build, formatting, lint (including a MISRA C:2012 subset), Doxygen coverage, unit tests, pre-commit hook, and CI — is fully implemented and verified, including CI checks that enforce documentation and test coverage on every new or modified source file. See [CONTRIBUTING.md](CONTRIBUTING.md)'s CI triggers section for the exact checks run locally, on push, on pull request, and on hardware.

**Not started:** The drivers, API, BSP, and RTOS kernel/API layers remain empty scaffolding; no driver logic has been implemented. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the target layout and [CONTRIBUTING.md](CONTRIBUTING.md) for the conventions these layers must follow.

## License

MIT — see [LICENSE](LICENSE).
