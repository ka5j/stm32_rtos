# Versioning Policy

This project uses [Semantic Versioning](https://semver.org/) (`MAJOR.MINOR.PATCH`).

## Where a version lives

Every official release is a commit on `main` (see [CONTRIBUTING.md](../CONTRIBUTING.md)'s branch-convention section) tagged with an annotated git tag: `vMAJOR.MINOR.PATCH`. The same number is set as `PROJECT_NUMBER` in the [Doxyfile](../Doxyfile), so `make docs` always reports the version of the last tagged release, not the version in progress on `develop`. `PROJECT_NUMBER` is deliberately left blank between releases rather than set in advance, so it never claims a version that has not actually shipped on `main`.

## Pre-1.0 (current phase)

`1.0.0` is reserved for the point at which this is a functioning RTOS: it boots, drivers work, the scheduler preempts and context-switches real tasks, and `api/`/`bsp/`/`rtos/api/` exist and are usable by application code. As of this writing, `api/`, `bsp/`, and `rtos/` remain empty scaffolding, and `drivers/` has its error-status contract (`drivers/inc/driver_status.h`), a GPIO driver (`drivers/inc/gpio.h`, `drivers/src/gpio.c`), and RCC peripheral clock gating (`drivers/inc/rcc.h`, `drivers/src/rcc.c`) - RCC's SYSCLK/PLL bring-up and the UART/NVIC/SysTick drivers are not yet implemented - see [README.md](../README.md)'s Status section. Until `1.0.0`:

- **MINOR** (`0.X.0`) bumps when a full architectural layer or major milestone lands on `main`. For example, `0.1.0` marked the completion of the register layer (`core/inc/`, `device/inc/`) plus the development pipeline (build, lint, tests, docs, CI); `0.2.0` is reserved for the completion of the entire `drivers/` layer (GPIO, RCC, UART, NVIC, SysTick all implemented and tested), not any single driver file landing.
- **PATCH** (`0.1.X`) bumps for two distinct cases: (1) a fix or small addition within an already-released layer, such as a bug found in an already-tagged register header or a missing peripheral added to an already-completed layer, and (2) incremental progress on a new layer that has not yet fully landed - a single driver file such as `gpio.c` is real, tested, shipped work, but it does not by itself complete `drivers/`, so it stays PATCH under the still-in-progress `0.1.x` line rather than jumping to MINOR prematurely. MINOR fires only once the layer is genuinely complete per the bullet above.
- Breaking changes are expected and permitted at any point pre-1.0, per SemVer's own rule for `0.y.z` releases, since there is no public API surface yet to break.

## Post-1.0

Once `1.0.0` ships, standard SemVer rules apply:

- **MAJOR** — a breaking change to a public API application code depends on (`api/`/`rtos/api/` function signatures, the meaning of `DriverStatus_e`, task/scheduler behavior a task relies on).
- **MINOR** — a backward-compatible addition (a new peripheral driver, a new RTOS primitive such as a queue or semaphore, a new board support target).
- **PATCH** — a bug fix with no API change.

## Cutting a release

1. On the `develop`→`main` PR that constitutes the release, bump `PROJECT_NUMBER` in the [Doxyfile](../Doxyfile) to the new version as part of that PR, and move [CHANGELOG.md](../CHANGELOG.md)'s `[Unreleased]` section content under a new `## [X.Y.Z] - <date>` heading (leaving `[Unreleased]` empty for whatever comes next).
2. Once merged, tag the resulting commit on `main`:
   ```sh
   git checkout main && git pull
   git tag -a vX.Y.Z -m "vX.Y.Z: <one-line summary of what this release contains>"
   git push origin vX.Y.Z
   ```
3. From that point forward, until the next release, `make docs` reports `PROJECT_NUMBER` as the currently released version rather than an in-progress one.
