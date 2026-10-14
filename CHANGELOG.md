# Changelog

All notable changes to this project are documented here. Format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); version numbers
follow this project's own [Semantic Versioning policy](docs/VERSIONING.md),
which explains what MAJOR/MINOR/PATCH mean pre-1.0 for this repository
specifically.

## [Unreleased]

### Added

- `tools/check_pr_files.sh` and `make check-pr [BASE=origin/develop]`: the
  two pull-request file checks that lived as inline shell in `ci.yml` (every
  touched source file has a Doxygen `@file` block; every touched
  driver/bsp/api/rtos `.c` has a `tests/unit/test_<name>.c` and every touched
  `*_reg.h` is included by a test), now runnable before pushing. CI calls the
  same target. Checked by hand against a branch that adds a bare module and
  a bare register header: all four error cases fire and the exit is 1.

- `make test SANITIZE=1` builds and runs the host suite under
  AddressSanitizer and UndefinedBehaviorSanitizer with every report fatal,
  and CI runs it after the plain suite. `gpio.h` documents an out-of-range
  pin as undefined behaviour; this is the build that would see it. All 275
  tests pass under it.

- `bsp/inc/bsp.h`, `bsp/src/bsp.c`: `bspClockInit(rcc, pwr, flash)`, the
  board's 180 MHz bring-up built from the driver calls in order: PWR
  clock, HSE bypass (8 MHz from the ST-LINK), regulator scale 1, PLL
  M=8/N=360/P=2, wait for the regulator, over-drive, AHB /1, APB1 /4
  (45 MHz), APB2 /2 (90 MHz), SYSCLK to the PLL with 5 flash wait states,
  then prefetch and caches. Stops at the first failing step and returns
  its status. `BSP_SYSCLK_HZ`/`BSP_HCLK_HZ`/`BSP_PCLK1_HZ`/`BSP_PCLK2_HZ`
  name the result for SysTick and UART setup. PLL M/N/P, the prescalers
  and the latency are the values in ST's own Nucleo-F446 180 MHz example;
  there is no VCO/APB validation in `rccPllConfig`/`rccBusPrescalerConfig`;
  the limits are `_Static_assert`s in `bsp.h` on this one configuration,
  and `test_bsp.c` checks that the registers `bspClockInit()` programs
  work out to the `BSP_*` frequencies. Ten host tests (the whole sequence, the
  limit checks, and one timeout or `BUSY` case per step). Adds a `board_support` Doxygen group, and `make
  coverage` now also gates `bsp/src` at 100%.

- `drivers/inc/flash.h`, `drivers/src/flash.c`: `flashEnableCaches()`
  sets ACR.PRFTEN, ICEN and DCEN in one read-modify-write that keeps
  LATENCY, and returns void (no failure path). No cache reset: the caches
  come out of power-on reset empty, as ST's `HAL_Init()` also assumes, so
  ICRST/DCRST stay unused. Covered by three tests in `test_flash.c`,
  including that `flashSetLatency()` preserves the cache bits.

- `drivers/inc/pwr.h`, `drivers/src/pwr.c`: `pwrEnableOverdrive()`
  (CR.ODEN, wait CSR.ODRDY, CR.ODSWEN, wait CSR.ODSWRDY) for the 180 MHz
  range, with bounded waits like `pwrWaitVoltageScaleReady()`; ODSWEN is
  not written if ODRDY never sets, and CR.VOS is preserved. The ready-flag
  waits share one helper, and the timeout constant is renamed
  `PWR_READY_TIMEOUT_ITERATIONS` (same 100000 value). `drivers/inc/rcc.h`'s
  ordered bring-up sequence gains the over-drive step between the VOS
  wait and the prescaler/SYSCLK-switch steps. Sequence and bit positions
  checked against ST's HAL (`HAL_PWREx_EnableOverDrive`), its
  `stm32f446xx.h`, and its Nucleo-F446 180 MHz example; RM0390 5.1.4 was
  not checked directly.

- `drivers/inc/systick.h`, `drivers/src/systick.c`: the SysTick driver,
  completing `drivers/` (7 of 7). `systickInit(systick, core_hz,
  tick_hz)` programs LOAD = `core_hz / tick_hz - 1`, rejecting zero
  arguments and any period outside 2 to 2^24 clocks (LOAD 0 never
  reloads; LOAD is 24 bits) without touching a register, and starts the
  timer last with CLKSOURCE|TICKINT|ENABLE; `systickDeinit` restores the
  reset state. The `SysTick_Handler` and tick counter stay with the kernel.
  Covered by `tests/unit/test_systick.c`.

- `drivers/inc/nvic.h`, `drivers/src/nvic.c`: the NVIC driver
  (`nvicEnableIrq`/`DisableIrq`, `SetPending`/`ClearPending`,
  `SetPriority`/`GetPriority`, and `nvicSetSystemHandlerPriority` for the
  six configurable system handlers via SCB SHPR1-3). Device IRQn is
  range-checked (0 to `FMPI2C1_Error_IRQn`) so a negative system-exception
  number can never index the NVIC arrays; priorities are 0-15, shifted into
  the implemented top nibble. ISER/ICER/ISPR/ICPR are written as single
  bits, never read-modify-written. No PRIGROUP setter: `Reset_Handler`
  already owns it. Covered by `tests/unit/test_nvic.c`; `make lint` gains
  scoped `misra-c2012-8.7` (nvic.c) and `2.5`/`2.3`/`2.4` (`nvic_reg.h`,
  `scb_reg.h`) suppressions now that those headers are analysed.

