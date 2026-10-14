# Design notes

The reasoning behind the project's conventions, in one place. Code and the
other pages link here instead of repeating it.

## Register blocks as parameters

Every driver function takes its peripheral's register block as a parameter,
for example `gpioInit(GpioRegisters_t *port, ...)`. It never uses a hardware
macro such as `GPIOA` or `RCC` internally, even for a singleton like RCC.

- A host test can then pass a plain in-memory struct in place of the
  hardware, which is what lets `make coverage` gate drivers at 100%.
- To identify one instance among several, pass the existing block pointer
  (`GPIOA`, `GPIOB`, ...) rather than inventing a parallel enum. See
  `rccGpioClockBit()` in `drivers/src/rcc.c`.

## Error contract

Drivers report failure with `DriverStatus_e` (`drivers/inc/driver_status.h`):
a small, closed set of failure categories used for control flow at the call
site.

- A function that can genuinely fail returns `DriverStatus_e` and is marked
  `DRIVER_MUST_CHECK`, so ignoring the result is a compile error. Failure
  means a bounded wait timed out, the hardware reported a fault, the
  peripheral was not initialised, a parameter is outside its domain, or the
  hardware is busy.
- A function with no failure mode (one deterministic register write, no
  polling) returns `void`. Never return a status defensively.
- Compare explicitly: `if (status != DRIVER_STATUS_OK)`. `DRIVER_STATUS_OK`
  is deliberately not 0: 0 is `DRIVER_STATUS_UNINITIALIZED`, so a status
  variable that a code path forgot to assign never reads as success.
- Exception: the GPIO functions that return `void` or a pin level
  (`gpioWritePin()`, `gpioReadPin()`, `gpioTogglePin()`, `gpioDeinit()`) do
  not validate the pin. They state the valid range as an `@pre`; an
  out-of-range pin is undefined behaviour.
- Do not add peripheral-specific values to `DriverStatus_e`. Add a
  per-module detail code returned alongside it only when a driver actually
  needs that granularity.

## const register parameters

A register-block parameter is `const` only when the call leaves the
peripheral unchanged, so it is repeatable and reorderable. `gpioReadPin()`
(IDR), `uartFlush()` (SR) and `pwrWaitVoltageScaleReady()` (CSR) qualify.

A read with a side effect is not `const`, even though C would allow it.
`uartReceiveByte()` reads DR, which clears SR.RXNE and the ORE/NE/FE/PE error
flags, so its parameter is non-`const` on purpose.

## Poll-count timeouts

Every hardware wait (HSIRDY, HSERDY, PLLRDY, SWS, VOSRDY, ODRDY, ODSWRDY, IWDG
PVU/RVU, and UART TXE/TC/RXNE) is a bounded number of register polls, not a wall-clock
timeout.

- During clock bring-up there is no timebase yet: SysTick's reload depends on
  the SYSCLK frequency being established. CMSIS's `SetSysClock()` does the
  same.
- The flag polled is always a different field from the one the driver
  writes, so a host test can hold it clear to exercise the timeout path.
- The cost: a count is not a time. It varies with optimisation level and
  clock speed. UART takes the count from its caller. A SysTick-based deadline
  replaces these once the kernel provides one.

## Clock bring-up order

RM0390 constrains the order, and `bspClockInit()` follows it.

- PWR_CR.VOS can only be written while the PLL is **off**, and CSR.VOSRDY
  only sets once the PLL is **on**. So the scale is selected before
  `rccPllEnable()` and awaited after it. Neither belongs inside
  `rccSysclkSwitch()`.
- Over-drive (180 MHz) is enabled after the PLL locks and before SYSCLK
  switches to it, following ST's own 180 MHz example.
- Flash latency is raised **before** the switch and lowered only **after**
  CFGR.SWS confirms it, so flash is never read with too few wait states.
- With no PLL, steps tied to the regulator can be skipped: the regulator then
  runs at scale 3, which covers HSI and HSE speeds.

## Concurrency

There is no kernel yet, so no driver guards against being called from two
contexts at once. When tasks and interrupts exist:

- **Safe on distinct bits:** single stores such as `gpioWritePin()` (BSRR),
  `nvicEnableIrq()`/`nvicDisableIrq()`/`nvicSetPending()`/`nvicClearPending()`
  (write-1-to-act), `nvicSetPriority()` (one byte), `extiClearPending()`
  (write-1-to-clear), `iwdgRefresh()` (one key write), and pure reads.
- **Caller must serialise:** read-modify-writes of shared registers, which
  covers every `rcc*Clock*()`, the RCC CR/CFGR/PLLCFGR writers, the PWR and
  Flash functions, `gpioInit()`/`gpioDeinit()`/`gpioSetAlternateFunction()`,
  `uartInit()`, `nvicSetSystemHandlerPriority()` (SHPR3 holds both
  PendSV and SysTick), `faultEnable()` (SHCSR, CCR), every other EXTI
  function (EXTICR, RTSR/FTSR, IMR, SWIER), `rccClearResetFlags()` (CSR),
  and `iwdgFreezeInDebug()` (APB1_FZ).
- **One context, once:** `iwdgStart()`. Its key sequence must not be
  interleaved with a refresh, which would relock PR and RLR.
- **Single owner only:** `gpioTogglePin()`, `uartReceiveByte()`/
  `uartReceive()` (the SR-then-DR read clears the error flags), and
  `systickInit()`.

Until the kernel's critical sections exist, configure peripherals from one
context, before any interrupt is enabled.

## Host testing and its limits

Host tests run driver logic against in-memory register structs (see
[Register blocks as parameters](#register-blocks-as-parameters) and
[Poll-count timeouts](#poll-count-timeouts)).

- One branch cannot run off-target: `flashSetLatency()` reads back ACR to
  confirm the latency was accepted, and an in-memory struct always agrees
  with itself. It is the project's only coverage exclusion. Treat a proposed
  second one with suspicion.
- Host tests cannot show that the hardware accepts what a driver writes, or
  in what order. The PWR voltage-scale sequence once shipped inverted with
  every test passing. The reference manual, not the test suite, is the
  authority on ordering and writability.
- The on-target smoke test (`tests/target/smoke.c`) covers that gap. It is
  a second image that runs the BSP and drivers on the board, reads back
  what the chip did (clock tree, regulator, flash wait states, GPIO, NVIC,
  SysTick) and reports PASS/FAIL on the VCP. `main()` itself is still an
  empty loop.
- The smoke test runs by hand (`make smoke-flash`). CI only builds it: there
  is deliberately no self-hosted hardware-in-the-loop runner (see
  [Security](../SECURITY.md)).

## MISRA C:2012 deviations

`make lint` runs cppcheck's MISRA addon and fails on any finding. CI pins
cppcheck 2.13.0 because other versions report different findings.

- **11.4** (pointer/integer conversion) is suppressed in `*_reg.h` only. A
  cast from a fixed address to a register-block pointer is the only way to
  reach memory-mapped hardware in standard C.
- **2.5, 2.3, 2.4** (unused macro/type/tag) are suppressed per register
  header that models more than its current users need, and **8.7**
  (external linkage used in one file) on `gpio.c`, `uart.c` and `iwdg.c`,
  which have no second caller yet. Remove each one once the last unused item gets a user.
- **12.2** (shift range) is never suppressed. `1U` is an 8-bit essential
  type, so every bit macro is written `((uint32_t)1U << n)`.
- Any inline `// cppcheck-suppress` states its reason beside it.
