# Changelog

All notable changes to this project are documented here. Format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); version numbers
follow this project's own [Semantic Versioning policy](docs/VERSIONING.md),
which explains what MAJOR/MINOR/PATCH mean pre-1.0 for this repository
specifically.

## [Unreleased]

### Added

- `CONTRIBUTING.md`: two new convention sections. "`const` on a
  register-block parameter" states that `const` is a claim about the
  hardware (the peripheral is unchanged), not merely about the pointer,
  which is why a side-effecting read like `uartReceiveByte()` does not
  get it. "Testing and its limits" records what host tests can and cannot
  establish, and names the PWR ordering bug as the worked example of the
  gap.
- `README.md`: a macOS 27 prerequisite note. The Command Line Tools ship
  an SDK whose `.tbd` stubs declare an `arm64e.x1` target that the
  linker in the same install rejects, which breaks every host link and so
  breaks `make test`, `make coverage`, and the entire pre-commit hook.
  Setting `SDKROOT` to an earlier installed SDK works around it; this is
  deliberately not put in the Makefile, where a hardcoded macOS
  `-isysroot` would break Linux CI.
- `drivers/inc/uart.h`, `drivers/src/uart.c`: `uartFlush()`, a blocking
  wait on `SR.TC`. `uartTransmit()` now calls it once its buffer is
  written, so a successful return means the data is actually on the wire
  rather than merely handed to DR - see the Fixed section.
- `drivers/inc/pwr.h`, `drivers/src/pwr.c`: `pwrWaitVoltageScaleReady()`,
  the second half of what was previously a single `pwrSetVoltageScale()`.
  See the Fixed section below for why voltage scaling had to be split in
  two.
- `drivers/src/flash.c`: `flashSetLatency()` now reads `ACR.LATENCY` back
  and reports `DRIVER_STATUS_ERR_HW_FAULT` if it does not match what was
  written, as RM0390 requires before relying on a programmed latency.
  This is the project's only region excluded from the `make coverage`
  gate (`GCOVR_EXCL_START`/`STOP`): the check compares a register against
  the value just written to it, which a plain in-memory test struct
  always agrees with by construction. The reasoning is recorded in the
  code and in the Makefile's coverage comment.
- `drivers/inc/uart.h`, `drivers/src/uart.c`: the UART/USART driver -
  `uartInit()` (BRR baud-rate divisor from a bus clock and target baud,
  standard 16x oversampling, plus CR1 parity/direction and CR2 stop
  bits, UE set last), `uartDeinit()`, and blocking
  `uartTransmitByte`/`uartTransmit`/`uartReceiveByte`/`uartReceive`. 8
  data bits only, matching `uart_reg.h`'s own documented "8N1" scope -
  CR1.M is unconditionally cleared, not a configurable parameter; an
  earlier draft of this driver exposed a `word_length` parameter
  accepting 9-bit mode without widening transmit/receive past
  `uint8_t`, which would have silently truncated every byte through DR
  in that mode. Caught in review before merge, not shipped.
  `uartReceiveByte()` always reads DR once RXNE sets, whether or not a
  fault (ORE/NE/FE/PE) is also reported, since RM0390 clears those flags
  via an SR read followed by a DR read - skipping the DR read on a fault
  would leave the flag set and corrupt the next call's fault check.
  `uartReceiveByte`/`uartReceive` take `const UartRegisters_t *`, matching
  `gpioReadPin()`'s CMSIS-style const-for-read-only convention - neither
  function ever writes through the pointer.
  BRR's baud-rate math is fixed-point integer arithmetic (USARTDIV*100 in
  a 64-bit intermediate), not floating point, matching RM0390's own
  worked examples and avoiding any FPU dependency; validates the computed
  mantissa fits BRR's 12-bit field (baud too low for the given clock) and
  rejects a mantissa/fraction of 0/0 (baud too high - RM0390 forbids a
  BRR of 0).
  Every transmit/receive function blocks by spin-polling TXE/RXNE with a
  bounded iteration count - the only thing implementable before an NVIC
  driver and RTOS scheduler exist to build an interrupt- or DMA-driven
  variant against. A task calling these once real tasks exist will
  monopolize the CPU for the duration of the transfer; this is a known,
  deliberate limitation of this revision, documented in `uart.h`'s
  file-level comment, not the intended final form.
- `device/inc/uart_reg.h`: `USART_CR2_STOP_1`/`_0_5`/`_2`/`_1_5`
  field-value macros, matching the existing `RCC_CFGR_SYSCLK_*`-style
  naming pattern - the field previously had only its raw bit/mask macro,
  no named values.