- `core/inc/scb_reg.h`, `core/inc/nvic_reg.h`, `core/inc/systick_reg.h`:
  added the core-register definitions the NVIC and SysTick drivers and the
  kernel's context switch need and the headers lacked. SCB: ICSR
  (`PENDSVSET`/`PENDSVCLR`/`PENDSTSET`/`PENDSTCLR`/`VECTACTIVE`), SCR
  (`SLEEPONEXIT`/`SLEEPDEEP`/`SEVONPEND`), `CCR.USERSETMPEND`, the SHCSR
  active and pended bits, the SHPR1 priority fields, and the six
  `EXC_RETURN_*` values (basic and FPU frames). `IRQn_e` gains the negative
  system-exception numbers (MemManage -12, BusFault -11, UsageFault -10,
  SVCall -5, PendSV -2, SysTick -1), which index SCB rather than the NVIC;
  `tools/check_vector_table.awk` only matches non-negative entries, so the
  vector-table cross-check is unaffected. SysTick gains the 24-bit
  `LOAD`/`VAL` masks and the `CALIB` fields. Each is covered by a test.
  Cross-checked against ARM's CMSIS `core_cm4.h` and ST's
  `stm32f446xx.h` (ST's F446 SVD has no core peripherals).

- `tools/install_cppcheck.sh` builds the exact cppcheck version CI pins
  (2.13.0) from source into a per-user prefix, verifying the cloned commit
  first, and `make lint` now uses it automatically when it is installed
  (`CPPCHECK=` overrides). `make lint` also warns whenever the cppcheck it
  runs differs from CI's version. Different cppcheck versions report
  different MISRA findings; a `cond ? A : B` expression passed locally under
  2.21 and failed CI under 2.13.0 (`misra-c2012-10.6`), costing a CI cycle.
  Reproduced that exact failure with the pinned build before relying on it.

### Changed

- `app/src/main.c`: dropped the unreachable `return 0;` after the infinite
  loop (MISRA C:2012 Rule 2.1). `tests/unit/test_nvic.c`: removed a
  duplicate `COUNT_OF` definition. No behaviour change.

- Docs and comments only: comments that narrated how the code used to be
  now say how it is, in `uart.c`, `uart.h`, `iwdg_reg.h`, `startup`, the
  Makefile, `ci.yml`, `test_pwr.c` and the README Status intro; the reasons
  they carried (why BRR rounds once, why the CI workflow must not use
  `cancel-in-progress`) stay. History belongs in this file and in commits.
  Also corrected: `rcc.h` now says a raising flash latency is already
  written when `rccSysclkSwitch()` returns `NOT_INITIALIZED` or `TIMEOUT`
  (safe, and left in place); the README's missing-VCO-check limitation is
  attributed to `rccPllConfig()`, not `rccBusPrescalerConfig()`; and
  `driver_status.h` names the GPIO functions that document a pin `@pre`
  instead of validating it.

- `Makefile`: the firmware now links with `-nostartfiles` and
  `-Wl,--fatal-warnings`. `Reset_Handler` already does everything crt0
  would, and linking crt0 pulled newlib's syscall stubs in ahead of
  `--gc-sections`, which printed four "`_close`/`_lseek`/`_read`/`_write`
  is not implemented" warnings on every link. The link is now silent, so
  any new linker warning fails the build. The image is 24 bytes smaller and
  the vector table is unchanged. The old Makefile comment calling those
  warnings unavoidable is replaced.

- `Makefile`: the host test and coverage objects now depend on a flags
  stamp, the way the firmware objects already did. Before this, editing
  `WARN_FLAGS` or `HOST_CFLAGS` rebuilt nothing, so a new warning stayed
  hidden until a clean build.

- `core/inc/*_reg.h`, `device/inc/*_reg.h`: every bit and mask macro now
  casts its constant before shifting, `((uint32_t)1U << X_Pos)` instead of
  `(1U << X_Pos)`. `1U` has the essential type of an 8-bit unsigned, so
  `1U << 17U` is a real MISRA C:2012 Rule 12.2 violation, not the
  cppcheck limitation the old comments called it; with the cast the rule
  passes under CI's cppcheck 2.13.0. That removes all 50 inline
  `// cppcheck-suppress misra-c2012-12.2` comments from `drivers/` and
  `bsp/`, and the comments that justified them. Generated code is
  unchanged. The Makefile's lint notes now say the rule is deliberately
  not suppressed anywhere.

- Docs and comments only, from the full review: claims the code or the
  manual did not back are corrected or removed. `rccBusPrescalerConfig()`
  no longer says its fields "take effect immediately and synchronously"
  (RM0390 has no ready flag, but a divider can apply a few AHB cycles after
  the write); `uartInit()` no longer attributes its clear-UE-first order to
  an RM0390 requirement (the order is the safe one either way) and now says
  it rewrites only the fields it names, so a re-init of an instance that
  used interrupts or CR3 features should `uartDeinit()` first; `bsp.h` says
  what the `_Static_assert`s do and do not cover and that `test_bsp.c` ties
  the programmed registers to the `BSP_*` constants; CONTRIBUTING.md no
  longer says an unresolved bare include signals a layering violation (every
  `inc/` directory is on every compile's path, so layering is a convention);
  the changelog no longer says a test binds the startup file's literals to
  `scb_reg.h` (it pins the header's side only); `iwdg_reg.h` names
  `IWDG_KEY_ENABLE_ACCESS` correctly; the Makefile comments give the real
  CI step order and explain the four libc "not implemented" link warnings.
  Checked against RM0390 Rev 6 (March 2021) once the manual was available:
  the "RM0390 Table 15" citations pointed at the *Low-power mode summary*;
  the flash wait states are Table 5 (section 3.4.1), and the note beneath
  it gives each voltage scale's maximum HCLK (scale 3 120 MHz, scale 2
  144 MHz, scale 1 168 MHz, the last two extended by over-drive; an
  earlier revision of this entry wrongly said the manual gives no such
  figures). Section numbers now match Rev 6, including the chapter numbers
  in each register header's file comment: GPIO 7.4 (was 8.4), SYSCFG 8.2
  (9.2), EXTI 10.3 (12.3), Flash registers 3.8 (3.7), IWDG 20.4 (21.4),
  WWDG 21.6 (20.6), USART 25.6 (19.6), and the voltage regulator and
  over-drive text is 5.1.3, not 5.1.4. Also from checking the headers
  against the manual: `iwdg_reg.h` said SR must be polled clear before a
  PR/RLR write "takes effect", but RM0390 says the wait is needed only
  before writing them again; `syscfg_reg.h` said PMC holds Ethernet
  selection (only ADCxDC2 on the F446), now notes SYSCFG_CFGR (0x2C) is not
  modeled and that PH exists only for EXTI0/1; `exti_reg.h` now says bit 19
  is reserved in RTSR/FTSR; `gpio_reg.h` notes GPIOx_IDR has no fixed
  reset value and calls OSPEEDR 0b11 "high speed" as the manual does;
  `pwr.h` no longer says VOSRDY "cannot set" with the PLL off or that a
  VOS write with the PLL on is "discarded" (RM0390 says neither);
  `flash.c`'s read-back comment now quotes what 3.4.1 actually says;
  `rcc.h` notes PLLSRC also needs PLLI2S off, which this driver never
  enables; `flash_reg.h` says OPTCR1's offset comes from ST's CMSIS header.
  GPIOA_OSPEEDR resetting to 0 is correct (RM0390 Table 26: `GPIOx_OSPEEDR`,
  x = A..H, resets to 0 and only `GPIOB_OSPEEDR` is `0xC0`); the F401/F411
  value `0x0C000000` does not apply to the F446. RM0390's over-drive text
  also confirms what `pwr.h` had only from ST's HAL: no peripheral clocks
  other than PWR's should be on during the over-drive switch.

- `drivers/inc/gpio.h`, `drivers/src/gpio.c`: `gpioDeinitToReset(port, pin,
  reset)` is `gpioDeinit()`'s register work with the reset values passed
  in; `gpioDeinit()` is now `gpioDeinitToReset(port, pin,
  gpioPortResetValues(port))`, with no change in behaviour. `gpioDeinit()`
  finds those values by comparing `port` with `GPIOA`/`GPIOB`, so on the
  host (a fake register block at no real address) it always restored
  zeros and the part that keeps the debugger connected, putting PA13-15
  and PB3/PB4 back into alternate-function mode with their pulls and speed,
  was never exercised: a mistake there would have left 100% line and branch
  coverage intact. Public for the same reason as `gpioPortResetValues()`.
  This is the seam the earlier "not justified yet" note was waiting for: a
  wrong SWD restore costs the debugger connection.

- Tests only, from the full review: tests that could not catch the
  regression they were written for now can. `rccSysclkSwitch` is tested
  with `CFGR.SW` starting at a stale value (its test preloaded SW to 0
  despite a comment saying all-1s, so an OR in place of a replace passed),
  including the PLL-to-HSI switch-back; `rccBusPrescalerConfig` with
  HPRE/PPRE1/PPRE2 starting at all-1s; `gpioDeinitToReset` with GPIOA's and
  GPIOB's real reset values and with three distinct values to catch fields
  taken from the wrong register; each `USART_SR` flag and each
  `RCC_AHB1ENR` GPIO enable bit is pinned to its own literal (the existing
  tests only showed they filled a range, so a swap passed). Each of these
  was checked by introducing the bug and watching the new test fail.
  `test_bsp_clock_configuration_respects_pll_limits` is removed: it
  repeated the `_Static_assert`s in `bsp.h` and could not fail separately.
  A new BSP test derives the frequencies from the PLL and prescaler fields
  `bspClockInit()` wrote and compares them to `BSP_SYSCLK_HZ`/`BSP_PCLK1_HZ`/
  `BSP_PCLK2_HZ`, which the `_Static_assert`s could not see because bsp.c
  programs its register values from separate literals.
  `test_uart_driver_transmit_sends_every_byte_in_order` and
  `..._receive_reads_every_byte_in_order` are renamed
  `..._completes_a_multi_byte_buffer` and `..._fills_every_element`, and
  their comments say what a one-word fake DR can show; the init test no
  longer claims it observes UE being cleared first; the duplicate
  `test_uart_driver_transmit_applies_timeout_to_closing_flush` is removed.

- `Makefile`, `.github/workflows/ci.yml`: warnings are enforced. The
  firmware and the host test builds used `-Wall -Wextra` with only
  `-Werror=unused-result` fatal, so a new warning of any other kind passed
  CI; the `-Wshadow -Wconversion -Wundef` claim in the `test_nvic.c` entry
  below was likewise checked by nothing. Both builds now share one
  `WARN_FLAGS` set, `-Wall -Wextra -Wshadow -Wconversion -Wundef -Werror`;
  the tree already built clean under it at -O0 and -O2 (arm-none-eabi-gcc
  15.2 and Apple clang). CI also builds `BUILD=release` (-O2) on every
  run, so an optimiser-only warning shows up on the pull request and not
  for the first time in the release workflow.

- Docs only, from the full review: `CONTRIBUTING.md` gains a Concurrency
  section classifying every driver function as single-store (safe on
  distinct bits), read-modify-write on a shared register (caller must
  serialise) or single-owner, since the drivers had no stated contract and
  most of them read-modify-write registers shared across peripherals;
  `pwrEnableOverdrive()`'s precondition now states ST's HAL note that no
  other peripheral clock should be enabled during the over-drive switch
  (which `bspClockInit()` satisfies); and the README lists the known
  limitations the review found and left alone (no `VTOR`/`.init_array` in
  startup, the PLLR SYSCLK source, `uartInit()`/`uartDeinit()`/
  `systickInit()` edge behaviour, no unwind in `bspClockInit()`).

- Header and comment hygiene, from the full review. Annotations and types:
  `USART_SR_RXNE` is write-0-to-clear (rc_w0), not "READ only";
  `NvicRegisters_t::IABR` is `volatile const` like every other read-only
  register; `FPU_FPCCR_LSPACT` is "lazy state preservation active".
  Includes: `gpio.h`, `flash.h`, `pwr.h`, `rcc.h` and `uart.h` include
  `<stdint.h>` for the types they use instead of relying on their register
  header, and `bsp.h` drops one it never used. Comments: `gpioSetAlternateFunction`'s
  `@return` now lists the pin check, `I2SSCR` is `I2SSRC`, the GPIO value
  macros all name their register, the SYSCFG EXTICR bit-position wording and
  a stale "top-of-file note" cross-reference are fixed, the flash header
  lists `RDERR` among the omitted SR bits, and `bsp.h` says its limits are
  `_Static_assert`s. `Makefile`, `device/inc/rcc_reg.h` and
  `tools/openocd.cfg` gained their missing final newline.

- Release process and setup docs, from the full review. `docs/VERSIONING.md`
  now describes how releases were actually cut (a `release/vX.Y.Z` branch
  into `main`, the changelog link references, the UTC-date convention, the
  strict "up to date" branch protection, and the sync-back PR into
  `develop`), where it described a `develop`->`main` PR and omitted the last
  two; `CONTRIBUTING.md` agrees. The README lists `arm-none-eabi-gdb` and
  `nc` (needed by `make debug`) and the CI tool versions (clang-format 18,
  Doxygen 1.18.0, cppcheck 2.13.0, Arm 15.2.rel1) with the warning that other
  versions can fail `make format-check`/`make docs`, and mentions the
  `flash.c` coverage exclusion next to the "100% coverage" claim.
  `SECURITY.md` no longer calls `bsp/` empty. `groups.dox`/`Doxyfile` order
  the layers bottom-up (`drivers` -> `bsp` -> `api`) like ARCHITECTURE.
  `release.yml`'s PR dry run now also triggers on `Doxyfile` and
  `CHANGELOG.md` - the two files a release PR changes, which it previously
  skipped. `ci.yml` runs on `ubuntu-24.04` (its apt pins are for that
  release; `ubuntu-latest` would break them when the alias moves) and passes
  the PR base ref through `env:` instead of interpolating it into the script.

- Tests only: closed the remaining gaps the review's contract matrix found
  between each driver's documented contract and what the tests pin (29
  tests; the mutations below each passed the old suite and fail now).
  `gpioInit` places every field on every pin with distinct non-zero values
  (including `GPIO_MODE_ANALOG`, never accepted by a test before), rejects
  each bad argument without touching any of the four registers, and
  `gpioWritePin` writes BSRR exactly (not OR-ed) and leaves ODR alone;
  `gpioTogglePin` reads ODR, not IDR. AF selection is tested on all 16 pins
  at AF 0 and 15 (pins 7/8 straddle AFRL/AFRH). RCC: `rccPllConfig` with an
  HSI source clears PLLSRC, `rccHseEnable` in crystal mode clears a stale
  HSEBYP, the enables preserve every other CR bit, each `rccSysclkSwitch`
  source checks its own ready bit. UART: a rejected `uartInit` leaves a
  running peripheral untouched, STOP 2 -> 1 clears the field, BRR rounds to
  nearest (down and tie cases) without 32-bit overflow, non-fault SR bits
  are not errors, a NULL output is rejected before waiting, `uartReceive`
  stops at the first fault, waits are flag-specific. NVIC: `nvicSetPriority`
  overwrites and leaves every neighbouring IP byte. PWR/flash/SysTick:
  flag-specific waits, other ACR bits preserved, `systickInit` rejections
  leave VAL untouched. `bspClockInit`: PLL enabled before the VOSRDY wait,
  PWR clock first, the documented state after a switch timeout, and no
  plain retry. Still not observable through a single-register fake: write
  *ordering* inside a function and UART byte order.

- Tests only: every register header now has a test with literal values for
  what the drivers rely on, taken from ST's SVD/HAL and ARM's CMSIS rather
  than built from the header's own macros. Before, the field-value macros
  were only tested for distinctness, so swapping `PWR_CR_VOS_SCALE1` and
  `SCALE3` (which would under-volt the 180 MHz bring-up), or exchanging
  `GPIO_MODE_AF` and `GPIO_PUPD_DOWN` (both 0x2), passed every test. Now
  pinned: the VOS/SYSCLK/HPRE/PPRE/PLLP/PLLSRC/GPIO/EXTICR/STOP/AP encodings,
  `Msk` values for the fields the drivers shift, the `CR`/`CSR`/`ACR`/`CTRL`
  bits the drivers touch, every base address and pointer macro that was
  unpinned (`GPIOA`..`GPIOH`, `USART1`..`UART5`, RCC, FLASH, PWR, EXTI,
  SYSCFG, IWDG, WWDG, NVIC, SCB, FPU, MPU, SysTick), `sizeof(RccRegisters_t)`
  and the offset of every member after a RESERVED gap in each struct. The
  constants `startup_stm32f446re.s` hard-codes (CPACR address, FPU enable
  mask, AIRCR address and the PRIGROUP=3 write) have `scb_reg.h`'s matching
  macros pinned to the same literals by a test. Nothing reads the `.s` file,
  so editing a literal there alone is not caught; the pins catch the header
  drifting from the manual. Every literal agreed with the existing macros, so
  this found no wrong value - it makes the next wrong header value fail. The EXTI line numbers
  16-22 are regression pins only: neither the SVD nor ST's headers define
  them.

- `drivers/{inc,src}/uart.{h,c}`: the TXE/TC/RXNE waits no longer use a
  fixed 100000-poll count; `uartFlush`, `uartTransmitByte`, `uartTransmit`,
  `uartReceiveByte` and `uartReceive` take a caller-supplied `timeout`
  (SR polls; per byte for the buffer functions). A fixed count is about
  5-9 ms at 180 MHz -O2 but 11x longer at 16 MHz and 2-3x longer at -O0, so
  it timed out spuriously at low baud rates (a TC wait is two frame times:
  8 ms at 2400 baud) and whenever a received byte arrived more than a few
  milliseconds after the call - which made an echo loop unusable. A receive
  wait depends on when the sender next speaks, not on the baud rate, so only
  the caller can size it. `timeout == 0` polls once (a non-blocking check);
  `UINT32_MAX` is an effectively unbounded wait. `uart.h` documents how to
  size one. API change, but nothing in the repo calls these yet. Per-poll
  cost figures are estimates, not measured on hardware.

- Docs and comments brought in line with the code after the driver and BSP
  work: `docs/ARCHITECTURE.md` (the drivers box omitted Flash and PWR; bsp/
  is no longer only "pin mapping"; `tools/install_cppcheck.sh`),
  `CONTRIBUTING.md` (layering diagram now `drivers` -> `bsp` -> `api` ->
  `app`, matching ARCHITECTURE; `if (status)` reasoning; coverage filter
  now `drivers/src` + `bsp/src`; the four workflows; the standalone-command
  list), `README.md`/`docs/VERSIONING.md`/`docs/mainpage.md`/`docs/groups.dox`
  (drivers are complete; "0.2.0 is reserved" reworded; BSP described),
  `SECURITY.md` (shared composite actions, graphviz unpinned on purpose),
  `driver_status.h` (`DRIVER_STATUS_ERR_BUSY` is returned by `rcc.c` - it
  is not reserved for non-blocking drivers), `uart.h` (no longer "8N1",
  nvic.h now exists), `rcc.h` (CR.VOS resets to 0b11, so "reset default
  scale 3" was wrong: scale 3 is what runs while the PLL is off; steps 2
  and 3 are interchangeable), `pwr.h` (over-drive), `bsp.h` (the state a
  failed `bspClockInit()` leaves behind and that it cannot be retried), and
  the Makefile, workflow and Doxyfile comments that still described
  `bsp/` as empty, `pwr.c` as caller-less, or `clean` as only deleting
  `build/`. `make lint`'s suppression rationale is rewritten against what
  the pinned cppcheck 2.13.0 actually reports: the `misra-c2012-8.7`
  suppressions on rcc.c, pwr.c, nvic.c, systick.c and bsp.c and four inline
  `misra-c2012-12.2` suppressions in nvic.c were not needed and are removed.
- `bsp/inc/bsp.h`: the PLL VCO and APB/SYSCLK limits are now
  `_Static_assert`s, so an out-of-range `BSP_*` constant fails any firmware
  build, not only `make test`.

- Tests only: closed the gaps a mutation pass over `drivers/` and `bsp/`
  found (each mutation passed all 207 tests before, and is caught now).
  `bspClockInit` tests assert CR.VOS was written before the PLL stage
  (the ordering bug CONTRIBUTING.md says shipped once) and cover the
  PLL-already-on `BUSY` case; `gpioInit` is tested with non-zero field
  values in every register; `rccPllConfig` is tested at M 2/63, N 50/432
  and PLLP DIV8; `uartInit`'s BRR bounds are tested at exactly 16, 65535
  and one past each; a tautological assertion in `test_pwr.c` was removed.
  Not closed: `uartTransmit`/`uartReceive` byte *order* is not observable
  through a single-register fake DR (the tests that claimed it are renamed
  below). `gpioDeinit`'s SWD reset-value restore was also out of reach here
  because `gpioPortResetValues()` identifies a port by its real address; it
  is tested now through `gpioDeinitToReset()` (see below).

### Fixed

- `tools/check_test_registration.awk`: two more ways to get "test compiles,
  never runs" past it, found by trying them. A `RUN_TEST(test_x);` inside a
  `/* ... */` block or an `#if 0` region still counted as a live registration
  (only a `//` comment or a line starting with `*` was excluded), and a
  definition written as `void` on one line and `test_x(void)` on the next was
  invisible to both the defined-test and the odd-signature patterns. Every
  rule now sees the line with line comments, block comments (one line or
  many) and `#if 0` regions removed, so a commented-out registration is
  reported as `MISSING RUN_TEST` and a test defined inside one as a
  `RUN_TEST` for a test nothing defines, and a column-0 `test_x(` is
  reported as an unrecognised signature. Checked against ten hand-made
  variants of the real test tree (BWK awk; not run under mawk, which CI's
  Ubuntu image uses). The file mode is now 644 like the other awk scripts.

- `Makefile`: plain `make` built nothing. The flags-stamp rule is the first
  target in the file, so it was the default goal and `all` was never
  reached; the README's "`make` / `make all`" was wrong, and CI, the
  pre-commit hook and the release workflow all hid it by calling `make all`.
  `.DEFAULT_GOAL := all` is now set explicitly.

- `Makefile`: `make debug` attached GDB to whatever image was already on the
  board, so the symbols of the ELF just built could silently disagree with
  the running code, and `Reset_Handler` could not be stepped. After
  attaching it now runs `monitor reset halt`, `load`, `monitor reset halt`.

- `startup/startup_stm32f446re.s`: `Default_Handler` was a `NOTYPE` symbol
  with an even address (`0x0800024c`). The vector table only ever used the
  `.thumb_set` aliases, which do carry the Thumb bit, so nothing was broken,
  but any direct reference to the global symbol (a fallback handler pointer
  in the kernel, say) would have loaded an even address and taken an
  INVSTATE UsageFault. It is now `.type Default_Handler, %function` (address
  `0x0800024d`); the vector table words are unchanged.

- Tests only: `test_nvic.c` passed `1UL << n` (64-bit on a 64-bit host) to
  32-bit parameters, a `-Wconversion` warning; every test file now compiles
  with `-Wall -Wextra -Wshadow -Wconversion -Wundef` and no warnings.

- `tools/`: the repo's own consistency checks are harder to fool (full
  review). `check_test_registration.awk` counted a commented-out
  `// RUN_TEST(test_x);` as registered, which is exactly the "test compiles,
  passes, never runs" failure it exists to catch: only a live statement
  counts now, a duplicate registration, a commented-out one, and a test
  function with a signature other than `void test_x(void)` (invisible to
  the strict pattern) are each reported, and finding no tests at all fails.
  `check_vector_table.awk` only compared the 97 peripheral slots; it now
  also checks the 16 architecturally fixed core words and that each
  negative `IRQn_e` entry (SVCall, PendSV, SysTick, MemManage, BusFault,
  UsageFault) names the vector word it claims - a swapped SVC/PendSV/SysTick
  slot used to pass - and that the table has exactly 97 peripheral slots
  and no duplicate IRQn. `install_cppcheck.sh` stages the install and swaps
  it into place only after the binary passes a real cppcheck+MISRA run
  (previously an interrupted run could leave a `bin/cppcheck` without its
  `cfg/` that `make lint` then trusted), restoring the previous install on
  failure, and cleans up on SIGINT/SIGTERM. New `make hooks` sets up the
  pre-commit hook; README/CONTRIBUTING no longer say `chmod +x` (the file is
  already executable), and now document that the hook checks the working
  tree rather than the index and that `git commit --no-verify` skips it.
  Each awk change was checked against a deliberately broken copy.

