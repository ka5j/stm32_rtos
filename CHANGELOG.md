# Changelog

All notable changes to this project are documented here. Format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); version numbers
follow this project's own [Semantic Versioning policy](docs/VERSIONING.md),
which explains what MAJOR/MINOR/PATCH mean pre-1.0 for this repository
specifically.

## [Unreleased]

### Added

- `drivers/inc/gpio.h`, `drivers/src/gpio.c`: the GPIO driver -
  `GpioPin_e`/`GpioPinState_e` types and `gpioInit`/`gpioDeinit`/
  `gpioSetAlternateFunction`/`gpioWritePin`/`gpioReadPin`/`gpioTogglePin`,
  the first driver-layer logic this project has shipped. `gpioInit`
  validates `mode`/`otype`/`ospeed`/`pupd` before writing any register
  (including rejecting `pupd == 0x3`, reserved per RM0390) and writes
  `AFRL`/`AFRH` before `MODER` to avoid a mode-switch glitch;
  `gpioWritePin`/`gpioTogglePin` use `BSRR`, never a non-atomic `ODR`
  read-modify-write.
- `tests/unit/test_gpio.c`: 18 tests covering every validation branch and
  field-isolation case (a write to one pin's bits must not touch its
  neighbors' bits in the same register).
- `make coverage` (Makefile) + `gcovr` (pinned `8.6`) in CI: gates line and
  branch coverage at 100% on `TEST_DRIVER_SOURCES`, currently just
  `gpio.c`. Scoped to driver logic, not the register headers, which are
  pure data with no branches to cover.
- `.vscode/settings.json`: editor format-on-save wired to `.clang-format`,
  tracked via a `.gitignore` exception scoped to just that one file.

### Fixed

- `tests/unit/test_gpio.c`: GCC does not honor a `(void)` cast as
  acknowledging a `warn_unused_result`-attributed return value the way
  Clang does, so `-Werror=unused-result` turned it into a hard build
  failure on CI's `arm-none-eabi-gcc`/host `cc` (both GCC) despite passing
  every time locally on macOS/Clang. Fixed by capturing the result into a
  named variable and asserting on it, matching every other `gpioInit` call
  in the file.
- `startup/startup_stm32f446re.s`: missing trailing newline.

### Changed

- `.clang-format`: `BasedOnStyle` `GNU` → `LLVM` (GNU silently ignores
  `BreakAfterReturnType`-family overrides - confirmed by testing the same
  override under each base style; every other property was already
  explicitly pinned so `LLVM` costs nothing), 4-space indent,
  `InsertBraces`/`AllowShort*OnASingleLine: Never` so no conditional, loop,
  or non-empty function can collapse to a one-liner that hides logic,
  supporting MISRA's brace-everywhere spirit at the tooling level. Verified
  byte-identical output between clang-format 18 (CI's pin) and 22 (local)
  before adopting; `AlignConsecutiveMacros` deliberately left off since its
  column math differs between those two versions.
- `Makefile`'s `lint` target: `misra-c2012-2.5` (unused macro) on
  `device/inc/gpio_reg.h` and `misra-c2012-8.7` (external linkage used in
  only one translation unit) on `drivers/src/gpio.c` suppressed, scoped to
  those two files specifically, not the rules project-wide - both are real
  findings today (nothing in `api`/`bsp`/`app` calls this code yet) but
  neither is a defect. Remove both suppressions once `api/` gives `gpio.c`
  a real caller.
- `Makefile`'s `test` target now also compiles host-testable
  `drivers/src/*.c` files (`TEST_DRIVER_SOURCES`, currently just
  `gpio.c`) into the test binary - previously only `tests/unit/*.c` was
  ever compiled, so `gpio.c` had no way to be tested until this landed.
- `README.md`, `docs/VERSIONING.md`, `docs/mainpage.md`,
  `docs/ARCHITECTURE.md`: corrected status text that still described
  `drivers/` as having no driver logic after the GPIO driver landed, same
  class of staleness `driver_status.h` caused in `0.1.2`.
- `docs/VERSIONING.md`: clarified that PATCH covers incremental progress on
  a not-yet-complete new layer (a single driver file, not the whole layer)
  as well as fixes to an already-released one - `0.2.0` is reserved for
  `drivers/`'s actual completion (GPIO, RCC, UART, NVIC, SysTick all
  implemented and tested), not any single driver file landing.
- `CONTRIBUTING.md`: documents `make coverage` in the CI step order and
  notes it's CI-only, not part of the pre-commit hook's five checks.

## [0.1.2] - 2026-08-26

Register-layer completeness fixes, a real NVIC/vector-table cross-check,
the `drivers/` error-status contract, per-peripheral Doxygen grouping, and
this changelog.

### Added

- `drivers/inc/driver_status.h`: the `DriverStatus_e` error-status contract
  required by every driver function that can fail, per `CONTRIBUTING.md`'s
  error-handling contract - a small, closed set of failure categories,
  `DRIVER_MUST_CHECK`-marked, with a non-zero `DRIVER_STATUS_OK` and an
  explicit `DRIVER_STATUS_UNINITIALIZED` sentinel so an unassigned status
  can't silently read as success under this project's `-O0` build.
- `tools/check_vector_table.awk`, run as part of `make test`: a real
  cross-check between `core/inc/nvic_reg.h`'s `IRQn_e` enum and
  `startup/startup_stm32f446re.s`'s vector table, replacing a test that only
  asserted the enum against its own values and could not catch the two
  files drifting apart.
- Per-peripheral Doxygen subgroups (`mpu_registers`, `nvic_registers`,
  `scb_registers`, `fpu_registers`, `systick_registers`, `gpio_registers`,
  `rcc_registers`, `uart_registers`, `exti_registers`, `flash_registers`,
  `iwdg_registers`, `pwr_registers`, `syscfg_registers`, `wwdg_registers`),
  nested under `core_peripherals`/`device_peripherals` instead of every
  register header joining one flat parent group. `scb_reg.h`'s two
  architecturally distinct blocks (SCB, FPU) now render as separate groups.
- `-Werror=unused-result` in the Makefile's `CFLAGS` and host test compile
  line: promotes an ignored `DRIVER_MUST_CHECK` return from a warning to a
  build failure on both `arm-none-eabi-gcc` and the host `cc`.

### Fixed

- `core/inc/scb_reg.h`: added the missing FPU lazy-stacking fault bits to
  `SCB_CFSR` (`MLSPERR`, bit 5; `LSPERR`, bit 13) - Cortex-M4F-specific
  CFSR extensions the header omitted, relevant to any future HardFault
  handler or PendSV context switch.
- `core/inc/mpu_reg.h`: added the missing `MPU_RASR_AP_PRIV_RW_UNPRIV_RO`
  (AP=0x2) encoding, one of six legitimate MPU access-permission values -
  the one needed to make kernel-owned data read-only to task code.
- CI: Arm GNU Toolchain, cppcheck, and clang-format are now pinned to exact
  versions (`15.2.rel1`, `2.13.0-2ubuntu3`, `1:18.0-59~exp2`) instead of
  floating `apt` packages, so an archive update can only fail the build
  loudly rather than silently changing what CI enforces.

### Changed

- `device/inc/flash_reg.h`, `device/inc/rcc_reg.h`: documented previously
  unstated scope decisions (which real register fields each header
  intentionally omits, and why), matching the pattern already used in
  `uart_reg.h`/`exti_reg.h`/`syscfg_reg.h`.
- `CONTRIBUTING.md`: documents the two-level Doxygen grouping convention
  (parent layer group, per-peripheral subgroup, multi-block-per-file
  splitting) for future register headers.
- `README.md`, `docs/VERSIONING.md`, `docs/mainpage.md`, `docs/ARCHITECTURE.md`:
  corrected status text that still described `drivers/` as entirely empty
  scaffolding after `driver_status.h` landed, and updated `docs/mainpage.md`'s
  description of the Doxygen Topics page to match the per-peripheral
  subgroup structure above.

### Removed

- `.github/dependabot.yml`: Dependabot is disabled for this repository.
  `SECURITY.md`'s claim that no automated tooling watches the Actions SHA
  pins is now accurate as a result.

## [0.1.1] - 2026-08-24

Register-layer bug fixes and completeness, Doxygen reorganization, CI
hardening, documentation tone rewrite.

### Added

- `device/inc/syscfg_reg.h`: SYSCFG register layer (RM0390 section 9),
  scoped to EXTI-line port routing (`EXTICR1-4`) and I/O compensation cell
  control (`CMPCR`).
- `docs/groups.dox`: Doxygen "Topics" page group hierarchy, reorganizing
  generated docs by architectural layer instead of raw file listing.
- `docs/mainpage.md`: a proper landing page for the generated Doxygen site.
- `tests/unit/test_rcc_reg.c`, `tests/unit/test_syscfg_reg.c`.

### Fixed

- `core/inc/systick_reg.h`: added the missing `SYSTICK_CTRL.TICKINT` bit.
- `core/inc/scb_reg.h`: corrected incorrect READ-only comments on `CFSR`/
  `HFSR` fields.
- Hardware-read-only register struct fields marked `const`, CMSIS-style,
  across the register layer (`volatile const uint32_t`, not `volatile
  uint32_t`, for fields the hardware alone writes).
- `device/inc/rcc_reg.h`: filled in RCC register/field gaps needed before
  driver work could start.
- Stale peripheral lists and `make clean` documentation corrected after
  `syscfg_reg.h` landed.

### Changed

- Documentation tone rewrite across `README.md`, `CONTRIBUTING.md`,
  `SECURITY.md`, `docs/VERSIONING.md`, `docs/ARCHITECTURE.md`.
- `.github/workflows/ci.yml`: added the PR-only `@file`-block and
  test-coverage diff checks.

### Removed

- `.github/dependabot.yml`.

## [0.1.0] - 2026-08-23

Initial release: boot pipeline, complete register layer, and full
development pipeline. `drivers/`, `api/`, `bsp/`, and `rtos/` remain empty
scaffolding - no driver logic implemented yet.

### Added

- Boot pipeline: `linker/STM32F446RE.ld`, `startup/startup_stm32f446re.s`,
  and the `Makefile` (`all`/`flash`/`erase`/`debug`/`size`/`clean` targets).
  Builds and flashes successfully to a NUCLEO-F446RE.
- Register layer, hand-derived from PM0214 and RM0390 with no HAL/CMSIS
  device headers:
  - `core/inc/`: MPU, NVIC, SCB (+ FPU context control), SysTick.
  - `device/inc/`: EXTI, Flash interface, GPIO, IWDG, PWR, RCC, UART, WWDG.
- Host-side unit tests (`tests/unit/`, vendored Unity in `tests/unity/`),
  one `test_<peripheral>_reg.c` per register header, aggregated by
  `tests/unit/test_runner.c`.
- Development pipeline: `make format`/`format-check` (clang-format),
  `make lint` (cppcheck with a MISRA C:2012 subset), `make docs`
  (Doxygen, `WARN_AS_ERROR` on), `make test`, and the `.githooks/pre-commit`
  hook running all of them before every local commit.
- CI: `ci.yml` (build/format/lint/docs/test on every push and PR),
  `hil.yml` (manual-only hardware-in-the-loop smoke test on a self-hosted
  runner), `pages.yml` (Doxygen site published to GitHub Pages on push to
  `main`), `codeql.yml` (static analysis), `.github/dependabot.yml`
  (GitHub Actions dependency updates).
- `SECURITY.md`: vulnerability reporting policy, threat model, CI/CD trust
  boundaries, and the SHA-pinning policy for third-party GitHub Actions.
- `docs/VERSIONING.md`: the SemVer policy this changelog follows.
- `docs/ARCHITECTURE.md`, `CONTRIBUTING.md`: directory layout, layering
  rule, naming conventions, and the (not-yet-implemented) error-handling
  contract for the layers above the register level.
