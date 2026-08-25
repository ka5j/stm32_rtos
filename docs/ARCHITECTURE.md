# Architecture

See [CONTRIBUTING.md](../CONTRIBUTING.md) for naming conventions, the layering rule, the error-handling contract, and the local development workflow (formatting/lint/docs gates, pre-commit hook).

## Directory layout

```
stm32_rtos/
├── core/            Cortex-M4 core peripheral registers (NVIC, SysTick, SCB)
│   └── inc/
├── device/          STM32F446-specific peripheral registers (GPIO, RCC, UART)
│   └── inc/
├── drivers/         Register-level driver logic (GPIO, RCC, UART, NVIC, SysTick)
│   ├── inc/
│   └── src/
├── api/             App-facing peripheral API (e.g. led_on(), debug_print())
│   ├── inc/
│   └── src/
├── bsp/             Nucleo-F446RE board/pin mapping, consumed by api/
│   ├── inc/
│   └── src/
├── rtos/
│   ├── kernel/      Scheduler, task control blocks, PendSV context switching
│   │   ├── inc/
│   │   └── src/
│   └── api/         RTOS syscall-style interface (task_create, semaphores, queues)
│       ├── inc/
│       └── src/
├── app/             Application task code — only includes api/ and rtos/api/
│   └── src/
├── tests/           Host-side unit tests (native compiler, not arm-none-eabi-gcc)
│   ├── unity/       Vendored Unity test framework
│   └── unit/        test_<peripheral>_reg.c per register header (aggregated
│                     by test_runner.c), test_<name>.c per driver/api/bsp/
│                     rtos module — see CONTRIBUTING.md's new-module test
│                     requirement
├── startup/         Vector table, reset handler
├── linker/          STM32F446RE.ld memory layout
└── tools/           openocd.cfg
```

`api/` and `rtos/api/` are readily conflated: `api/` is the peripheral-facing layer (drives GPIO/UART directly, independent of the RTOS and board), while `rtos/api/` is the RTOS syscall-facing layer (tasks, semaphores, queues). Application code in `app/` calls into both but never reaches past them into `drivers/` or `rtos/kernel/` directly.

## Layer diagram

```
┌─────────────────────────────────────────────┐
│                 app/  (tasks)                │
└───────────────┬───────────────┬─────────────┘
                 │               │
        ┌────────▼───────┐ ┌─────▼──────────┐
        │      api/       │ │   rtos/api/     │
        │ led_on(), etc.  │ │ task_create(),  │
        │                 │ │ semaphores, ... │
        └────────┬────────┘ └─────┬───────────┘
                 │                │
        ┌────────▼────────┐ ┌─────▼───────────┐
        │      bsp/        │ │  rtos/kernel/   │
        │  pin/peripheral   │ │  scheduler,     │
        │  mapping          │ │  PendSV switch  │
        └────────┬─────────┘ └─────┬────────────┘
                 │                 │
        ┌────────▼─────────────────▼────────────┐
        │              drivers/                  │
        │  GPIO, RCC, UART, NVIC, SysTick         │
        └────────────────────┬────────────────────┘
                              │
        ┌─────────────────────▼───────────────────┐
        │           core/ + device/                │
        │  hand-derived register structs           │
        └───────────────────────────────────────────┘
```

## Memory map (STM32F446RE)

Defined in [`linker/STM32F446RE.ld`](../linker/STM32F446RE.ld):

| Region | Base       | Size   |
| ------ | ---------- | ------ |
| FLASH  | 0x08000000 | 512 KB |
| RAM    | 0x20000000 | 128 KB |

`.isr_vector` is placed first in FLASH and `KEEP()`'d so the linker cannot garbage-collect it. `.data` uses `AT>FLASH` to keep its load address in FLASH while its run address is in RAM, initialized by the `.data`/`.bss` copy loop in `Reset_Handler`. `._user_heap_stack` reserves space above `.bss` so the link fails at build time if there is not room for the requested heap and stack, rather than silently corrupting memory at runtime.

## Vector table entry points

Defined in [`startup/startup_stm32f446re.s`](../startup/startup_stm32f446re.s), standard ARMv7-M layout: `SVC_Handler` (exception 11), `PendSV_Handler` (14), `SysTick_Handler` (15), followed by all ~97 F446 peripheral IRQs in RM0390 order. Every handler is weakly aliased to `Default_Handler` (an infinite loop) so an unimplemented interrupt hangs in a debuggable state instead of jumping to garbage — `PendSV_Handler` and `SysTick_Handler` currently fall through to this default until `rtos/kernel/src/context_switch.S` and the scheduler are implemented.

## Board defaults (NUCLEO-F446RE)

| Function          | Pin  | Peripheral |
| ----------------- | ---- | ---------- |
| User LED (LD2)     | PA5  | GPIOA      |
| User button (B1)   | PC13 | GPIOC      |
| ST-LINK VCP UART   | PA2 (TX) / PA3 (RX) | USART2 |