- `Makefile`: the build now tracks what it was silently ignoring (found by
  the full review). **Flags:** after a `BUILD=debug` build, `make
  BUILD=release` printed "nothing to be done" and left the -O0 image to be
  mistaken for a release one, and a partial edit linked a mix of -O0 and -O2
  objects; the objects and the ELF now depend on a `build/.flags` stamp that
  is rewritten only when the compile/link command lines change, and the ELF
  also depends on the linker script (editing `STM32F446RE.ld` did not
  relink). **Host builds:** `make test` and `make coverage` had no header
  dependency tracking, so editing a driver or register header could leave
  stale test objects and a green run; they now use `-MMD -MP` like the
  firmware build. **Coverage:** `make coverage` deletes leftover `.gcda`
  files before running (counters accumulate, so a removed or broken test
  could keep showing hits and let the 100% gate pass; CI's fresh checkout
  was never affected, local runs were). Also: `make re` no longer races
  under `make -j` (`clean` and `all` are now sequenced), source discovery is
  `$(sort)`ed so the link order does not depend on the filesystem,
  `.DELETE_ON_ERROR` stops a failed `objcopy` leaving a truncated `.bin`
  that looks up to date. The flags stamp compares mtimes, so macOS make
  3.81's 1-second resolution can miss a profile switch made within a second
  of the previous build - only reachable from a script. Verified by
  switching profiles, touching the linker script and headers, and running
  `make -j4 re`.

