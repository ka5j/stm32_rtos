# Security Policy

## Reporting a vulnerability

Open a [GitHub Security Advisory](https://github.com/ka5j/stm32_rtos/security/advisories/new) for this repo rather than a public issue, so a fix can land before details go public. This is a solo project with no dedicated security team and no SLA, but reports are read and taken seriously, and a fix (or an explanation of why something isn't exploitable) should follow in a reasonable time.

## Supported versions

Pre-`1.0.0` (see [docs/VERSIONING.md](docs/VERSIONING.md)), only the most recently tagged release on `main` is supported - there's no backport policy for older `0.y.z` tags at this stage, since breaking changes are expected and allowed at any point before `1.0.0`. Once `1.0.0` ships, this section will define which MAJOR lines still receive fixes.

## Threat model

This is bare-metal firmware with no network stack, no third-party dependencies, and - until the `drivers`/`api`/`bsp`/`rtos` layers are written (see [README.md](README.md)'s Status section) - no code that parses untrusted input. The attack surface today is the register-definition layer (`core/inc/`, `device/inc/`), the build tooling, and the CI/CD pipeline itself - not runtime firmware behavior, since there's no running firmware logic yet beyond an empty `while(1)`. That will materially change once UART or other input-handling drivers exist; this document should be revisited then, not treated as a one-time checkbox.

A wrong register definition (wrong offset, wrong bit position) is a correctness bug, caught by the size/offset/field assertions in `tests/unit/` - not a vulnerability in the conventional sense, since nothing untrusted reaches it. That distinction is why this policy is scoped the way it is rather than treating every bug report as a security report.

## CI/CD trust boundaries

- **`ci.yml`** runs on every push and pull request, including from forks. It only builds/lints/tests/docs on a GitHub-hosted, ephemeral runner with `permissions: contents: read` - nothing it does is privileged or persists past the job.
- **`hil.yml`** (hardware-in-the-loop) runs on a self-hosted runner physically wired to real hardware. It is `workflow_dispatch`-only - never on `push`, never on `pull_request`. A self-hosted runner executing automatically-triggered workflows on a public repo would let an untrusted push or fork PR run arbitrary code on that physical machine; requiring a manual, deliberate trigger closes that off.
- **`pages.yml`** only deploys from `main`, never from a PR or a fork, so the published docs site (<https://ka5j.github.io/stm32_rtos/>) can't be hijacked by an untrusted branch. It also only ever reflects a tagged release, not in-progress `develop` work, since `PROJECT_NUMBER` is only bumped on a `develop`→`main` release PR.
- **`codeql.yml`** runs GitHub CodeQL static analysis on every push/PR to `main`/`develop`, plus a weekly schedule, with `permissions: contents: read, security-events: write` - just enough to read the repo and upload findings.
- **Dependabot** (`.github/dependabot.yml`) watches the `github-actions` ecosystem weekly - the actual supply-chain surface here, since there are no C package-manager dependencies to scan.

## Action pinning

Every third-party GitHub Action used in this repo's workflows is pinned to a full commit SHA, not a mutable version tag (`actions/checkout@11d5960...` with a `# v4.4.0` comment for readability, not `actions/checkout@v4`). A tag can be force-moved to point at different, potentially malicious code without any change visible in this repo's own history; a commit SHA can't be silently repointed the same way. Dependabot is what keeps these pins from calcifying - it opens a PR bumping both the SHA and its version comment when an action publishes a new release, so pinning for security doesn't quietly turn into running a permanently stale, unpatched action.
