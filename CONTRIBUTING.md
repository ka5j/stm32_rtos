# Contributing / Conventions

Solo-project conventions for `stm32_rtos`, documented to remain consistent as the driver/API/BSP/RTOS layers are developed. This is the reference for naming, layering, documentation, and the local development workflow; it is not a guide for external contributors.

## Layering rule

```
core/, device/  (register structs)
      ↓
   drivers/       (register-level driver logic)
      ↓
    api/          (peripheral-facing API: led_on(), debug_print())
      ↓
    bsp/          (board pin/peripheral mapping, consumed by api/)
      ↓
    app/          (task code)
```

`rtos/kernel/` and `rtos/api/` form a parallel stack consumed by `app/` alongside `api/`/`bsp/` — see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the complete diagram. The rule: **a layer may include headers only from itself or the layers below it.** `drivers/` never includes `api/` or `bsp/`. `api/` never reaches past `bsp/`/`drivers/` back into `core/`/`device/` directly. `app/` includes only from `api/` and `rtos/api/`, never `drivers/` or `rtos/kernel/` directly. If a file needs to reach two layers down, that indicates the intermediate layer is missing functionality; it is not a reason to bypass it.

## Naming conventions

**Types** — PascalCase with a suffix indicating category; the tag name matches the typedef:
- Structs: `Foo_t` (e.g. `GpioRegisters_t`) — `typedef struct Foo_t { ... } Foo_t;`, not an anonymous `struct { ... }`.
- Enums: `Foo_e` (e.g. `DriverStatus_e`).

**Functions** — camelCase, with consistent internal capitalization.

**Macros** — `SCREAMING_SNAKE_CASE`. Register field macros follow `<PERIPH>_<REG>_<FIELD>_Pos` / `_Msk`, matching the pattern established in `rcc_reg.h`.

**Files** — `<peripheral>_reg.h` for register headers (`core/inc/`, `device/inc/`); plain `<peripheral>.h`/`.c`, with no suffix, for driver/API files.

**Include guards** — `#ifndef FOO_H` / `#define FOO_H` / `#endif /* FOO_H */`. The trailing comment on `#endif` is required, not optional. Register headers already follow this convention; it extends to `drivers/`, `api/`, `bsp/`, and `app/`.

## Include-path convention

Use bare filenames resolved via the Makefile's `-I` search paths (e.g. `#include "rcc_reg.h"`), **not** root-relative paths (`#include "device/inc/rcc_reg.h"`). Every `inc/` directory in the project is already on the include path; a root-relative include is redundant and was the reason an earlier commit had to add `.` to `INC_DIRS` unnecessarily. If a bare include fails to resolve, this indicates a violation of the layering rule above (a header two layers away is being referenced directly), not a reason to fully qualify the path.

## Documentation (Doxygen)

Every public function requires `@brief`/`@param`/`@return`. `@return` must enumerate every `DriverStatus_e` value the function can return and the condition producing each, not merely state that a status is returned. Every public struct/enum requires a `@brief`. Preconditions and postconditions not evident from the signature belong in `@pre`/`@post`.

**Coverage caveat**: `make docs` enforces coverage on a file only once it contains at least one Doxygen comment (e.g. an `@file` block). A file with no doc comments is invisible to the gate — it will not fail the build, but it is also not being checked. Add the `@file`/`@brief` header to a new file as the first step, before adding other content, so the gate covers the file from the outset. CI enforces this directly as well (see below); a pull request that omits it fails the build regardless of local practice.