- `drivers/src/nvic.c`: `nvicDisableIrq()` now issues `DSB`+`ISB` after the
  ICER write, and `nvicEnableIrq()` brackets the ISER write with compiler
  barriers, as CMSIS' `__NVIC_DisableIRQ()`/`__NVIC_EnableIRQ()` do. Without
  the DSB the ICER write can still be in the write buffer when the caller
  continues, so an interrupt that was already pending could be taken after
  the function returned and break the usual "disable, then touch state the
  ISR also uses" pattern; without the compiler barrier an inlined or LTO
  build could move stores to ISR-shared state past the enable. Host builds
  get a compiler barrier only (there is no NVIC to synchronise); the
  `dsb`/`isb` instructions are confirmed in the target object. The
  behaviour is not observable in the in-memory fake, so no new test.

- `drivers/src/rcc.c`: `rccHsiDisable()`, `rccHseDisable()` and
  `rccPllDisable()` now wait (bounded, like the enables) for the ready flag
  to follow the ON bit down, and return `DRIVER_STATUS_ERR_TIMEOUT` if it
  does not. The flag drops a few cycles after the ON bit, so a quick
  disable-then-enable read a stale `HSERDY`/`PLLRDY` and reported success
  before the oscillator had restarted; with the PLL that also let
  `rccSysclkSwitch()`'s ready check pass on a stale `PLLRDY`. ST's HAL waits
  for the flag to clear in all three paths (`HAL_RCC_OscConfig`). Not on
  the `bspClockInit()` path (it starts from reset), so this affects any
  later reconfiguration. A timed-out `rccHseEnable()`/`rccPllEnable()`
  still leaves its ON bit set (a retry reports `BUSY`); `rcc.h` now says
  to call the matching disable first. Six tests (each oscillator: success
  preserves every other CR bit; a ready flag that never clears times out).

