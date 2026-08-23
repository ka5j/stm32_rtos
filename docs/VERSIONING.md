# Versioning Policy

This project uses [Semantic Versioning](https://semver.org/) (`MAJOR.MINOR.PATCH`).

## Where a version lives

Every official release is a commit on `main` (see [CONTRIBUTING.md](../CONTRIBUTING.md)'s branch-convention section) tagged with an annotated git tag: `vMAJOR.MINOR.PATCH`. The same number is set as `PROJECT_NUMBER` in the [Doxyfile](../Doxyfile), so `make docs` always reports the version of the last tagged release - not of whatever's in progress on `develop`. `PROJECT_NUMBER` is deliberately left blank between releases rather than set ahead of time, so it never claims a version that hasn't actually shipped on `main` yet.

## Pre-1.0 (current phase)

`1.0.0` is reserved for the point this is a functioning RTOS: boots, drivers work, the scheduler preempts and context-switches real tasks, and `api/`/`bsp/`/`rtos/api/` exist and are usable by application code. None of that exists yet - as of this writing `drivers/`, `api/`, `bsp/`, and `rtos/` are still empty scaffolding (see [README.md](../README.md)'s Status section). Until `1.0.0`:

- **MINOR** (`0.X.0`) bumps when a full architectural layer or major milestone lands on `main` - e.g. `0.1.0` is the register layer (`core/inc/`, `device/inc/`) plus the dev pipeline (build, lint, tests, docs, CI) reaching completion. The next expected MINOR bump is whichever layer is next per [docs/ARCHITECTURE.md](ARCHITECTURE.md)'s layer diagram - most likely `drivers/`.
- **PATCH** (`0.1.X`) bumps for a fix or small addition within an already-released layer that doesn't complete a new one - e.g. a bug found in an already-tagged register header, or a missing peripheral added to an already-"done" layer.
- Breaking changes are expected and allowed at any point pre-1.0, per SemVer's own rule for `0.y.z` releases - there's no public API surface yet to break.

## Post-1.0

Once `1.0.0` ships, standard SemVer rules apply:

- **MAJOR** - a breaking change to a public API application code depends on (`api/`/`rtos/api/` function signatures, `DriverStatus_e`'s meaning, task/scheduler behavior a task relies on).
- **MINOR** - a backward-compatible addition (a new peripheral driver, a new RTOS primitive like a queue or semaphore, a new board support target).
- **PATCH** - a bug fix with no API change.

## Cutting a release

1. On the `develop`→`main` PR that constitutes the release, bump `PROJECT_NUMBER` in the [Doxyfile](../Doxyfile) to the new version as part of that PR.
2. Once merged, tag the resulting commit on `main`:
   ```sh
   git checkout main && git pull
   git tag -a vX.Y.Z -m "vX.Y.Z: <one-line summary of what this release contains>"
   git push origin vX.Y.Z
   ```
3. `make docs` from that point forward (until the next release) reports `PROJECT_NUMBER` as the currently-released version, not an in-progress one.
