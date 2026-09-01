# Changelog

All notable changes to this project are documented here. Format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); version numbers
follow this project's own [Semantic Versioning policy](docs/VERSIONING.md),
which explains what MAJOR/MINOR/PATCH mean pre-1.0 for this repository
specifically.

## [Unreleased]

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
