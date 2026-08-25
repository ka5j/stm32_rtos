# Contributing / Conventions

Solo-project conventions for `stm32_rtos`, written down so they stay consistent as the driver/API/BSP/RTOS layers grow. This is the reference for naming, layering, documentation, and the local dev loop — not a guide for external contributors.

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

`rtos/kernel/` and `rtos/api/` are a parallel stack consumed by `app/` alongside `api/`/`bsp/` — see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full diagram. The rule: **a layer may only include headers from itself or the layers below it.** `drivers/` never includes `api/` or `bsp/`. `api/` never reaches past `bsp/`/`drivers/` back into `core/`/`device/` directly. `app/` only includes from `api/` and `rtos/api/`, never `drivers/` or `rtos/kernel/` directly. If a file needs to reach two layers down, that's a sign the layer in between is missing something, not a reason to skip it.

## Naming conventions

**Types** — PascalCase with a suffix indicating category, tag name matches the typedef:
- Structs: `Foo_t` (e.g. `GpioRegisters_t`) — `typedef struct Foo_t { ... } Foo_t;`, not an anonymous `struct { ... }`.
- Enums: `Foo_e` (e.g. `DriverStatus_e`).

**Functions** — camelCase, no inconsistent internal capitalization.

**Macros** — `SCREAMING_SNAKE_CASE`. Register field macros follow `<PERIPH>_<REG>_<FIELD>_Pos` / `_Msk`, matching the pattern already established in `rcc_reg.h`.

**Files** — `<peripheral>_reg.h` for register headers (`core/inc/`, `device/inc/`), plain `<peripheral>.h`/`.c` for driver/API files, no suffix.

**Include guards** — `#ifndef FOO_H` / `#define FOO_H` / `#endif /* FOO_H */` — the trailing comment on `#endif` is required, not optional (register headers already do this; extend it to `drivers/`, `api/`, `bsp/`, `app/`).

## Include-path convention

Use bare filenames resolved via the Makefile's `-I` search paths (e.g. `#include "rcc_reg.h"`), **not** root-relative paths (`#include "device/inc/rcc_reg.h"`). Every `inc/` directory in the project is already on the include path — a root-relative include is redundant and was the reason an earlier commit had to add `.` to `INC_DIRS` unnecessarily. If a bare include doesn't resolve, that's a signal the layering rule above is being violated (you're reaching for a header two layers away), not a reason to fully-qualify the path.

## Documentation (Doxygen)

Every public function needs `@brief`/`@param`/`@return` — and `@return` must enumerate every `DriverStatus_e` value the function can actually return and the condition that produces it, not just "returns a status." Every public struct/enum needs a `@brief`. Preconditions/postconditions that aren't obvious from the signature go in `@pre`/`@post`.

**Gotcha, worth knowing before it surprises you**: `make docs` only enforces coverage on a file *once it has at least one Doxygen comment in it* (e.g. an `@file` block). A file with zero doc comments anywhere is currently invisible to the gate — it won't fail the build, but it also isn't actually being checked. Add the `@file`/`@brief` header to a new file as step one, before you write anything else in it, so the gate is actually watching it from the start rather than after the fact. CI also enforces this directly (see below), so it's not just a local habit — a PR that skips it fails the build either way.

## Error-handling contract

Every driver function that can fail returns `DriverStatus_e` (defined in `drivers/inc/driver_errors.h`) — a small, closed set of failure *categories* (`DRIVER_STATUS_ERR_INVALID_PARAM`, `_NOT_INITIALIZED`, `_TIMEOUT`, `_HW_FAULT`, `_BUSY`, `_UNSUPPORTED`), used for control-flow decisions at the call site. **This is the required pattern for any new driver code, not something already implemented** — `driver_errors.h` doesn't exist on `develop` yet; the first driver work should create it with this exact enum before anything returns it. Always compare explicitly:

```c
DriverStatus_e status = rccGpioClockEnable(GPIO_PORT_A);
if (status != DRIVER_STATUS_OK) { /* handle */ }
```

Never `if (status)` / `if (!status)` — that relies on `OK == 0` implicitly instead of stating the comparison, and breaks silently if the enum is ever reordered.

Module-specific failure detail (which check failed, on which peripheral) belongs in a per-module detail enum, not the shared status enum — don't add peripheral-specific values to `DriverStatus_e` itself. See the tier-2 design (per-module detail codes returned alongside status) once a driver actually needs that granularity; don't build it speculatively before then.