- `drivers/src/flash.c`: a comment containing the coverage-exclusion
  marker text made gcovr see a second, never-closed exclusion region and
  warn on every `make coverage`; the comment no longer spells the markers
  out (coverage is unchanged at 100%).

- `drivers/src/rcc.c`: every `rcc*ClockEnable()` now reads its enable
  register back after setting the bit, so the clock is running before the
  caller's first access to the peripheral (e.g. `gpioInit()` straight
  after `rccGpioClockEnable()`). ST's HAL does the same in each
  `__HAL_RCC_*_CLK_ENABLE` macro ("Delay after an RCC peripheral clock
  enabling"), which is what this follows; ST's errata sheet itself was not
  checked, and the effect cannot be observed in the host tests.

- `drivers/src/uart.c`: `uartTransmit()`/`uartReceive()` with a NULL buffer
  and a non-zero length, and `uartReceiveByte()` with a NULL output, now
  return `DRIVER_STATUS_ERR_INVALID_PARAM` instead of dereferencing NULL,
  as CONTRIBUTING.md's error-handling contract already requires of every
  runtime parameter outside its domain (`nvicGetPriority()` already did).
  A zero length with a NULL buffer is still a successful no-op. Documented
  in `uart.h` and covered by three tests.

- `drivers/src/nvic.c`: the NVIC functions now reject the reserved vector
  positions inside 0 to `FMPI2C1_Error_IRQn` (61-62, 79-80, 82-83, 85-86,
  88-90) with `DRIVER_STATUS_ERR_INVALID_PARAM`, where #99 accepted them
  as "architecturally harmless". Enabling and then pending one vectors
  through the zero in startup's vector table, which is a fault. The tests
  for the write-1-to-act banks now preload every ISER/ICER/ISPR/ICPR word
  with a pattern, so a read-modify-write (`ICER |= bit`, which on silicon
  disables every enabled line) is caught; the system-handler tests
  preload SHPR1-3 so a wrong byte-lane mask is caught. Both were found by
  mutation testing.

- `device/inc/iwdg_reg.h`: removed `WINR` (0x10), `IWDG_SR_WVU` and
  `IWDG_WINR_WIN_*`, which the F446's IWDG does not have; the struct is
  0x10 bytes, not 0x14. ST's F446 SVD and `stm32f446xx.h` both define only
  KR, PR, RLR and SR (the window option is a feature of other STM32
  families). Nothing in the repo used them. RM0390 section 21 itself was
  not available to check.

- `uartInit()` accepted any computed BRR from 1 upward and rejected only 0.
  BRR holds USARTDIV x 16, and USARTDIV below 1 cannot be generated: with
  16x oversampling the highest baud rate is pclk / 16, which is BRR 16. A
  baud rate above that (e.g. 8 MHz at 1 Mbaud, BRR 8) returned
  `DRIVER_STATUS_OK` and configured a baud rate the peripheral cannot
  produce. It now returns `DRIVER_STATUS_ERR_INVALID_PARAM` for any BRR
  below 16. The old test that asserted BRR 1 was accepted is replaced by
  one asserting BRR 16 is the smallest valid value and one covering 1, 8
  and 15.
- `GPIOA_OSPEEDR_RESET` was `0x0C000000` (PA13 at very high speed), a value
  that belongs to other STM32F4 variants such as the F401/F411. ST's own
  STM32F446 SVD, which lists a reset value for every register, has GPIOA's
  `OSPEEDR` at `0x00000000` (GPIOB's `0x000000C0` and the other four
  non-zero values were right). Left in, `gpioDeinit()` on PA13 would have
  written a very-high-speed setting the chip never resets to. The constant is
  removed, `gpioPortResetValues()` reports 0 for it, and a test pins it. This
  corrects the reset table added in v0.1.5.

## [0.1.5] - 2026-10-08

### Added

- `.github/workflows/release.yml`: pushing a `vX.Y.Z` tag now checks that
  the tag, the Doxyfile `PROJECT_NUMBER` and `CHANGELOG.md`'s newest release
  agree and that the commit is on `main`, builds the release firmware with
  the pinned toolchain, and creates the GitHub Release with that version's
  changelog section as the notes and the `.elf`/`.bin`/`.hex`/`.map`, a
  size report and `SHA256SUMS` attached. Releases were previously created
  by hand, and v0.1.3 and v0.1.4 never got one. On a pull request touching
  the release pipeline's files it runs the same steps without publishing,
  so the first real run is not a real release.
- `make check-version` (`tools/check_version.awk`), also run in `ci.yml` on
  every PR and push: fails when the Doxyfile's `PROJECT_NUMBER` and the
  newest released heading in `CHANGELOG.md` differ. The two are edited by
  hand in different files at release time and nothing else compared them.
- `tools/changelog_section.awk`: extracts one version's changelog section
  for the release notes; refuses an empty or missing section.

### Changed

- Documentation brought back in line with the code: the README, ARCHITECTURE
  and VERSIONING peripheral/driver lists now name every register block
  (MPU on the core side; EXTI, Flash, IWDG, PWR, SYSCFG, WWDG on the device
  side) and every implemented driver (Flash and PWR were missing from the
  0.2.0 definition); the README's one-line pitch no longer reads as if a
  scheduler exists; the GPIO and UART driver summaries describe
  `gpioPortResetValues` and parity; `docs/ARCHITECTURE.md` lists all four awk
  tools. The `gcovr` prerequisite now says to use Homebrew on macOS: the
  pinned 8.6 needs Python 3.10, and the Command Line Tools ship 3.9, so the
  documented `pip install gcovr` fails there.
- `CHANGELOG.md` now ends with the Keep a Changelog comparison links, so each
  version heading links to its diff.

- The toolchain and Doxygen install steps, previously copy-pasted across
  `ci.yml`, `codeql.yml` and `pages.yml`, now live in two composite actions
  under `.github/actions/` (`setup-arm-toolchain`, `setup-doxygen`), so each
  version and checksum is defined once. Cache keys now include the start of
  the pinned checksum.
- Every job has a `timeout-minutes` ceiling (20 for CI and CodeQL, 15 and 10
  for the Pages build and deploy) instead of GitHub's 6-hour default, so a
  hung download fails fast instead of holding a required check pending.

### Fixed

- The Makefile discovered assembly with `-name '*.s'` and had only a `%.s`
  rule, so `rtos/kernel/src/context_switch.S` (uppercase, preprocessed) was
  silently never assembled or linked. Left unfixed, the PendSV context
  switch would have built cleanly, with `PendSV_Handler` still the weak
  `Default_Handler` loop. Both `.s` and `.S` are now discovered, `.S` gets
  its own rule that runs the C preprocessor with the project's include
  paths, and the empty `context_switch.S` placeholder is now part of the
  link.
- `make debug` backgrounded OpenOCD and started GDB immediately, so GDB's
  single `target extended-remote :3333` could run before OpenOCD (which
  must open the ST-LINK and halt the target first) was listening and fail
  with "Connection refused". It now waits for the port, and stops with a
  clear message if OpenOCD exits (no board, ST-LINK busy) or does not
  listen within ~10 s.
- Stale comments: `make lint`'s said the drivers were still empty and
  would find nothing; the lint-suppression note listed `CR1.OVER8` as
  having no consumer, which stopped being true when `uartInit()` started
  clearing it; `startup_stm32f446re.s` named itself `.S`.
- `rccHsiDisable()` and `rccHseDisable()` only refused when `CFGR.SWS`
  named the oscillator itself. RM0390 also forbids clearing `HSION`/`HSEON`
  when the oscillator is used *indirectly* as the system clock, i.e. `SWS`
  names the PLL and `PLLCFGR.PLLSRC` selects that oscillator. In that case
  the hardware ignores the write while the driver reported
  `DRIVER_STATUS_OK`. Both now share one guard
  (`rccOscillatorDisableGuard`) and return `DRIVER_STATUS_ERR_BUSY` for the
  indirect case too. An active PLL sourced from the *other* oscillator does
  not block the disable.
- `gpioDeinit()` zeroed the pin's MODER/OSPEEDR/PUPDR fields and documented
  that as the power-on-reset state, which is only true for most pins. GPIOA
  and GPIOB come out of reset with the SWD/JTAG pins (PA13/14/15, PB3/4) in
  alternate-function mode with pulls, so deinitialising one of them dropped
  the debug link. It now restores the real reset value for the pin's
  field. The per-port values live in `gpio_reg.h` (`GPIOA_MODER_RESET` etc.)
  behind a new `gpioPortResetValues()`, which identifies the port by address
  only so the lookup is host-testable.
- `uartInit()` corrupted data whenever parity was enabled. `CR1.M` was
  always cleared, but RM0390 counts the parity bit inside the frame length
  `M` selects, so parity on with `M` clear is 7 data bits plus parity: the
  hardware replaced bit 7 of every transmitted byte with the parity bit,
  and a receiver would have discarded it. `M` is now derived from the
  parity setting (set exactly when parity is enabled), giving the 8 data
  bits plus parity the API always advertised. Transmit/receive stay
  `uint8_t`: the 9th bit is parity, generated and checked by hardware.
- `uartInit()` now clears `CR1.OVER8`. The BRR it computes assumes 16x
  oversampling, so a stale `OVER8` (set by earlier code or a debugger)
  silently doubled the baud rate the same divisor produced.

### Security

- The Arm GNU Toolchain and Doxygen archives are now verified against a
  pinned SHA256 before extraction. They were previously fetched with
  `curl -sSL ... | tar` and no integrity check, which undercut the
  SHA-pinning policy applied to the GitHub Actions themselves: a changed or
  tampered download would have been installed silently. A mismatch now
  fails the job. `curl` also uses `-f`, so an HTTP error fails the step
  instead of handing an error page to `tar`.

## [0.1.4] - 2026-09-30

### Added

- `tools/check_test_registration.awk`, run by `make test`: cross-checks
  every test function defined in `tests/unit/*.c` against the `extern`
  declaration and `RUN_TEST()` call it needs in `test_runner.c`. Unity
  allows one `main()`/`setUp()`/`tearDown()` per binary, so that file
  reaches every test through a hand-maintained pair - and a test missing
  either half still compiles and the suite still passes green, it just
  never runs. `make coverage` would catch that for a driver test, but its
  filter is `drivers/src` only, so for the 13 `test_<peripheral>_reg.c`
  files a forgotten registration was entirely silent. Same spirit as the
  existing `check_vector_table.awk`: two hand-written lists, no shared
  source of truth, nothing else watching them drift. All 142 tests pass
  the check today; it was verified to fail on a dropped `RUN_TEST` and on
  a dropped `extern`+`RUN_TEST` pair before being wired in.
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

- `Makefile`: `make debug` backgrounded OpenOCD and never reaped it, so
  the GDB server outlived the session that started it, kept holding the
  ST-LINK, and made the next `make flash` or `make debug` fail with a
  device-busy error until it was killed by hand. It now traps EXIT/INT/
  TERM and kills the server it started.
- `.github/workflows/codeql.yml`: both `github/codeql-action` pins
  carried a `# v4` comment while actually pinning `v4.37.8`, and the
  floating `v4` tag has since moved to a different commit - so the
  comment named something the workflow was not running. That is the exact
  drift SHA pinning exists to prevent, reintroduced in the part a human
  reads. Both now name the exact release. Audited every other pin in the
  process; `actions/checkout`, `actions/cache`, `actions/deploy-pages`
  and `actions/upload-pages-artifact` all resolve correctly.
- `SECURITY.md`: the action-pinning section illustrated the convention
  with `actions/checkout@11d5960... # v4.4.0`, which is not what this
  repository pins (`3d3c42e5... # v7.0.1`). A security document whose
  concrete example does not match the code undermines the document. It
  also now states that the version comment must name an exact release
  rather than a floating major.
- `docs/VERSIONING.md`: the policy contradicted itself on
  `PROJECT_NUMBER`, saying in one place that it is "deliberately left
  blank between releases" and in another that it is bumped on the release
  PR and reports the released version until the next one. The Doxyfile
  follows the second, so the first was removed.
- `drivers/src/gpio.c`: `gpioInit()` and `gpioSetAlternateFunction()` now
  validate `pin`. Both already rejected every other out-of-domain
  parameter but took `pin` on trust, and `GpioPin_e` does not constrain
  an enum-typed parameter to its enumerators - a caller can cast any
  integer in. Every field this driver touches is indexed by the pin
  number, so `(GpioPin_e)16` meant shifting a `uint32_t` by 32: undefined
  behaviour, not merely a wrong register write. The four functions that
  return `void`/`GpioPinState_e` have no failure path to report a bad pin
  and are unchanged; they now carry an explicit `@pre` instead, matching
  how this project documents preconditions it cannot check.
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
- **`drivers/src/rcc.c`: `rccSysclkSwitch()` applied the target flash
  latency unconditionally before writing `CFGR.SW`, on the reasoning
  (stated in both `drivers/inc/flash.h` and `drivers/inc/rcc.h`) that a
  higher latency is always electrically safe to apply early. That
  reasoning only holds for a *raising* change - on a switch to a lower
  target latency (e.g. dropping from the PLL at 180 MHz back to HSI),
  the old code lowered `ACR.LATENCY` while SYSCLK was still running at
  the higher frequency the old, higher latency was rated for. Every
  fetch between that write and the switch actually landing - including
  the function's own `CFGR.SW` write - executed with too few wait
  states, silently corrupting instruction fetch per `flash_reg.h`'s own
  documented failure mode; in practice this HardFaults or hangs the core
  before the switch is ever reached.** Found in review, not on hardware -
  the driver's off-target tests held 100% branch coverage throughout,
  same as the PWR sequencing bug above, because nothing in a host test
  represents "this instruction fetch happened at the wrong flash
  latency." The fix makes the ordering direction-dependent: raising
  `ACR.LATENCY` still lands before `CFGR.SW`, as before, but lowering it
  is now deferred until *after* `CFGR.SWS` confirms the switch completed,
  and is skipped entirely (leaving the higher, still-safe value in place)
  if the switch times out. `drivers/inc/flash.h` and `drivers/inc/rcc.h`
  now document the asymmetry instead of asserting the switch is safe
  "regardless of direction," which was the sentence that hid the bug.
  New tests in `tests/unit/test_rcc.c` cover both the lowering-succeeds
  and lowering-deferred-on-timeout cases, plus a `CFGR` non-interference
  test for `rccSysclkSwitch()` and field-placement assertions (not just
  status) for the `rccBusPrescalerConfig()` HPRE/PPRE exhaustive-value
  tests, none of which previously asserted anything beyond
  `DRIVER_STATUS_OK`.
- **`drivers/src/uart.c`: `uartInit()` accepted `USART_CR2_STOP_0_5` and
  `USART_CR2_STOP_1_5`, two real RM0390 `CR2.STOP` encodings this driver
  had no way to honor, and reported `DRIVER_STATUS_OK` while doing so.**
  Both encodings exist for smartcard mode - `uart_reg.h`'s own
  file-level comment already scopes `CR3`'s smartcard bits (`SCEN`/
  `IREN`/`IRLP`) as omitted, since this driver never enables that mode -
  and RM0390 additionally documents both as unavailable specifically on
  UART4/UART5. A caller requesting either got a framing configuration
  silently different from what it asked for, the same class of bug the
  9-bit-mode (`CR1.M`) parameter had before it was removed. `uartInit()`
  now validates `stop_bits` against only `USART_CR2_STOP_1` and
  `USART_CR2_STOP_2`; `uart.h`'s file-level comment states why, in the
  same shape as the existing 9-bit-mode paragraph.
  `device/inc/uart_reg.h`'s `USART_CR2_STOP_0_5`/`_1_5` macros stay
  defined (RM0390 documents them; a future consumer implementing
  smartcard mode would need them) but are now commented as unused by
  this driver, covered by the file's existing `misra-c2012-2.5`
  suppression - the Makefile's suppression-rationale comment is updated
  to name them alongside the other fields it already lists as modeled
  for completeness with no current consumer.
  `tests/unit/test_uart.c`: the exhaustive `stop_bits`-acceptance test
  now covers only the 2 supported values (down from asserting all 4
  reported `DRIVER_STATUS_OK`, which was itself wrong) and additionally
  asserts each lands in `CR2.STOP`, not just the return status; a new
  `test_uart_driver_init_rejects_smartcard_only_stop_bits` is the direct
  regression test, asserting both smartcard-only encodings are now
  rejected and `CR2` is left untouched.
