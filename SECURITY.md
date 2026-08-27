# Security Policy

## Reporting a vulnerability

Open a [GitHub Security Advisory](https://github.com/ka5j/stm32_rtos/security/advisories/new) for this repository rather than a public issue, so a fix can be prepared before details become public. This is a solo project with no dedicated security team and no formal SLA; reports are reviewed and taken seriously, and a fix (or an explanation of why an issue is not exploitable) will follow within a reasonable time.

## Supported versions

Pre-`1.0.0` (see [docs/VERSIONING.md](docs/VERSIONING.md)), only the most recently tagged release on `main` is supported. There is no backport policy for older `0.y.z` tags at this stage, since breaking changes are expected and permitted at any point before `1.0.0`. Once `1.0.0` ships, this section will define which major version lines continue to receive fixes.

## Threat model

This is bare-metal firmware with no network stack, no third-party dependencies, and, until the `drivers`/`api`/`bsp`/`rtos` layers are implemented (see [README.md](README.md)'s Status section), no code that parses untrusted input. The current attack surface is the register-definition layer (`core/inc/`, `device/inc/`), the build tooling, and the CI/CD pipeline itself, not runtime firmware behavior, since no firmware logic beyond an empty `while(1)` currently executes. This will change materially once UART or other input-handling drivers exist; this document should be revisited at that point rather than treated as a one-time checklist.

An incorrect register definition (wrong offset, wrong bit position) is a correctness bug, caught by the size/offset/field assertions in `tests/unit/`, not a vulnerability in the conventional sense, since no untrusted input reaches it. This distinction is why the policy is scoped as it is rather than treating every bug report as a security report.

## CI/CD trust boundaries

- **`ci.yml`** runs on every push and pull request, including from forks. It only builds, lints, tests, and generates docs on a GitHub-hosted, ephemeral runner with `permissions: contents: read`; nothing it does is privileged or persists past the job.
- **`hil.yml`** (hardware-in-the-loop) runs on a self-hosted runner physically connected to real hardware. It is triggered only by `workflow_dispatch`, never by `push` or `pull_request`. A self-hosted runner executing automatically triggered workflows on a public repository would allow an untrusted push or fork PR to run arbitrary code on that physical machine; requiring a manual, deliberate trigger prevents this.
- **`pages.yml`** deploys only from `main`, never from a pull request or fork, so the published documentation site (<https://ka5j.github.io/stm32_rtos/>) cannot be hijacked by an untrusted branch. It also reflects only a tagged release, not in-progress `develop` work, since `PROJECT_NUMBER` is bumped only on a `develop`→`main` release PR.
- **`codeql.yml`** runs GitHub CodeQL static analysis on every push/PR to `main`/`develop`, plus a weekly schedule, with `permissions: contents: read, security-events: write`, sufficient to read the repository and upload findings.

## Action pinning

Every third-party GitHub Action used in this repository's workflows is pinned to a full commit SHA rather than a mutable version tag (`actions/checkout@11d5960...` with a `# v4.4.0` comment for readability, not `actions/checkout@v4`). A tag can be force-moved to reference different, potentially malicious code without any visible change in this repository's history; a commit SHA cannot be silently repointed in the same way. No automated dependency-update tooling watches these pins; bumping them is a manual step, performed by checking an action's release notes on GitHub and updating the SHA and version comment together. Pins remain safer than tracking a mutable tag even without automated bumping, but they stay current only if someone actually revisits them; this does not self-maintain.
