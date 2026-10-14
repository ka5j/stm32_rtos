# Versioning Policy

This project uses [Semantic Versioning](https://semver.org/) (`MAJOR.MINOR.PATCH`).

## Where a version lives

Every official release is a commit on `main` (see [CONTRIBUTING.md](../CONTRIBUTING.md)'s branch-convention section) tagged with an annotated git tag: `vMAJOR.MINOR.PATCH`. The same number is set as `PROJECT_NUMBER` in the [Doxyfile](../Doxyfile), so `make docs` always reports the version of the last tagged release, not the version in progress on `develop`. It is bumped as part of the `develop`→`main` release PR and then left alone, so between releases it names the last version that actually shipped rather than one in progress — it never claims a version that has not shipped on `main`.

## Pre-1.0 (current phase)

`1.0.0` is reserved for the point at which this is a functioning RTOS: it boots, drivers work, the scheduler preempts and context-switches real tasks, and `api/`/`bsp/`/`rtos/api/` exist and are usable by application code. For where the project actually stands against that bar right now, see [README.md](../README.md)'s Status section, which is the single source of truth for it. Until `1.0.0`:

- **MINOR** (`0.X.0`) bumps when a full architectural layer or major milestone lands on `main`. For example, `0.1.0` marked the completion of the register layer (`core/inc/`, `device/inc/`) plus the development pipeline (build, lint, tests, docs, CI); `0.2.0` marks the completion of the entire `drivers/` layer (GPIO, RCC, Flash, PWR, UART, NVIC, SysTick all implemented and tested), not any single driver file landing.
- **PATCH** (`0.1.X`) bumps for two distinct cases: (1) a fix or small addition within an already-released layer, such as a bug found in an already-tagged register header or a missing peripheral added to an already-completed layer, and (2) incremental progress on a new layer that has not yet fully landed - a single driver file such as `gpio.c` is real, tested, shipped work, but it does not by itself complete `drivers/`, so it stayed PATCH under the `0.1.x` line while the layer was in progress, rather than jumping to MINOR prematurely. MINOR fires only once the layer is genuinely complete per the bullet above.
- Breaking changes are expected and permitted at any point pre-1.0, per SemVer's own rule for `0.y.z` releases, since there is no public API surface yet to break.

## Post-1.0

Once `1.0.0` ships, standard SemVer rules apply:

- **MAJOR** — a breaking change to a public API application code depends on (`api/`/`rtos/api/` function signatures, the meaning of `DriverStatus_e`, task/scheduler behavior a task relies on).
- **MINOR** — a backward-compatible addition (a new peripheral driver, a new RTOS primitive such as a queue or semaphore, a new board support target).
- **PATCH** — a bug fix with no API change.

## Cutting a release

Every release so far (v0.1.4 to v0.1.6) was cut this way; the numbered steps are the whole procedure.

1. **Release branch.** From an up-to-date `develop` whose CI is green and whose `make coverage` is 100%, create `release/vX.Y.Z`. On it:
   - bump `PROJECT_NUMBER` in the [Doxyfile](../Doxyfile) to the new version;
   - move [CHANGELOG.md](../CHANGELOG.md)'s `[Unreleased]` content under a new `## [X.Y.Z] - <date>` heading, merged into one section per kind (Added / Changed / Fixed / Security) as the 0.1.5 release did, leaving `[Unreleased]` empty. Use the UTC date of the tag (the git tag's own date, not the local one, so the changelog and the GitHub Release agree);
   - update the link references at the bottom of the changelog: `[Unreleased]` becomes `.../compare/vX.Y.Z...HEAD`, and a new `[X.Y.Z]: .../compare/vPREV...vX.Y.Z` line goes above the previous one.
   Run `make check-version` before opening the PR: it fails if the Doxyfile and the newest changelog heading disagree. CI runs it on every PR too.
2. **Release PR.** Open `release/vX.Y.Z` into `main` titled `Release vX.Y.Z`, and merge it with a merge commit once `build-and-check` and `Analyze (c-cpp)` pass. Both `main` and `develop` are protected with "require branches to be up to date", so if either moved, update the branch (`gh pr update-branch`) and let CI run again first.
3. **Tag.** Once merged, tag the resulting commit on `main`:
   ```sh
   git checkout main && git pull
   git tag -a vX.Y.Z -m "vX.Y.Z: <one-line summary of what this release contains>"
   git push origin vX.Y.Z
   ```
   Pushing the tag triggers `.github/workflows/release.yml`, which re-checks that the tag, Doxyfile and CHANGELOG agree and that the commit is on `main`, builds the firmware, and publishes the GitHub Release: the changelog section as the notes, with the `.elf`/`.bin`/`.hex`/`.map`, a size report and `SHA256SUMS` attached. If a check fails, nothing is published; fix the cause, delete the tag (`git push origin :refs/tags/vX.Y.Z`) and push it again. The release notes should say plainly what has and has not been verified (for example, that no driver has yet run on hardware) - the README's "Known gap" paragraph is the source.
4. **Sync back.** Open `chore/sync-develop-with-main-vX.Y.Z` into `develop` (merging `main` back, which carries the release commit and tag ancestry) and merge it after CI, so `develop` is never behind the release.
5. From that point forward, until the next release, `make docs` reports `PROJECT_NUMBER` as the currently released version rather than an in-progress one.
