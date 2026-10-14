# Contributing

Conventions and workflow for `stm32_rtos`. The reasons behind them are in
[Design notes](docs/DESIGN.md).

## Layering

A layer includes headers only from itself and the layers below it (see the
[layer diagram](docs/ARCHITECTURE.md#layers)). `drivers/` never includes
`bsp/` or `api/`; `app/` includes only `api/` and `rtos/api/`. If code needs
to reach two layers down, the layer in between is missing something; add it
there.

Include headers by bare name (`#include "rcc_reg.h"`). Every `inc/`
directory is on the include path. Never use a root-relative path.

## Naming

| Kind            | Convention                                    | Example                    |
| --------------- | --------------------------------------------- | -------------------------- |
| Struct          | `Name_t`, tag equal to typedef                | `typedef struct GpioRegisters_t {...} GpioRegisters_t;` |
| Enum            | `Name_e`                                      | `DriverStatus_e`           |
| Function        | camelCase, module prefix                      | `rccPllConfig()`           |
| Macro           | `SCREAMING_SNAKE_CASE`                        | `RCC_CR_HSEON`             |
| Register field  | `<PERIPH>_<REG>_<FIELD>`, plus `_Pos`/`_Msk`  | `RCC_CFGR_SW_Msk`          |
| Register header | `<peripheral>_reg.h` in `core/inc`, `device/inc` | `gpio_reg.h`            |
| Include guard   | `#ifndef FOO_H` ... `#endif /* FOO_H */`      |                            |

Register bit and mask macros cast before shifting, `((uint32_t)1U << n)`
(see [MISRA deviations](docs/DESIGN.md#misra-c2012-deviations)).

## Drivers

- Take the peripheral's register block as a parameter.
- Return `DriverStatus_e` where the call can fail, `void` where it cannot.
- Make a register parameter `const` only if the call leaves the peripheral
  unchanged.

See [Register blocks as parameters](docs/DESIGN.md#register-blocks-as-parameters),
[Error contract](docs/DESIGN.md#error-contract) and
[const register parameters](docs/DESIGN.md#const-register-parameters).

## Documentation (Doxygen)

`make docs` fails on any warning, including a missing `@param`. Each new
file starts with a `@file` block. Without one Doxygen does not check the
file at all, and CI rejects the pull request.

**Functions**
- `@brief`: one line saying what the function does.
- `@param` for every parameter.
- `@retval` for each `DriverStatus_e` value returned, with its condition.
  Use `@return` only for non-status results.
- `@pre` and `@post` for ordering contracts.
- `@warning` for hazards a caller can trip over.
- `@note` sparingly.

**Header content**
- Each driver header gives one `@code` example of the usual call sequence.

**Text**
- Details cover only what a caller cannot infer from the signature.
- Link to Design notes with `@see` instead of restating the reasoning.
- No indented prose. Markdown treats four spaces as a code block. Use `-`
  or `1.` lists.

**Grouping**
- Each file joins its group with `@addtogroup <id>` / `@{` ... `@}` around
  its definitions. Groups are declared in `docs/groups.dox`, one per layer
  and one per peripheral.

**Register headers**
- Group each register's macros with `@name <REG> (offset 0xNN)` and
  `@{` ... `@}`.
- Give each macro's `///<` comment its bits and its access type, as
  `\regaccess{rw}`, `ro`, `wo`, `rc_w0` or `rc_w1`.

## Tests

Every driver, BSP or API `.c` has a `tests/unit/test_<name>.c`, and every
register header is included by a test.

- `tests/unit/test_runner.c` owns `main()` and must declare and `RUN_TEST()`
  every test. `make test` fails if one is missing.
- Drivers and the BSP must stay at 100% line and branch coverage. Read
  [Host testing and its limits](docs/DESIGN.md#host-testing-and-its-limits)
  before excluding anything.

## Workflow

One-time setup: `make hooks`. The pre-commit hook then runs
`format-check`, `test`, `lint`, `docs` and `all` on the working tree
(not just what is staged).

| Command                   | Use                                                        |
| ------------------------- | ---------------------------------------------------------- |
| `make format`             | Apply `.clang-format`. Never hand-format.                  |
| `make lint`               | cppcheck + MISRA. Run `tools/install_cppcheck.sh` once for CI's version. |
| `make test SANITIZE=1`    | The suite under ASan and UBSan, as CI runs it.             |
| `make coverage`           | 100% line and branch gate (CI only, not the hook).         |
| `make check-pr`           | CI's pull-request file checks, against `BASE` (default `origin/develop`). |

The full command list is in the [README](README.md#make-commands).

## CI

`ci.yml` runs on every pull request and on pushes to `main` and `develop`.

- **Build:** debug and release builds.
- **Checks:** format, test (plain and sanitized), coverage, lint, docs,
  version, and the pull-request file checks.
- **Other workflows:**
  - `codeql.yml` runs static analysis;
  - `pages.yml` publishes the docs from `main`;
  - `release.yml` publishes a release from a `vX.Y.Z` tag.

The trust boundaries for each workflow are described in
[SECURITY.md](SECURITY.md).

## Branches and pull requests

- Work on a topic branch and merge it into `develop` by pull request.
- Split unrelated changes into separate pull requests. When they touch the
  same files, stack them and merge bottom-up with merge commits. Never
  squash: squashing the bottom of a stack disconnects the rest.
- `develop` reaches `main` only through a release pull request (see
  [Versioning](docs/VERSIONING.md)).
- Add a `CHANGELOG.md` entry under `[Unreleased]` for every user-visible
  change.