- **`drivers/src/uart.c`: `uartComputeBrr()` simplified from a manual
  mantissa/fraction computation to a single correctly-rounded division,
  fixing a real (if minor) rounding-accuracy bug in the process.** With
  `OVER8` clear, `BRR`'s own bit layout (`USART_BRR_DIV_MANTISSA` at bits
  15:4, `USART_BRR_DIV_FRACTION` at bits 3:0) already *is* the binary
  representation of `USARTDIV * 16`, and `USARTDIV * 16` collapses
  algebraically to `pclk_hz / baud` - so `BRR = round(pclk_hz / baud)`
  directly, with no mantissa/fraction split or repacking needed. The
  previous version computed `USARTDIV*100` via integer division
  (truncating to 2 decimal digits of the true ratio) before rounding
  that truncated value to the nearest 16th; truncating before rounding
  could shift the result away from the true nearest integer. Concretely,
  for a 42 MHz bus clock at 115200 baud the true ratio is 364.5833
  (correctly rounds to 365), but the old code produced 364 - one off, in
  the wrong direction. The new formula computes 365 directly. Every
  operand of the round-to-nearest `(a + b/2) / b` idiom is now explicitly
  cast to the 64-bit intermediate (MISRA C:2012 Rule 10.7 - cppcheck
  flagged the first draft, which relied on C's implicit promotion instead
  of an explicit cast on `baud / 2U` and the final division).
  `tests/unit/test_uart.c`: all four existing BRR test vectors are
  unchanged as regression vectors (391, 1667, 976, 1), reworded where
  their comments described mantissa/fraction mechanics this function no
  longer has; a new `test_uart_driver_init_computes_brr_rounds_
  correctly_past_old_truncation_bug` (the 42 MHz/115200 -> 365 case) is
  the direct regression test for the accuracy fix; the invalid-baud
  boundary tests' comments were reworded to describe the bound in terms
  of `BRR`'s 16-bit field directly instead of a separate 12-bit mantissa
  field. `make coverage` holds 100% line/branch on the smaller function.

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

[Unreleased]: https://github.com/ka5j/stm32_rtos/compare/v0.1.5...HEAD
[0.1.5]: https://github.com/ka5j/stm32_rtos/compare/v0.1.4...v0.1.5
[0.1.4]: https://github.com/ka5j/stm32_rtos/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/ka5j/stm32_rtos/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/ka5j/stm32_rtos/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/ka5j/stm32_rtos/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/ka5j/stm32_rtos/releases/tag/v0.1.0