- `tests/unit/test_uart.c`: full coverage including every timeout and
  hardware-fault branch, the BRR fraction-carry case, every one of
  STOP's 4 documented values individually (matching
  `rccBusPrescalerConfig`'s exhaustive-value tests), and that CR1.M is
  unconditionally cleared even if it started set. BRR test vectors were
  computed independently (Python, not this driver's own arithmetic) and
  asserted as exact values. `make coverage` holds 100% line/branch
  across `drivers/src/{gpio,rcc,flash,pwr,uart}.c`.
- `Makefile`: `uart.c` added to `TEST_DRIVER_SOURCES`; `misra-c2012-2.5`
  suppression added for `device/inc/uart_reg.h` (SR.IDLE/TC and every CR1
  interrupt-enable/SBK/RWU/WAKE/OVER8 bit have no consumer - this driver
  is blocking-only, no interrupts or IDLE-line detection) and
  `misra-c2012-8.7` for `drivers/src/uart.c` (no caller outside its own
  file yet, matching `gpio.c`/`rcc.c`).
- `drivers/inc/rcc.h`, `drivers/src/rcc.c`: RCC's HSI/HSE-to-PLL SYSCLK
  bring-up - `rccHsiEnable`/`rccHsiDisable`, `rccHseEnable` (crystal or
  bypass mode)/`rccHseDisable`, `rccPllConfig`/`rccPllEnable`/
  `rccPllDisable`, `rccBusPrescalerConfig`, and the orchestrating
  `rccSysclkSwitch`, which sequences the PWR voltage-scale and flash-
  latency changes a SYSCLK switch requires (via the two new drivers
  below) before touching `RCC_CFGR.SW`, always applying the target
  voltage scale/latency unconditionally regardless of switch direction
  (see `pwrSetVoltageScale()`/`flashSetLatency()` for why that ordering
  is safe both ways). Every hardware-ready wait (HSIRDY/HSERDY/PLLRDY/
  SWS) is a bounded iteration-count retry, not a wall-clock timeout -
  SysTick isn't configured this early in bring-up - which turned out to
  make this host-testable the same way clock gating already was: the
  ready bit is a field distinct from what the driver writes, so a test
  can hold it clear indefinitely to exercise the timeout branch. This
  corrects the previous assumption (`rcc.h`'s file comment, the
  Makefile's `TEST_DRIVER_SOURCES` comment, and the `0.1.3` CHANGELOG
  entry) that SYSCLK bring-up couldn't be host-tested; that assumption
  predated actually writing and testing it.
  `rccGpioClockEnable`/`rccGpioClockDisable`/etc. (clock gating, `0.1.3`)
  are unchanged.
- `drivers/inc/flash.h`, `drivers/src/flash.c`: the flash interface
  driver - `flashSetLatency()`, a single validated write to `ACR.LATENCY`
  with no polling (RM0390 documents no busy/ready flag for this field).
- `drivers/inc/pwr.h`, `drivers/src/pwr.c`: the PWR driver -
  `pwrSetVoltageScale()`, which writes `CR.VOS` and polls `CSR.VOSRDY`
  with the same bounded-retry pattern as RCC's ready-waits.
- `device/inc/rcc_reg.h`: `RCC_PLLCFGR_PLLSRC_HSI`/`_HSE` field-value
  macros, matching the existing `RCC_CFGR_SYSCLK_*` naming pattern -
  `RCC_PLLCFGR_PLLSRC` previously named only the HSE-selected bit value,
  with no name for HSI (0).
- `tests/unit/test_rcc.c`, `tests/unit/test_flash.c`, `tests/unit/test_pwr.c`:
  full coverage of the above, including every timeout branch (via a fake
  register block that never sets the polled ready bit) and, for
  `rccBusPrescalerConfig`'s chained validation, every one of HPRE's 9 and
  PPRE1/PPRE2's 5 documented field values individually - gcov branch
  coverage requires each `&&`-chained comparison's false outcome
  exercised at least once, which a single valid-value test doesn't reach
  for every clause. `make coverage` holds 100% line/branch across
  `drivers/src/{gpio,rcc,flash,pwr}.c`.
- `Makefile`: `TEST_DRIVER_SOURCES` gains `drivers/src/flash.c` and
  `drivers/src/pwr.c`. `lint`'s suppression list gains `misra-c2012-2.5`
  for `device/inc/flash_reg.h`/`device/inc/pwr_reg.h` (both still have
  RM0390 fields with no consumer - self-programming, low-power modes,
  PVD, ... - beyond what this change uses), matching the existing
  `gpio_reg.h`/`rcc_reg.h` pattern. `flash.c`/`pwr.c` do **not** need a
  `misra-c2012-8.7` suppression the way `gpio.c`/`rcc.c` do: `rcc.c`
  calls into both directly, so cppcheck's whole-project analysis already
  sees a real caller.

- `drivers/inc/rcc.h`, `drivers/src/rcc.c`: RCC peripheral clock gating -
  `rccGpioClockEnable`/`rccGpioClockDisable` (any of GPIOA..GPIOH, taking
  the port's existing `GpioRegisters_t *` rather than a new enum),
  `rccUsart2ClockEnable`/`rccUsart2ClockDisable`,
  `rccSyscfgClockEnable`/`rccSyscfgClockDisable`,
  `rccPwrClockEnable`/`rccPwrClockDisable`. Scoped to clock gating only -
  RCC's HSI/HSE-to-PLL SYSCLK bring-up is deliberately deferred to a
  separate change, since it requires a blocking hardware-ready poll (not
  host-testable the way this file is) and careful flash-latency/PWR-
  voltage-scale sequencing against `flash_reg.h`/`pwr_reg.h`. This is the
  first driver in the project where the register block is a hardware
  singleton (there is only one RCC), yet still takes `RccRegisters_t *`
  as a parameter rather than reaching for the `RCC` macro internally -
  see `CONTRIBUTING.md`'s error-handling contract section for why.
- `tests/unit/test_rcc.c`: 12 tests covering every port-identity branch
  (all 8 GPIO ports plus the unrecognized-pointer rejection case, for
  both enable and disable), bit-isolation, and the USART2/SYSCFG/PWR
  gate functions - 100% line and branch coverage via `make coverage`.
- `Makefile`'s `lint` target: added a permanent deviation for
  `misra-c2012-11.4` (pointer/integer conversion), scoped by glob
  (`*_reg.h`) rather than to one file or globally - every peripheral
  base-address macro in every register header
  (`#define GPIOA ((GpioRegisters_t *)GPIOA_BASE)`, and the same pattern
  for `RCC`, `USART2`, `EXTI`, ...) does this cast, it's the only way to
  define a pointer to a fixed memory-mapped hardware address in standard
  C, and it will never represent a real defect here - scoping it per-file
  the way `misra-c2012-2.5`/`8.7` are scoped would mean adding a new
  suppression line every time any peripheral's pointer macro gets its
  first real caller, forever, which doesn't scale. The glob covers every
  register header without that growing list, while leaving the rule
  active for driver/api/bsp/rtos `.c` files, where a genuinely risky
  pointer/integer conversion should still be caught - verified with a
  deliberately-bad cast in a throwaway `.c` file. Also added
  `misra-c2012-2.5` for `device/inc/rcc_reg.h` and `misra-c2012-8.7` for
  `drivers/src/rcc.c`, matching the existing `gpio.c`/`gpio_reg.h`
  suppressions and for the same reasons (see the Makefile's comment).
- `Doxyfile`: turned on the diagram generator (`HAVE_DOT=YES`, previously
  off) with a curated subset rather than every default graph - directory
  and include/included-by graphs (`DIRECTORY_GRAPH`, `INCLUDE_GRAPH`,
  `INCLUDED_BY_GRAPH`) and the Topics group graph (`GROUP_GRAPHS`) stay on,
  since they're generated straight from the real `#include`/`@ingroup`
  structure and can't drift from what the code does the way a hand-drawn
  diagram can; `CLASS_GRAPH`/`COLLABORATION_GRAPH`/`GRAPHICAL_HIERARCHY`
  (no C++ inheritance to show - every register struct is flat) and
  `CALL_GRAPH`/`CALLER_GRAPH` (almost nothing calls anything yet above
  `drivers/src/gpio.c`) are explicitly turned back off since they'd render
  as content-free noise at this project's current stage. Graphs render as
  interactive, zoomable SVG (`DOT_IMAGE_FORMAT=svg`, `INTERACTIVE_SVG=YES`)
  instead of fixed-resolution PNG. `DIR_GRAPH_MAX_DEPTH` raised to 2 so
  `rtos/kernel/` and `rtos/api/` show as distinct nodes instead of
  collapsing into one `rtos` node.
- `Doxyfile`: `HTML_COLORSTYLE=TOGGLE` (manual light/dark switch, not just
  OS-preference-following `AUTO_LIGHT`), `FULL_SIDEBAR=YES`
  (ReadTheDocs-style full-height nav - no `PROJECT_LOGO` is set to lose
  room for), `SOURCE_BROWSER=YES` with `REFERENCES_RELATION`/
  `REFERENCED_BY_RELATION=YES` (browsable, cross-linked source; click a
  macro and jump to its definition, or see every documented place that
  references it), `SHOW_ENUM_VALUES=YES` (shows e.g. an `IRQn_e` entry's
  actual interrupt number inline instead of requiring a click-through),
  and `HTML_DYNAMIC_SECTIONS=YES` (collapsible sections for the register
  headers that document 20-40+ macros in one file).
- `.github/workflows/ci.yml`, `.github/workflows/pages.yml`: install
  `graphviz` before `make docs`, now required for the diagrams above -
  unpinned, unlike the Doxygen/cppcheck/clang-format versions, since it
  only draws pictures and gates no pass/fail check CI enforces.
- `README.md`: `graphviz` added to the prerequisites list for both
  package managers.
- `docs/mainpage.md`: mentions the new dependency graphs and browsable,
  cross-linked source on every file/group page.

### Changed

- `README.md`'s Status section is now the single source of truth for
  project status, restructured as a per-layer table. `docs/ARCHITECTURE.md`,
  `docs/VERSIONING.md`, and `docs/mainpage.md` previously each carried
  their own prose copy of the same status, so every driver merge needed
  five synchronized edits that nothing checked for drift; they now link to
  it instead. It also records the on-target gap explicitly: no driver has
  ever executed on silicon, because `app/src/main.c` is an empty loop and
  the linker garbage-collects all of `drivers/` out of the image.
- Removed `.github/workflows/hil.yml`. Its smoke-test step was never
  written - it ended in `exit 1`, so the only workflow that touched real
  hardware failed by construction on every run - and the board is
  attached to the development machine anyway, which made the self-hosted
  runner a trust boundary the project was carrying without collecting any
  benefit from. On-target work is now manual (`make flash`, `make debug`),
  no GitHub-triggered workflow reaches physical hardware, and every
  remaining workflow runs on an ephemeral GitHub-hosted runner.
  `SECURITY.md` and `CONTRIBUTING.md` updated accordingly.
- `Makefile`: `misra-c2012-8.7` is now suppressed for `drivers/src/pwr.c`
  too. It did not need the suppression while `rcc.c` called
  `pwrSetVoltageScale()` directly, which gave cppcheck a second
  translation unit using it; the voltage-scale sequencing moving out to
  the caller leaves pwr.c with no in-project caller until `api/` or `bsp/`
  grows one.
- `CONTRIBUTING.md`: the error-handling-contract example now shows
  `rccGpioClockEnable(RCC, GPIOA)` (two arguments) instead of the stale
  `rccGpioClockEnable(GPIO_PORT_A)` (one argument, predating `rcc.c`).
  Added a paragraph stating explicitly, as project policy, that every
  driver function takes its peripheral's register block as a parameter -
  including for a hardware singleton like RCC - and why: it's what makes
  driver logic host-testable off-target, which this project prioritizes
  over the vendor HAL/LL convention (operate on the fixed global instance
  directly) since that convention exists only because HAL/LL has no
  host-side test infrastructure to begin with.
- `README.md`, `docs/ARCHITECTURE.md`, `docs/VERSIONING.md`,
  `docs/mainpage.md`: corrected status text that still described `RCC`
  as entirely unimplemented after clock gating landed - same class of
  staleness the project has corrected after every previous driver
  addition.
- `Doxyfile`: condensed every tag's explanatory comment from Doxygen's
  full stock template prose (often 5-20 lines) down to at most two lines
  each, keeping only what's project-relevant; 3053 lines -> 1078.
- `Makefile`: `BUILD` profile knob (`debug`, the existing default, or
  `release`) - `debug` keeps `-O0 -g3` (weakest optimizer, so GCC's
  uninitialized-variable analysis stays maximally sensitive per
  `driver_status.h`'s rationale); `release` builds `-O2 -g`, needed before
  any real timing characterization of the scheduler/context switch, since
  `-O0` code doesn't represent shipped instruction counts or cycle timing.

### Fixed

- `startup/startup_stm32f446re.s`: `.size vector_table, .-vector_table`
  was emitted *before* the `vector_table:` label, so the symbol's size was
  computed at the wrong point and reported wrong by `nm` and debuggers. It
  now follows the table. Separately, the post-`main()` fallback was
  `bl main` / `bx lr`, which happens to spin (bl leaves `lr` pointing at
  the `bx` itself) but reads as a return to a caller that does not exist;
  it is now an explicit labelled infinite loop, matching
  `Default_Handler`.
- `Makefile`: test and coverage object paths now mirror each source's full
  path instead of being flattened with `$(notdir)` and resolved through a
  `vpath`. The old scheme silently collapsed any two sources sharing a
  basename onto one object file - `tests/unit/gpio.c` and
  `drivers/src/gpio.c` would have collided, with whichever `vpath` entry
  came first winning. No collision existed yet; the shape that permits one
  is gone.
- `drivers/src/uart.c`: `uartTransmit()` returned as soon as the last
  byte reached DR, while that byte was still being shifted out. Following
  it with `uartDeinit()`, a clock gate, a baud change, or a low-power
  transition truncated the final character. It now ends with a `SR.TC`
  wait via the new `uartFlush()`, and `uartDeinit()` documents the
  precondition for callers driving `uartTransmitByte()` directly.
- `drivers/src/uart.c`: `uartInit()` programmed BRR and the framing
  fields without first clearing `CR1.UE`. RM0390 requires those be
  written with the USART disabled, so reconfiguring a running instance
  (changing baud rate, say) wrote into a live peripheral and could
  corrupt an in-flight frame. UE is now cleared first and restored last.
- `drivers/src/uart.c`: the `SR.TXE`/`SR.RXNE` waits had the same
  off-by-one as the RCC/PWR waits - concluding `ERR_TIMEOUT` from
  `timeout == 0` misreports success when the flag sets on the final
  iteration. Both now re-read the flag, through a shared
  `uartWaitSrFlag()` helper that also serves the new TC wait.
- `drivers/inc/uart.h`: `uartReceiveByte()`/`uartReceive()` no longer take
  `const UartRegisters_t *`. Nothing is written through the pointer, which
  is what the const originally recorded, but reading DR clears `SR.RXNE`
  and the ORE/NE/FE/PE flags - the peripheral does change. The const
  advertised a repeatable, side-effect-free observation that the hardware
  does not provide. cppcheck flags the parameter as const-able for exactly
  the same reason a reader would assume it; that finding is suppressed
  inline with the rationale.
- **`drivers/src/rcc.c`, `drivers/src/pwr.c`: the PWR voltage-scale
  sequencing was wrong in both directions and is now corrected.**
  `rccSysclkSwitch()` took `pwr` and `vos` parameters and applied the
  voltage scale as part of the switch, but it also requires the
  requested source's ready bit to be set before switching - so for a PLL
  source the scale was being written with the PLL already locked. RM0390
  section 5.1.4 permits `PWR_CR.VOS` to be modified *only while the PLL
  is off*, so that write was discarded by the hardware and the core would
  then have run off the PLL at a frequency the regulator was never scaled
  for. Compounding it, `pwrSetVoltageScale()` polled `CSR.VOSRDY`
  immediately after writing `CR.VOS`; that flag only reports the
  regulator settled *after* the PLL is switched on, so simply moving the
  call earlier would have made it time out on every boot instead.
  The fix follows the hardware's actual two phases: `pwrSetVoltageScale()`
  now writes `CR.VOS` and polls nothing, `pwrWaitVoltageScaleReady()`
  polls `CSR.VOSRDY` and is called after `rccPllEnable()`, and
  `rccSysclkSwitch()` drops both PWR parameters, narrowing to the flash
  latency it can legitimately sequence. The full ordered bring-up
  sequence a caller must now follow is documented in `drivers/inc/rcc.h`'s
  file-level comment. Neither half was caught by the test suite, which
  held 100% branch coverage throughout: a register struct in host memory
  has no opinion about when a field is writable.
- `drivers/src/rcc.c`, `drivers/src/pwr.c`: the bounded hardware-ready
  waits could report a spurious timeout. Each spun
  `while (flag clear && timeout > 0) timeout--;` and then concluded
  `if (timeout == 0) -> ERR_TIMEOUT`, which misreports success whenever
  the flag sets on the final iteration - the loop exits with the counter
  already at zero. They now re-read the flag after the loop and decide on
  that instead. The three CR oscillator waits in `rcc.c` are now one
  `rccWaitCrReady()` helper, so the reasoning lives in one place rather
  than being restated at each site.

- `Doxyfile`: `SORT_MEMBER_DOCS` YES -> NO. Every struct here is a
  memory-mapped register block where declaration order is the real
  hardware byte-offset order (RM0390/PM0214) - the default alphabetical
  sort was scrambling each struct's *detailed* field documentation
  (e.g. `GpioRegisters_t` showing `AFRH, AFRL, BSRR, IDR, ...`) out of
  sync with the correctly byte-offset-ordered summary table directly
  above it on the same generated page.
- `startup/startup_stm32f446re.s`: `Reset_Handler` now enables the FPU
  (`SCB->CPACR` CP10/CP11 full access) and sets interrupt priority
  grouping (`SCB->AIRCR` PRIGROUP=3, all 4 implemented priority bits as
  preemption priority) before copying `.data`/calling `main()`. Every C
  file already builds `-mfpu=fpv4-sp-d16 -mfloat-abi=hard`; without this,
  the first VFP instruction any code emits would trap as a UsageFault
  (NOCP) instead of executing, and `rtos/kernel/` would inherit whatever
  priority-grouping default happened to be in place instead of the
  grouping a preemptive scheduler assumes. Both were previously
  unconfigured - `core/inc/scb_reg.h` defined the fields but nothing wrote
  them.
- `startup/startup_stm32f446re.s`: removed the `.fpu softvfp` assembler
  directive - vestigial, contradicted the hard-float build (this file has
  no VFP instructions of its own either way, so it was a no-op, but
  confusing to read next to `-mfloat-abi=hard`).

## [0.1.3] - 2026-09-01

### Added

- `drivers/inc/gpio.h`, `drivers/src/gpio.c`: the GPIO driver -
  `GpioPin_e`/`GpioPinState_e` types and `gpioInit`/`gpioDeinit`/
  `gpioSetAlternateFunction`/`gpioWritePin`/`gpioReadPin`/`gpioTogglePin`,
  the first driver-layer logic this project has shipped. `gpioInit`
  validates `mode`/`otype`/`ospeed`/`pupd` before writing any register
  (including rejecting `pupd == 0x3`, reserved per RM0390) and writes
  `AFRL`/`AFRH` before `MODER` to avoid a mode-switch glitch;
  `gpioWritePin`/`gpioTogglePin` use `BSRR`, never a non-atomic `ODR`
  read-modify-write.
- `tests/unit/test_gpio.c`: 18 tests covering every validation branch and
  field-isolation case (a write to one pin's bits must not touch its
  neighbors' bits in the same register).
- `make coverage` (Makefile) + `gcovr` (pinned `8.6`) in CI: gates line and
  branch coverage at 100% on `TEST_DRIVER_SOURCES`, currently just
  `gpio.c`. Scoped to driver logic, not the register headers, which are
  pure data with no branches to cover.
- `.vscode/settings.json`: editor format-on-save wired to `.clang-format`,
  tracked via a `.gitignore` exception scoped to just that one file.

### Fixed

- `tests/unit/test_gpio.c`: GCC does not honor a `(void)` cast as
  acknowledging a `warn_unused_result`-attributed return value the way
  Clang does, so `-Werror=unused-result` turned it into a hard build
  failure on CI's `arm-none-eabi-gcc`/host `cc` (both GCC) despite passing
  every time locally on macOS/Clang. Fixed by capturing the result into a
  named variable and asserting on it, matching every other `gpioInit` call
  in the file.
- `startup/startup_stm32f446re.s`: missing trailing newline.

### Changed

- `.clang-format`: `BasedOnStyle` `GNU` → `LLVM` (GNU silently ignores
  `BreakAfterReturnType`-family overrides - confirmed by testing the same
  override under each base style; every other property was already
  explicitly pinned so `LLVM` costs nothing), 4-space indent,
  `InsertBraces`/`AllowShort*OnASingleLine: Never` so no conditional, loop,
  or non-empty function can collapse to a one-liner that hides logic,
  supporting MISRA's brace-everywhere spirit at the tooling level. Verified
  byte-identical output between clang-format 18 (CI's pin) and 22 (local)
  before adopting; `AlignConsecutiveMacros` deliberately left off since its
  column math differs between those two versions.
- `Makefile`'s `lint` target: `misra-c2012-2.5` (unused macro) on
  `device/inc/gpio_reg.h` and `misra-c2012-8.7` (external linkage used in
  only one translation unit) on `drivers/src/gpio.c` suppressed, scoped to
  those two files specifically, not the rules project-wide - both are real
  findings today (nothing in `api`/`bsp`/`app` calls this code yet) but
  neither is a defect. Remove both suppressions once `api/` gives `gpio.c`
  a real caller.
- `Makefile`'s `test` target now also compiles host-testable
  `drivers/src/*.c` files (`TEST_DRIVER_SOURCES`, currently just
  `gpio.c`) into the test binary - previously only `tests/unit/*.c` was
  ever compiled, so `gpio.c` had no way to be tested until this landed.
- `README.md`, `docs/VERSIONING.md`, `docs/mainpage.md`,
  `docs/ARCHITECTURE.md`: corrected status text that still described
  `drivers/` as having no driver logic after the GPIO driver landed, same
  class of staleness `driver_status.h` caused in `0.1.2`.
- `docs/VERSIONING.md`: clarified that PATCH covers incremental progress on
  a not-yet-complete new layer (a single driver file, not the whole layer)
  as well as fixes to an already-released one - `0.2.0` is reserved for
  `drivers/`'s actual completion (GPIO, RCC, UART, NVIC, SysTick all
  implemented and tested), not any single driver file landing.
- `CONTRIBUTING.md`: documents `make coverage` in the CI step order and
  notes it's CI-only, not part of the pre-commit hook's five checks.

## [0.1.2] - 2026-08-26

Register-layer completeness fixes, a real NVIC/vector-table cross-check,
the `drivers/` error-status contract, per-peripheral Doxygen grouping, and
this changelog.

### Added

- `drivers/inc/driver_status.h`: the `DriverStatus_e` error-status contract
  required by every driver function that can fail, per `CONTRIBUTING.md`'s
  error-handling contract - a small, closed set of failure categories,
  `DRIVER_MUST_CHECK`-marked, with a non-zero `DRIVER_STATUS_OK` and an
  explicit `DRIVER_STATUS_UNINITIALIZED` sentinel so an unassigned status
  can't silently read as success under this project's `-O0` build.
- `tools/check_vector_table.awk`, run as part of `make test`: a real
  cross-check between `core/inc/nvic_reg.h`'s `IRQn_e` enum and
  `startup/startup_stm32f446re.s`'s vector table, replacing a test that only
  asserted the enum against its own values and could not catch the two
  files drifting apart.
- Per-peripheral Doxygen subgroups (`mpu_registers`, `nvic_registers`,
  `scb_registers`, `fpu_registers`, `systick_registers`, `gpio_registers`,
  `rcc_registers`, `uart_registers`, `exti_registers`, `flash_registers`,
  `iwdg_registers`, `pwr_registers`, `syscfg_registers`, `wwdg_registers`),
  nested under `core_peripherals`/`device_peripherals` instead of every
  register header joining one flat parent group. `scb_reg.h`'s two
  architecturally distinct blocks (SCB, FPU) now render as separate groups.
- `-Werror=unused-result` in the Makefile's `CFLAGS` and host test compile
  line: promotes an ignored `DRIVER_MUST_CHECK` return from a warning to a
  build failure on both `arm-none-eabi-gcc` and the host `cc`.

### Fixed

- `core/inc/scb_reg.h`: added the missing FPU lazy-stacking fault bits to
  `SCB_CFSR` (`MLSPERR`, bit 5; `LSPERR`, bit 13) - Cortex-M4F-specific
  CFSR extensions the header omitted, relevant to any future HardFault
  handler or PendSV context switch.
- `core/inc/mpu_reg.h`: added the missing `MPU_RASR_AP_PRIV_RW_UNPRIV_RO`
  (AP=0x2) encoding, one of six legitimate MPU access-permission values -
  the one needed to make kernel-owned data read-only to task code.
- CI: Arm GNU Toolchain, cppcheck, and clang-format are now pinned to exact
  versions (`15.2.rel1`, `2.13.0-2ubuntu3`, `1:18.0-59~exp2`) instead of
  floating `apt` packages, so an archive update can only fail the build
  loudly rather than silently changing what CI enforces.

### Changed

- `device/inc/flash_reg.h`, `device/inc/rcc_reg.h`: documented previously
  unstated scope decisions (which real register fields each header
  intentionally omits, and why), matching the pattern already used in
  `uart_reg.h`/`exti_reg.h`/`syscfg_reg.h`.
- `CONTRIBUTING.md`: documents the two-level Doxygen grouping convention
  (parent layer group, per-peripheral subgroup, multi-block-per-file
  splitting) for future register headers.
- `README.md`, `docs/VERSIONING.md`, `docs/mainpage.md`, `docs/ARCHITECTURE.md`:
  corrected status text that still described `drivers/` as entirely empty
  scaffolding after `driver_status.h` landed, and updated `docs/mainpage.md`'s
  description of the Doxygen Topics page to match the per-peripheral
  subgroup structure above.

### Removed

- `.github/dependabot.yml`: Dependabot is disabled for this repository.
  `SECURITY.md`'s claim that no automated tooling watches the Actions SHA
  pins is now accurate as a result.

## [0.1.1] - 2026-08-24

Register-layer bug fixes and completeness, Doxygen reorganization, CI
hardening, documentation tone rewrite.

### Added

- `device/inc/syscfg_reg.h`: SYSCFG register layer (RM0390 section 9),
  scoped to EXTI-line port routing (`EXTICR1-4`) and I/O compensation cell
  control (`CMPCR`).
- `docs/groups.dox`: Doxygen "Topics" page group hierarchy, reorganizing
  generated docs by architectural layer instead of raw file listing.
- `docs/mainpage.md`: a proper landing page for the generated Doxygen site.
- `tests/unit/test_rcc_reg.c`, `tests/unit/test_syscfg_reg.c`.

### Fixed

- `core/inc/systick_reg.h`: added the missing `SYSTICK_CTRL.TICKINT` bit.
- `core/inc/scb_reg.h`: corrected incorrect READ-only comments on `CFSR`/
  `HFSR` fields.
- Hardware-read-only register struct fields marked `const`, CMSIS-style,
  across the register layer (`volatile const uint32_t`, not `volatile
  uint32_t`, for fields the hardware alone writes).
- `device/inc/rcc_reg.h`: filled in RCC register/field gaps needed before
  driver work could start.
- Stale peripheral lists and `make clean` documentation corrected after
  `syscfg_reg.h` landed.

### Changed

- Documentation tone rewrite across `README.md`, `CONTRIBUTING.md`,
  `SECURITY.md`, `docs/VERSIONING.md`, `docs/ARCHITECTURE.md`.
- `.github/workflows/ci.yml`: added the PR-only `@file`-block and
  test-coverage diff checks.

### Removed

- `.github/dependabot.yml`.

## [0.1.0] - 2026-08-23

Initial release: boot pipeline, complete register layer, and full
development pipeline. `drivers/`, `api/`, `bsp/`, and `rtos/` remain empty
scaffolding - no driver logic implemented yet.

### Added

- Boot pipeline: `linker/STM32F446RE.ld`, `startup/startup_stm32f446re.s`,
  and the `Makefile` (`all`/`flash`/`erase`/`debug`/`size`/`clean` targets).
  Builds and flashes successfully to a NUCLEO-F446RE.
- Register layer, hand-derived from PM0214 and RM0390 with no HAL/CMSIS
  device headers:
  - `core/inc/`: MPU, NVIC, SCB (+ FPU context control), SysTick.
  - `device/inc/`: EXTI, Flash interface, GPIO, IWDG, PWR, RCC, UART, WWDG.
- Host-side unit tests (`tests/unit/`, vendored Unity in `tests/unity/`),
  one `test_<peripheral>_reg.c` per register header, aggregated by
  `tests/unit/test_runner.c`.
- Development pipeline: `make format`/`format-check` (clang-format),
  `make lint` (cppcheck with a MISRA C:2012 subset), `make docs`
  (Doxygen, `WARN_AS_ERROR` on), `make test`, and the `.githooks/pre-commit`
  hook running all of them before every local commit.
- CI: `ci.yml` (build/format/lint/docs/test on every push and PR),
  `hil.yml` (manual-only hardware-in-the-loop smoke test on a self-hosted
  runner), `pages.yml` (Doxygen site published to GitHub Pages on push to
  `main`), `codeql.yml` (static analysis), `.github/dependabot.yml`
  (GitHub Actions dependency updates).
- `SECURITY.md`: vulnerability reporting policy, threat model, CI/CD trust
  boundaries, and the SHA-pinning policy for third-party GitHub Actions.
- `docs/VERSIONING.md`: the SemVer policy this changelog follows.
- `docs/ARCHITECTURE.md`, `CONTRIBUTING.md`: directory layout, layering
  rule, naming conventions, and the (not-yet-implemented) error-handling
  contract for the layers above the register level.
