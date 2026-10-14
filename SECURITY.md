# Security

## Reporting a vulnerability

Open a [GitHub Security Advisory](https://github.com/ka5j/stm32_rtos/security/advisories/new)
rather than a public issue. This is a solo project with no formal response
time, but every report gets a fix or an explanation.

Only the latest release on `main` is supported before `1.0.0`.

## Threat model

The firmware has no network stack and no third-party code.

- **UART reads:** the UART driver can read bytes from a physical line, but
  nothing in the built image calls it. `uartReceive()` copies raw bytes into
  a caller-sized buffer and never interprets them.
- **When this changes:** untrusted input first exists when something parses
  received data, such as a command parser or a bootloader protocol. Revisit
  this section then.
- **Register definitions:** a wrong register definition is a correctness
  bug, caught by the offset and field tests, not a vulnerability.

## CI/CD trust boundaries

| Workflow      | Runs on                                   | Permissions                     |
| ------------- | ----------------------------------------- | ------------------------------- |
| `ci.yml`      | every PR (forks included), push to `main`/`develop` | `contents: read`      |
| `codeql.yml`  | PR and push to `main`/`develop`, weekly   | `contents: read`, `security-events: write` |
| `pages.yml`   | push to `main`, manual dispatch           | `pages: write`, `id-token: write` |
| `release.yml` | `vX.Y.Z` tag push; dry run on PRs         | `contents: write` on its publishing job only |

The `github-pages` environment accepts deployments only from `main`, so a
manual run on another branch cannot publish. `codeql.yml` can also be run
manually.

- **Runners:** every workflow runs on ephemeral GitHub-hosted runners.
  There is no self-hosted runner. The board is attached to the development
  machine, and no GitHub-triggered job reaches hardware. Do not add a
  self-hosted runner casually.
- **Merge gates:** `main` and `develop` require `build-and-check` and
  `Analyze (c-cpp)` to pass.
- **release.yml:**
  - it refuses a tag whose commit is not on `main`, or whose version
    disagrees with the Doxyfile and changelog;
  - the tag name reaches the shell only through an environment variable.

## Supply chain

- **Actions** are pinned to full commit SHAs, with a comment naming the
  exact release (for example `# v7.0.1`, never a floating `# v7`). Bump the
  SHA and the comment together, by hand, after reading the release notes.
- **Arm GNU Toolchain and Doxygen** are downloaded from upstream and checked
  against a SHA256 pinned in `.github/actions/` before extraction. The
  cache key includes the checksum, so changing the pin invalidates the
  cache. A restored cache is trusted, and only this repository's own runs
  can write one.
- **Apt and PyPI tools:**
  - `cppcheck`, `clang-format` and `gcovr` are pinned by version, not hash,
    and rely on their package manager's signing;
  - Graphviz is unpinned because it only draws diagrams and cannot change
    what CI enforces.
