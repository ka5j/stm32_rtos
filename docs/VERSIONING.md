# Versioning

The project uses [Semantic Versioning](https://semver.org/). A release is
a commit on `main` with an annotated tag `vMAJOR.MINOR.PATCH`. The Doxyfile's
`PROJECT_NUMBER` names the last release and changes only in a release pull
request, so the docs never claim an unreleased version.

## Before 1.0.0

`1.0.0` means a working RTOS: it boots, the drivers run on hardware, the
scheduler preempts real tasks, and `api/`, `bsp/` and `rtos/api/` are
usable by application code. Until then:

- **MINOR** (`0.X.0`) marks a completed layer or milestone. `0.1.0` was the
  register layer and tooling; `0.2.0` is reserved for a completed driver
  layer.
- **PATCH** (`0.1.X`) covers fixes, small additions, and progress on a
  layer that is not finished yet.
- Breaking changes are allowed at any time, as SemVer permits for `0.y.z`.

## From 1.0.0

- **MAJOR:** a breaking change to something application code relies on,
  such as `api/` or `rtos/api/` signatures, the meaning of
  `DriverStatus_e`, or scheduler behaviour.
- **MINOR:** a backward-compatible addition, such as a new driver, RTOS
  primitive or board.
- **PATCH:** a bug fix with no API change.

## Cutting a release

1. **Branch.** From an up-to-date `develop` with green CI, create
   `release/vX.Y.Z`. On it:
   - set `PROJECT_NUMBER` in the Doxyfile;
   - rename `CHANGELOG.md`'s `[Unreleased]` to `## [X.Y.Z] - <UTC date>`,
     with one section per kind (Added, Changed, Fixed, Security), and add an
     empty `[Unreleased]` above it;
   - open the section with what has **not** been verified, for example
     whether the code has run on hardware;
   - update the compare links at the bottom of the changelog;
   - run `make check-version`.
2. **Pull request.** Open `release/vX.Y.Z` into `main` titled
   `Release vX.Y.Z`. Merge with a merge commit once CI passes. If `main`
   moved, run `gh pr update-branch` first; both branches require being up to
   date.
3. **Tag.** On `main`:
   ```sh
   git checkout main && git pull
   git tag -a vX.Y.Z -m "vX.Y.Z: <one-line summary>"
   git push origin vX.Y.Z
   ```
   `release.yml` checks that the tag, Doxyfile and changelog agree and that
   the commit is on `main`, builds the firmware, and publishes the GitHub
   Release with the changelog section as notes. If a check fails nothing is
   published: fix it, then run `git push origin :refs/tags/vX.Y.Z` and push
   the tag again.
4. **Sync.** Merge `main` back into `develop` through
   `chore/sync-develop-with-main-vX.Y.Z`.
