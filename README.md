# STM32F446RE Bare-Metal RTOS

A preemptive RTOS for the STM32F446RE, built from scratch on direct register-level access, without HAL/LL or CMSIS device headers. All peripheral and core register structures are derived directly from the reference manual and mapped to their memory addresses.

## Getting Started

**Prerequisites:**
- [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (`arm-none-eabi-gcc`) on the `PATH`
- [OpenOCD](https://openocd.org/) on the `PATH` — required for `flash`, `erase`, `debug`
- GNU Make
- `clang-format`, `cppcheck`, `doxygen` on the `PATH` — required for `make format-check`, `make lint`, and `make docs`, and consequently for the pre-commit hook to execute
  - macOS: `brew install clang-format cppcheck doxygen graphviz`
  - Ubuntu/Debian: `sudo apt-get install clang-format cppcheck doxygen graphviz`
  - `graphviz` (the `dot` tool) is required for `make docs`'s include/directory/group diagrams (`Doxyfile`'s `HAVE_DOT`); without it on the `PATH`, Doxygen silently omits every diagram instead of failing.
- `gcovr` — required for `make coverage` only (not the pre-commit hook): `pip install gcovr`
- A host C compiler (`cc`) for `make test`/`make coverage`, which build and run natively rather than cross-compiling
  - **macOS 27 note**: the Command Line Tools ship an SDK whose `.tbd` stubs declare an `arm64e.x1` target that the linker in the *same* install rejects, so any host link fails with `tapi error: malformed file ... unknown architecture`. This breaks `make test`, `make coverage`, and therefore the whole pre-commit hook. Point the toolchain at an earlier SDK that is still present:
    ```sh
    export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk
    ```
    Put that in your shell profile so the pre-commit hook inherits it. This is deliberately not baked into the Makefile — an `-isysroot` hardcoded to a macOS path would break Linux CI and every other machine.
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

The system is structured in layers, starting from boot (linker script and startup file) and building upward. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the complete directory tree and layer diagram. Generated API documentation (Doxygen) is published at <https://ka5j.github.io/stm32_rtos/> and rebuilt on every push to `main` — see [docs/VERSIONING.md](docs/VERSIONING.md) for the release policy governing what is published there. See [CHANGELOG.md](CHANGELOG.md) for what changed in each release.

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
| `make test`         | Compile and run host-side unit tests (`tests/unit/`) against Unity, then cross-check `core/inc/nvic_reg.h` against the startup vector table and every test function against its registration in `test_runner.c`; fails on any test failure or mismatch |
| `make coverage`     | Recompile `TEST_DRIVER_SOURCES` and their tests with coverage instrumentation, run them, then fail (via `gcovr`) if line or branch coverage drops below 100% |

## Status

This section is the single source of truth for project status. [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/VERSIONING.md](docs/VERSIONING.md), and [docs/mainpage.md](docs/mainpage.md) describe structure, policy, and the generated reference respectively, and link here rather than restating what is done — an earlier revision repeated the same status paragraph in five files, and keeping five copies in step after every merge was a losing proposition.

| Layer | Status |
| ----- | ------ |
| Boot (`startup/`, `linker/`, `Makefile`) | **Complete** — builds and flashes |
| Register layer (`core/inc/`, `device/inc/`) | **Complete** — 4 core + 9 device peripherals, one `test_<peripheral>_reg.c` each |
| Development pipeline (build, format, lint, docs, tests, coverage, hook, CI) | **Complete** |
| `drivers/` | **5 of 7** — GPIO, RCC, Flash, PWR, UART done; NVIC and SysTick not started |
| `api/`, `bsp/` | **Not started** — empty scaffolding |
| `rtos/kernel/`, `rtos/api/` | **Not started** — empty scaffolding |
| `app/` | **Not started** — `main.c` is an empty loop |

**Register layer** covers every Cortex-M4 core peripheral this project models (MPU, NVIC, SCB, SysTick) and every F446-specific peripheral it models (EXTI, Flash interface, GPIO, IWDG, PWR, RCC, SYSCFG, UART, WWDG), aggregated by `tests/unit/test_runner.c`.

**Drivers** implemented so far, all holding 100% line and branch coverage under `make coverage`:

- **`driver_status.h`** — the `DriverStatus_e` contract every driver returns, per [CONTRIBUTING.md](CONTRIBUTING.md)'s error-handling contract.
- **GPIO** — `gpioInit`/`gpioDeinit`/`gpioSetAlternateFunction`/`gpioWritePin`/`gpioReadPin`/`gpioTogglePin`.
- **RCC** — clock gating (`rccGpioClockEnable`/`Disable`, `rccUsart2ClockEnable`/`Disable`, `rccSyscfgClockEnable`/`Disable`, `rccPwrClockEnable`/`Disable`) and HSI/HSE-to-PLL SYSCLK bring-up (`rccHsiEnable`/`Disable`, `rccHseEnable`/`Disable`, `rccPllConfig`/`rccPllEnable`/`rccPllDisable`, `rccBusPrescalerConfig`, `rccSysclkSwitch`). The ordered bring-up sequence a caller must follow is documented in `drivers/inc/rcc.h`'s file-level comment.
- **Flash** — `flashSetLatency`, with the read-back verification RM0390 requires.
- **PWR** — `pwrSetVoltageScale` and `pwrWaitVoltageScaleReady`, split because RM0390 permits `CR.VOS` to be written only while the PLL is off while `CSR.VOSRDY` only settles once it is on.
- **UART** — `uartInit`/`uartDeinit`/`uartFlush`/`uartTransmitByte`/`uartTransmit`/`uartReceiveByte`/`uartReceive`, blocking 8N1-class asynchronous transfer.

`0.2.0` is reserved for `drivers/`' completion — see [docs/VERSIONING.md](docs/VERSIONING.md).

**Known gap:** no driver has executed on hardware. Every one is fully covered by host tests, but `app/src/main.c` is an empty loop, so the linker garbage-collects all of `drivers/` out of the image. On-target verification is manual (`make flash`, `make debug`) and nothing currently asserts runtime behaviour — see [CONTRIBUTING.md](CONTRIBUTING.md)'s testing section for what that does and does not buy you.

## License

MIT — see [LICENSE](LICENSE).
