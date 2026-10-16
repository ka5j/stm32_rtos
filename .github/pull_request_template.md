## What

<!-- What changes and why. For a stacked pull request, name the one it builds on. -->

## Verification

<!-- What you ran and what it showed: gates, board runs, anything checked by hand. -->

## Checklist

- [ ] One concern; stacked on the pull request below it if it touches the same files
- [ ] `CHANGELOG.md` entry under `[Unreleased]`, or the change is not user-visible
- [ ] Docs updated where behaviour or conventions changed
- [ ] Pre-commit hook passed (`make hooks` once per clone)
- [ ] Firmware change: `make smoke-flash` passes on the board in debug and release; or no firmware change
- [ ] To be merged with a merge commit, never squash