**Topics grouping**: the generated documentation site (Doxygen's "Topics" page) is organized by architectural layer rather than raw file listing. [docs/groups.dox](docs/groups.dox) declares the group hierarchy and mirrors [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)'s layer diagram exactly, including groups for layers that do not yet exist (declared there as a naming reference, not yet instantiated as live `@defgroup`s). When a new layer receives its first source file (e.g. `drivers/inc/gpio.h`), add that layer's `@defgroup` to `groups.dox` using the name already reserved for it, then wrap the new file's struct/macro/function definitions in `@addtogroup <group_id>` / `@{` ... `@}` so they join the group. Do not repeat `@ingroup` on every individual member.

Within `core/inc/`/`device/inc/`, grouping goes one level deeper: `core_peripherals`/`device_peripherals` are parent groups, and every individual peripheral gets its own `@defgroup <peripheral>_registers` nested under the appropriate parent (`@ingroup core_peripherals` or `@ingroup device_peripherals`) - e.g. `mpu_registers`, `gpio_registers`. A register header wraps its content in `@addtogroup <peripheral>_registers`, not the broader parent group directly. If a single header defines more than one architecturally distinct register block (e.g. `scb_reg.h` defines both `ScbRegisters_t` and the separate `FpuRegisters_t`), give each block its own subgroup and its own `@addtogroup` region within the file - open, close, and reopen as needed at the natural boundaries between each block's content; do not lump architecturally distinct peripherals into one group just because they share a file. See `core/inc/scb_reg.h` for the pattern (`scb_registers` and `fpu_registers` interleaved in one file) and any other `core/inc`/`device/inc` header for the single-peripheral case.

## Error-handling contract

Every driver function that can fail returns `DriverStatus_e` (defined in `drivers/inc/driver_status.h`), a small, closed set of failure *categories* (`DRIVER_STATUS_ERR_INVALID_PARAM`, `_NOT_INITIALIZED`, `_TIMEOUT`, `_HW_FAULT`, `_BUSY`, `_UNSUPPORTED`) used for control-flow decisions at the call site. This is the required pattern for all new driver code. Compare explicitly:

```c
DriverStatus_e status = rccGpioClockEnable(GPIO_PORT_A);
if (status != DRIVER_STATUS_OK) { /* handle */ }
```

Do not use `if (status)` / `if (!status)`; this relies implicitly on `OK == 0` rather than an explicit comparison, and breaks silently if the enum is reordered.

Module-specific failure detail (which check failed, on which peripheral) belongs in a per-module detail enum, not the shared status enum. Do not add peripheral-specific values to `DriverStatus_e` itself. Introduce the tier-2 design (per-module detail codes returned alongside status) once a driver requires that granularity; do not build it speculatively before then.

Mark fallible functions `__attribute__((warn_unused_result))` so an ignored return value is a compiler error rather than a silent bug.

## CI-enforced checks on new/modified files

Two checks run in `ci.yml` that are not evident from running `make docs`/`make test` locally. Both diff the pull request against its base branch, so they run only on `pull_request` events (a plain push has no base to compare against), and both apply identically regardless of target branch — a feature branch's PR into `develop` and a `develop`→`main` PR are checked the same way. Both checks cover *modified* files, not only newly added ones, since several files in this repository began as empty scaffolding in the initial commit; populating one is a modification, not a new file.

1. **Doxygen `@file` block** — every source file touched under `core/inc/`, `device/inc/`, `drivers/`, `api/`, `bsp/`, `rtos/`, or `app/src/` requires one, or `make docs` cannot check it at all (see the coverage caveat above).
2. **Matching unit test** — every `.c` file touched under `drivers/`, `api/`, `bsp/`, or `rtos/` (except `app/src/main.c`, the entry point) requires a `tests/unit/test_<name>.c`. Every `*_reg.h` touched under `core/inc`/`device/inc` must be `#include`d by at least one file in `tests/unit/`. CI does not enforce a 1:1 naming rule for this, but the repository convention is one test file per register header (`test_<peripheral>_reg.c`, e.g. `gpio_reg.h` → `test_gpio_reg.c`). All test files are aggregated into a single `run_tests` binary by `tests/unit/test_runner.c`, which owns `main()`/`setUp()`/`tearDown()`; Unity permits only one definition of each per binary, so individual test files declare no `main()` of their own.

## CI triggers and branch conventions

`ci.yml` runs on every `push` (any branch) and every `pull_request` (any target branch): `make all`, `make format-check`, `make test`, `make coverage`, `make lint`, `make docs`, in that order. `make coverage` is CI-only, not part of the pre-commit hook's five checks - it recompiles `TEST_DRIVER_SOURCES` with instrumentation, a real (if fast) extra compile pass, so it stays out of the hook's every-commit path and lives only in CI. On `pull_request` events specifically, the two diff-based checks above also run, since only a pull request has a base branch to diff against.

`hil.yml` is separate and considerably narrower: it runs only on `workflow_dispatch` (manual trigger), on a self-hosted runner physically connected to a Nucleo-F446RE (`make all` → `make flash` → a hardware smoke test). It never runs on `pull_request` or `push`, including to `main`. A self-hosted runner executing automatically triggered workflows on a public repository would allow arbitrary fork or push code to execute on that physical machine; triggering is therefore always a manual, deliberate action from the Actions tab.

`pages.yml` is also restricted to `main`, for a related but distinct reason: it builds `make docs` and publishes `docs/html/` to GitHub Pages (<https://ka5j.github.io/stm32_rtos/>), so the published site always reflects the last tagged release (`PROJECT_NUMBER` is bumped only on a `develop`→`main` release PR — see [docs/VERSIONING.md](docs/VERSIONING.md)), not in-progress `develop` work, and a pull request from an untrusted fork cannot publish to it. Nothing under `docs/html/` is committed; `pages.yml` rebuilds it in the runner on every execution and passes it to GitHub's Pages deploy action directly.

`codeql.yml` runs GitHub CodeQL static analysis on every push/PR to `main`/`develop` and weekly on a schedule. This project's actual supply-chain surface is the third-party Actions the workflows use, all pinned to commit SHAs rather than mutable tags, not a C package manager, since none is used. These pins are bumped manually. See [SECURITY.md](SECURITY.md) for the complete trust-boundary reasoning behind all four workflows and the pinning practice.

**Branch convention:** feature/topic branches are merged via pull request into `develop`. `develop` is merged into `main` at release points, not on every merge, which is also why hardware-in-the-loop testing fires only on `main`, not on every `develop` commit. See [docs/VERSIONING.md](docs/VERSIONING.md) for release criteria and version numbering.

## Formatting

`.clang-format` is enforced via `make format-check` (CI and pre-commit hook); do not hand-format. Run `make format` to apply formatting automatically before committing.

## Local development workflow

One-time setup per clone:
```sh
git config core.hooksPath .githooks
chmod +x .githooks/pre-commit
```

After setup, `git commit` runs `format-check` → `test` → `lint` → `docs` → `all` automatically before the commit is created, matching the checks CI runs remotely. Standalone commands: `make format`, `make format-check`, `make test`, `make lint`, `make docs`, `make all`, `make flash`.
