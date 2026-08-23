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

## Error-handling contract

Every driver function that can fail returns `DriverStatus_e` (`drivers/inc/driver_errors.h`) — a small, closed set of failure *categories* (`DRIVER_STATUS_ERR_INVALID_PARAM`, `_NOT_INITIALIZED`, `_TIMEOUT`, `_HW_FAULT`, `_BUSY`, `_UNSUPPORTED`), used for control-flow decisions at the call site. Always compare explicitly:

```c
DriverStatus_e status = rccGpioClockEnable(GPIO_PORT_A);
if (status != DRIVER_STATUS_OK) { /* handle */ }
```

Never `if (status)` / `if (!status)` — that relies on `OK == 0` implicitly instead of stating the comparison, and breaks silently if the enum is ever reordered.

Module-specific failure detail (which check failed, on which peripheral) belongs in a per-module detail enum, not the shared status enum — don't add peripheral-specific values to `DriverStatus_e` itself. See the tier-2 design (per-module detail codes returned alongside status) once a driver actually needs that granularity; don't build it speculatively before then.

Mark fallible functions `__attribute__((warn_unused_result))` so an ignored return value is a compiler error, not a silent bug.

## Documentation (Doxygen)

Every public function needs `@brief`/`@param`/`@return` — and `@return` must enumerate every `DriverStatus_e` value the function can actually return and the condition that produces it, not just "returns a status." Every public struct/enum needs a `@brief`. Preconditions/postconditions that aren't obvious from the signature go in `@pre`/`@post`.

**Gotcha, worth knowing before it surprises you**: `make docs` only enforces coverage on a file *once it has at least one Doxygen comment in it* (e.g. an `@file` block). A file with zero doc comments anywhere is currently invisible to the gate — it won't fail the build, but it also isn't actually being checked. Add the `@file`/`@brief` header to a new file as step one, before you write anything else in it, so the gate is actually watching it from the start rather than after the fact.

## Formatting

`.clang-format` is enforced via `make format-check` (CI + pre-commit hook) — don't hand-format. Run `make format` to auto-fix before committing.

## Local dev loop

One-time per clone:
```sh
git config core.hooksPath .githooks
chmod +x .githooks/pre-commit
```

After that, `git commit` runs `format-check` → `lint` → `docs` → `all` automatically before the commit is created — same checks CI runs remotely, just local and fast. Useful standalone commands: `make format`, `make format-check`, `make lint`, `make docs`, `make all`, `make flash`.