Mark fallible functions `__attribute__((warn_unused_result))` so an ignored return value is a compiler error, not a silent bug.

## CI-enforced checks on new/modified files

Two checks run in `ci.yml` that aren't obvious from running `make docs`/`make test` locally — both work the same way: they diff the PR against its base branch, so they only run on `pull_request` events (a plain push has no "base" to compare against), and they apply identically regardless of target branch — a feature branch's PR into `develop` and a `develop`→`main` PR are checked the same way. Both check *modified* files, not just newly-added ones, since several files in this repo started as empty scaffolding in the initial commit, so filling one in is a modification, not a new file.

1. **Doxygen `@file` block** — every source file touched under `core/inc/`, `device/inc/`, `drivers/`, `api/`, `bsp/`, `rtos/`, or `app/src/` needs one, or `make docs` can't check it at all (the gotcha above).
2. **Matching unit test** — every `.c` file touched under `drivers/`, `api/`, `bsp/`, or `rtos/` (except `app/src/main.c`, the entry point) needs a `tests/unit/test_<name>.c`. Every `*_reg.h` touched under `core/inc`/`device/inc` must be `#include`d by at least one file in `tests/unit/` — CI doesn't enforce a 1:1 naming rule there, but this repo's convention is one test file per register header (`test_<peripheral>_reg.c`, e.g. `gpio_reg.h` → `test_gpio_reg.c`). All of them are aggregated into a single `run_tests` binary by `tests/unit/test_runner.c`, which owns `main()`/`setUp()`/`tearDown()` — Unity allows only one definition of each per binary, so the individual test files declare no `main()` of their own.

## CI triggers and branch conventions

`ci.yml` runs on every `push` (any branch) and every `pull_request` (any target branch): `make all`, `make format-check`, `make test`, `make lint`, `make docs`, in that order — the same five checks the pre-commit hook already ran locally. On `pull_request` events specifically, the two diff-based checks above also run, since only a PR has a base branch to diff against.

`hil.yml` is separate and much narrower: it only runs on `workflow_dispatch` (manual), on a self-hosted runner physically wired to a Nucleo-F446RE (`make all` → `make flash` → a hardware smoke test). It deliberately never runs on `pull_request` or `push` (even to `main`) — a self-hosted runner executing automatically-triggered workflows on a public repo would run arbitrary fork/push code on that physical machine, so triggering it is always a manual, deliberate act from the Actions tab.

`pages.yml` is also `main`-only for a related but different reason: it builds `make docs` and publishes `docs/html/` to GitHub Pages (<https://ka5j.github.io/stm32_rtos/>), so the live site always reflects the last tagged release (`PROJECT_NUMBER` is only ever bumped on a `develop`→`main` release PR — see [docs/VERSIONING.md](docs/VERSIONING.md)), not in-progress `develop` work, and a PR from an untrusted fork can't publish to it. Nothing under `docs/html/` is ever committed — `pages.yml` rebuilds it fresh in the runner every time and hands it to GitHub's Pages deploy action directly.

`codeql.yml` runs GitHub CodeQL static analysis on every push/PR to `main`/`develop` and weekly on a schedule. `.github/dependabot.yml` watches the `github-actions` ecosystem, since this project's actual supply-chain surface is the third-party Actions the workflows use (all pinned to commit SHAs, not mutable tags), not a C package manager - there isn't one. See [SECURITY.md](SECURITY.md) for the full trust-boundary reasoning behind all four workflows and the pinning practice.

**Branch convention:** feature/topic branches PR into `develop`. `develop` is PR'd into `main` occasionally, at release points — not on every merge — which is also why hardware-in-the-loop only fires on `main`, not on every `develop` commit. See [docs/VERSIONING.md](docs/VERSIONING.md) for what makes something release-worthy and how the version number is chosen.

## Formatting

`.clang-format` is enforced via `make format-check` (CI + pre-commit hook) — don't hand-format. Run `make format` to auto-fix before committing.

## Local dev loop

One-time per clone:
```sh
git config core.hooksPath .githooks
chmod +x .githooks/pre-commit
```

After that, `git commit` runs `format-check` → `test` → `lint` → `docs` → `all` automatically before the commit is created — same checks CI runs remotely, just local and fast. Useful standalone commands: `make format`, `make format-check`, `make test`, `make lint`, `make docs`, `make all`, `make flash`.
