##########################################################################
# Makefile - STM32F446RE Nucleo, bare-metal, no HAL/CubeMX
##########################################################################

TARGET    := stm32_rtos
BUILD_DIR := build
DOXYGEN_DOCS_DIR := docs/html

##########################################################################
# Toolchain
##########################################################################
PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
AS      := $(PREFIX)gcc
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size
GDB     := $(PREFIX)gdb

##########################################################################
# MCU flags (Cortex-M4F, hardware FPU)
##########################################################################
MCU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

##########################################################################
# Source discovery - picks up every .c/.s/.S under these dirs
# automatically, so new drivers/api files don't need to be added to the
# Makefile by hand. .s is plain assembly; .S is assembly run through the
# C preprocessor first (so it can #include the project's headers, e.g. the
# register definitions a context switch needs). Both are matched
# case-sensitively on purpose: a lowercase-only pattern silently skipped
# rtos/kernel/src/context_switch.S, which would have left PendSV_Handler
# as the weak Default_Handler loop with no build error.
##########################################################################
SRC_DIRS    := core device drivers api bsp rtos app startup
# $(sort) makes the link order (and so the map file and the image) independent of
# the filesystem's directory order.
C_SOURCES   := $(sort $(shell find $(SRC_DIRS) -name '*.c' 2>/dev/null))
ASM_SOURCES := $(sort $(shell find $(SRC_DIRS) \( -name '*.s' -o -name '*.S' \) 2>/dev/null))

##########################################################################
# Include paths - one per inc/ directory in the project structure
##########################################################################
INC_DIRS := core/inc device/inc drivers/inc api/inc bsp/inc \
            rtos/kernel/inc rtos/api/inc
INCLUDES := $(addprefix -I,$(INC_DIRS))

##########################################################################
# Build profile - debug (default) or release
##########################################################################
# debug: -O0, weakest optimizer, so GCC's uninitialized-variable analysis
# is at its least aggressive and won't optimize away a bug this project
# wants surfaced (see driver_status.h's DRIVER_STATUS_UNINITIALIZED
# comment) - this is what every target has always built with so far.
# release: -O2, representative of what actually ships; needed before any
# real timing characterization of the scheduler/context switch, since
# -O0 code does not represent shipped instruction counts or cycle timing.
# Objects depend on a flags stamp (see FLAGS_STAMP below), so switching
# BUILD - or anything else that changes the compiler/linker flags - rebuilds
# everything instead of mixing objects built under different flags into one
# image. It shares build/ with the other profile, so the last build wins.
BUILD ?= debug

ifeq ($(BUILD),debug)
  OPT_FLAGS := -O0 -g3
else ifeq ($(BUILD),release)
  OPT_FLAGS := -O2 -g
else
  $(error Unknown BUILD '$(BUILD)' - use 'debug' or 'release')
endif

##########################################################################
# Compile / assemble / link flags
##########################################################################
# Every warning is an error, so a new one fails the build instead of scrolling
# by. That includes the __attribute__((warn_unused_result)) check behind
# drivers/inc/driver_status.h's DRIVER_MUST_CHECK, which used to be the only
# warning promoted. -Wshadow, -Wconversion and -Wundef are on top of -Wall
# -Wextra because the register code is full of mask and shift arithmetic
# (-Wconversion) and of macros that could silently be undefined (-Wundef).
# The firmware, the drivers and the host tests all build clean under this set,
# at -O0 and -O2; the host test build (HOST_CFLAGS) shares it.
WARN_FLAGS := -Wall -Wextra -Wshadow -Wconversion -Wundef -Werror

CFLAGS  := $(MCU_FLAGS) $(INCLUDES) -std=c11 \
           $(WARN_FLAGS) \
           -ffunction-sections -fdata-sections $(OPT_FLAGS) -MMD -MP
ASFLAGS := $(MCU_FLAGS) -g3 -MMD -MP

LDSCRIPT := linker/STM32F446RE.ld

LDFLAGS := $(MCU_FLAGS) -T$(LDSCRIPT) -Wl,--gc-sections \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
           --specs=nosys.specs
# Note: --specs=nano.specs is not used. It was originally omitted because
# Homebrew's arm-none-eabi-gcc ships only standard newlib; the Arm GNU
# Toolchain this project builds with (locally and in CI) does ship it, so
# that reason is gone. It is still left out because no libc code is linked
# into the image today (the linker script has no .init_array, startup never
# calls __libc_init_array, and nothing uses printf/malloc), so nano would
# change nothing; revisit when something does. nosys.specs (syscall stubs)
# is unaffected. The image really does contain no libc code, but the link
# still pulls newlib's syscall stubs in before --gc-sections drops them, so
# it prints a "_close/_lseek/_read/_write is not implemented" warning for
# each; they are harmless and not a sign of anything wrong.

##########################################################################
# Object files - mirrors the source tree under build/, e.g.
# drivers/src/gpio.c -> build/drivers/src/gpio.o
##########################################################################
OBJECTS := $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJECTS += $(patsubst %.S,$(BUILD_DIR)/%.o,$(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES)))
DEPS    := $(OBJECTS:.o=.d)

##########################################################################
# Targets
##########################################################################
# Plain `make` must build the firmware. Without this, the first target in the
# file is the default goal, and that is the flags stamp rule below (which
# only writes build/.flags), not `all`.
.DEFAULT_GOAL := all

.PHONY: all clean flash erase debug size re docs format format-check lint test coverage check-version hooks FORCE

# A failed recipe must not leave a truncated .bin/.hex/.elf that a later
# make would treat as up to date.
.DELETE_ON_ERROR:

# ------------------------------------------------------------------------
# Flags stamp
# Make rebuilds an object only when its .c/.d prerequisites change, which
# says nothing about the compiler flags: after a debug build,
# `make BUILD=release` would report "nothing to be done" and present -O0
# objects as the release image. This stamp holds the full compile and link
# command lines; FORCE makes make re-evaluate it on every run, and the
# recipe rewrites it only when the text actually changed, so its mtime -
# and with it every object and the ELF that depend on it - moves only when
# the flags do.
# ------------------------------------------------------------------------
FLAGS_STAMP := $(BUILD_DIR)/.flags

$(FLAGS_STAMP): FORCE
	@mkdir -p $(BUILD_DIR)
	@printf '%s\n' '$(CC) $(CFLAGS) | $(AS) $(ASFLAGS) | $(LD) $(LDFLAGS)' | cmp -s - $@ || \
	  printf '%s\n' '$(CC) $(CFLAGS) | $(AS) $(ASFLAGS) | $(LD) $(LDFLAGS)' > $@

FORCE:

# ------------------------------------------------------------------------
# make / make all
# Compiles + links everything, produces .elf/.bin/.hex, prints size.
# Does NOT touch the board - no OpenOCD involved.
# Use case: confirm the code builds and check its size, with or without
# the board connected. This is the "does it compile" check.
# ------------------------------------------------------------------------
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex size

$(BUILD_DIR)/%.o: %.c $(FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s $(FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# .S goes through the C preprocessor, so unlike .s it needs the include
# paths.
$(BUILD_DIR)/%.o: %.S $(FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $(INCLUDES) -c $< -o $@

# The linker script and the flags are inputs of the link too.
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT) $(FLAGS_STAMP)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# ------------------------------------------------------------------------
# make size
# Reports flash (.text+.data) and RAM (.data+.bss) usage of the built elf.
# No compiling, no board interaction - just reads the existing .elf.
# Use case: keep an eye on your 128K RAM / 512K flash budget as the
# RTOS and drivers grow, or check whether a change bloated the binary.
# ------------------------------------------------------------------------
size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

# ------------------------------------------------------------------------
# make flash
# Builds first if needed, then programs the board over the onboard
# ST-LINK, verifies the write, and resets the MCU to run the new code.
# Use case: your everyday "I changed code, now run it on hardware"
# command - this is the one you'll use constantly.
# ------------------------------------------------------------------------
flash: $(BUILD_DIR)/$(TARGET).elf
	openocd -f tools/openocd.cfg -c "program $< verify reset exit"

# ------------------------------------------------------------------------
# make erase
# Full chip mass-erase - no compiling, no programming, board ends up
# blank. Does not depend on the elf and does not touch build/.
# Use case: rare - flash is in a bad state (e.g. after testing option
# bytes, or a bad write left it refusing to reprogram) and you want a
# clean slate before flashing again. Not part of the normal loop.
# ------------------------------------------------------------------------
erase:
	openocd -f tools/openocd.cfg \
	  -c "init" -c "reset halt" -c "stm32f4x mass_erase 0" \
	  -c "reset run" -c "exit"

# ------------------------------------------------------------------------
# make debug
# Builds first if needed, starts OpenOCD as a GDB server, and attaches
# arm-none-eabi-gdb to it.
# Use case: something is misbehaving on real hardware and you need to
# single-step, inspect registers, or set breakpoints - for investigating
# a bug, not for routine "run my code" (that's what flash is for).
#
# The trap matters: OpenOCD is backgrounded here, and without it the
# server outlived the GDB session that started it, kept holding the
# ST-LINK, and made the next `make flash` or `make debug` fail with a
# device-busy error until it was killed by hand. Killing it on EXIT/INT/
# TERM means quitting GDB - or Ctrl-C'ing the whole thing - leaves no
# stray process behind.
#
# GDB is started only once OpenOCD is accepting connections on its GDB
# port (3333, OpenOCD's default). Backgrounding OpenOCD and launching GDB
# immediately raced the server's startup - it has to open the ST-LINK and
# halt the target first - so GDB's single "target extended-remote"
# attempt could land before anything was listening and fail with
# "Connection refused". The wait gives up after ~10 s, or at once if
# OpenOCD itself exits (no board, ST-LINK busy), instead of hanging.
#
# GDB then halts the core, loads the freshly built ELF into flash and halts
# again at the reset vector. Attaching alone would leave whatever image is
# already on the board running against the symbols of the one just built,
# and the core could not be stepped from Reset_Handler.
# ------------------------------------------------------------------------
DEBUG_GDB_PORT := 3333

debug: $(BUILD_DIR)/$(TARGET).elf
	@openocd -f tools/openocd.cfg & \
	OPENOCD_PID=$$!; \
	trap 'kill $$OPENOCD_PID 2>/dev/null' EXIT INT TERM; \
	tries=0; \
	until nc -z localhost $(DEBUG_GDB_PORT) 2>/dev/null; do \
	  kill -0 $$OPENOCD_PID 2>/dev/null || { echo "make debug: OpenOCD exited before listening on :$(DEBUG_GDB_PORT)" >&2; exit 1; }; \
	  tries=$$((tries + 1)); \
	  [ $$tries -le 50 ] || { echo "make debug: timed out waiting for OpenOCD on :$(DEBUG_GDB_PORT)" >&2; exit 1; }; \
	  sleep 0.2; \
	done; \
	$(GDB) $< -ex "target extended-remote :$(DEBUG_GDB_PORT)" \
	  -ex "monitor reset halt" -ex "load" -ex "monitor reset halt"

# ------------------------------------------------------------------------
# make re
# Runs clean, then all, back to back.
# Use case: you changed something Make's dependency tracking might not
# catch cleanly - a Makefile variable, a compiler flag, the linker
# script itself - and you want zero risk of a half-stale build rather
# than trusting the incremental rebuild.
# ------------------------------------------------------------------------
re:
	$(MAKE) clean
	$(MAKE) all

# ------------------------------------------------------------------------
# make clean
# Deletes build/ and the generated docs/html/ entirely. No compiling happens.
# tests/build and tests/coverage are not touched (both are gitignored); remove
# tests/coverage by hand if you want a cold start (`make coverage` itself deletes
# stale .gcda files before it runs, and the flags stamp keeps the firmware objects
# consistent with the BUILD profile).
# Use case: you suspect a stale object file (renamed a function but an
# old .o still has the old symbol, or a .d file didn't catch a change)
# and want to start completely fresh.
# ------------------------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DOXYGEN_DOCS_DIR)

# ------------------------------------------------------------------------
# make check-version [TAG=vX.Y.Z]
# Fails if the Doxyfile's PROJECT_NUMBER and CHANGELOG.md's newest released
# heading name different versions, and - when TAG is given - if the tag
# names a third. See tools/check_version.awk and docs/VERSIONING.md.
# Use case: run on every PR in CI (no TAG) so a release PR that bumps one
# file and not the other is caught before merge; the release workflow
# passes the pushed tag.
# ------------------------------------------------------------------------
check-version:
	awk -v tag="$(TAG)" -f tools/check_version.awk Doxyfile CHANGELOG.md

# ------------------------------------------------------------------------
# make format
# Applies .clang-format to every tracked .c/.h file in place.
# Use case: fix formatting drift before committing.
# ------------------------------------------------------------------------
format:
	clang-format -i $(shell git ls-files '*.c' '*.h' | grep -v '^tests/unity/')

# ------------------------------------------------------------------------
# make format-check
# Same as format, but non-mutating: fails (exit 1) if any tracked file
# would be reformatted. Safe for CI and the pre-commit hook.
# ------------------------------------------------------------------------
format-check:
	clang-format --dry-run --Werror $(shell git ls-files '*.c' '*.h' | grep -v '^tests/unity/')

# ------------------------------------------------------------------------
# cppcheck version: CI pins an exact version (.github/workflows/ci.yml) and
# different versions report different MISRA findings, so a pass under a
# newer local cppcheck does not mean CI will pass. tools/install_cppcheck.sh
# builds the pinned version into $(CPPCHECK_PINNED); once it exists, lint
# uses it automatically. Override with CPPCHECK=/path/to/cppcheck. Keep
# CPPCHECK_CI_VERSION in step with ci.yml's pin.
# ------------------------------------------------------------------------
CPPCHECK_CI_VERSION := 2.13.0
CPPCHECK_PINNED     := $(HOME)/.local/share/cppcheck-$(CPPCHECK_CI_VERSION)/bin/cppcheck
CPPCHECK            ?= $(if $(wildcard $(CPPCHECK_PINNED)),$(CPPCHECK_PINNED),cppcheck)

# ------------------------------------------------------------------------
# make lint
# Runs cppcheck across the project's source/include dirs, failing (exit 1)
# on any finding - same severity as a compile error. Catches classes of
# bug -Wall/-Wextra don't (deeper dataflow), plus a MISRA C:2012 subset via
# --addon=misra (bundled with cppcheck itself, so no extra install step).
# Findings print as e.g. "[misra-c2012-8.4]" without the rule prose - MISRA
# doesn't allow redistributing rule text, only cppcheck's own summary. Look
# the number up in the MISRA C:2012 document if the summary isn't enough.
# Only checks .c files cppcheck discovers under $(SRC_DIRS) (headers are
# checked in the context of whichever .c includes them, not standalone).
#
# Two kinds of suppression here, deliberately handled differently, and one rule that is deliberately not suppressed (3):
#
# 1. Scoped to specific files, not disabled project-wide, and each one
#    verified needed under CI's pinned cppcheck (a newer cppcheck reports
#    more - e.g. 2.21 also wants misra-c2012-8.7 on rcc.c, nvic.c,
#    systick.c and bsp.c, which 2.13.0 does not - so a local pass with
#    another version proves nothing either way):
#    - misra-c2012-2.5 (unused macro) on device/inc/gpio_reg.h,
#      rcc_reg.h, flash_reg.h, pwr_reg.h and uart_reg.h, core/inc/
#      nvic_reg.h, scb_reg.h and systick_reg.h, and bsp/inc/bsp.h. These
#      headers model more than any consumer uses yet - rcc_reg.h's CIR
#      (clock-security-system) and CSR (LSI enable, reset-cause flags)
#      sections, flash_reg.h's SR/CR bits and ACR.ICRST/DCRST, pwr_reg.h's
#      low-power/PVD bits, uart_reg.h's SR.IDLE, every CR1 interrupt-enable
#      /SBK/RWU/WAKE bit and USART_CR2_STOP_0_5/_1_5 (smartcard-mode
#      framing, which uartInit() never enables), nvic_reg.h's STIR/IABR,
#      scb_reg.h's CFSR/HFSR/FPCCR, systick_reg.h's COUNTFLAG/CALIB - and
#      bsp.h's BSP_*_HZ constants have no caller until SysTick and UART are
#      initialised from them. Modeled for completeness per RM0390/PM0214,
#      not defects.
#    - misra-c2012-2.3/2.4 on core/inc/scb_reg.h for FpuRegisters_t,
#      whose first user is the kernel's context switch.
#    - misra-c2012-8.7 (external linkage used in only one translation
#      unit) on drivers/src/gpio.c and drivers/src/uart.c: nothing calls
#      them from a second file yet. Every other driver has a second
#      caller in the whole-project analysis (rcc.c, flash.c, pwr.c are
#      called by bsp.c, and rcc.c calls flashSetLatency()), and nvic.c,
#      systick.c and bsp.c have none but are not flagged by 2.13.0.
#    Remove a suppression the moment its file's last unused macro or
#    function gets a real caller. A project-wide suppression would blind
#    this check to a genuinely dead macro in any future file.
#
# 2. A permanent deviation scoped by glob, not to any one file: misra-
#    c2012-11.4 (pointer/integer conversion), suppressed for *_reg.h only.
#    Every peripheral base-address macro in every core/inc/device/inc
#    register header (`#define GPIOA ((GpioRegisters_t *)GPIOA_BASE)`,
#    and the same pattern for RCC, USART2, EXTI, ...) does exactly this
#    cast - it is the only way to define a pointer to a fixed, memory-
#    mapped hardware address in standard C, and it will never represent a
#    real defect here. Scoping this per-file instead would mean adding
#    one more suppression line every single time any peripheral's pointer
#    macro gets its first real caller, forever, which doesn't scale the
#    way the file-scoped suppressions below do (a small, bounded set of
#    driver files) - so this is scoped by the *_reg.h glob instead: every
#    register header, present and future, without an ever-growing list,
#    while still leaving the rule active for driver/api/bsp/rtos .c files,
#    where a genuinely risky pointer/integer conversion (e.g. casting a
#    runtime-computed address) should still be caught. Verified this
#    glob doesn't just suppress everything: a deliberately-bad
#    `(volatile uint32_t *)some_runtime_value` cast in a plain .c file
#    still gets flagged.
#
# 3. misra-c2012-12.2 (shift count must be in range for the operand's
#    essential type) is not suppressed anywhere, and must stay that way.
#    `1U` has the essential type of an 8-bit unsigned, so `1U << 17U` is a
#    genuine violation; every register bit and mask macro therefore casts
#    its constant first, `((uint32_t)1U << X_Pos)`, which is a 32-bit
#    operand. A new macro written as a bare `1U << n` for n >= 8 fails
#    lint, which is the check doing its job. Any other inline
#    `// cppcheck-suppress` needs its reason in the comment beside it.
#
#    --suppress=missingIncludeSystem below just silences cppcheck's note
#    that it did not look inside <stdint.h>/<stddef.h>.
# ------------------------------------------------------------------------
lint:
	@v=$$($(CPPCHECK) --version | awk '{print $$2}'); \
	if [ "$$v" != "$(CPPCHECK_CI_VERSION)" ]; then \
	  echo "make lint: warning: cppcheck $$v is not CI's $(CPPCHECK_CI_VERSION), so a pass here does not guarantee CI passes - run tools/install_cppcheck.sh" >&2; \
	fi
	$(CPPCHECK) --addon=misra \
	  --enable=warning,style,performance,portability \
	  --std=c11 --error-exitcode=1 --inline-suppr \
	  --suppress=missingIncludeSystem \
	  --suppress=misra-c2012-11.4:'*_reg.h' \
	  --suppress=misra-c2012-2.5:device/inc/gpio_reg.h \
	  --suppress=misra-c2012-2.5:device/inc/rcc_reg.h \
	  --suppress=misra-c2012-2.5:device/inc/flash_reg.h \
	  --suppress=misra-c2012-2.5:device/inc/pwr_reg.h \
	  --suppress=misra-c2012-2.5:device/inc/uart_reg.h \
	  --suppress=misra-c2012-2.5:core/inc/nvic_reg.h \
	  --suppress=misra-c2012-2.5:core/inc/scb_reg.h \
	  --suppress=misra-c2012-2.5:core/inc/systick_reg.h \
	  --suppress=misra-c2012-2.5:bsp/inc/bsp.h \
	  --suppress=misra-c2012-2.3:core/inc/scb_reg.h \
	  --suppress=misra-c2012-2.4:core/inc/scb_reg.h \
	  --suppress=misra-c2012-8.7:drivers/src/gpio.c \
	  --suppress=misra-c2012-8.7:drivers/src/uart.c \
	  $(INCLUDES) $(SRC_DIRS)

# ------------------------------------------------------------------------
# make test
# Compiles and runs the host-side unit tests (tests/unit/) against Unity
# (vendored in tests/unity/), using the NATIVE compiler - not
# arm-none-eabi-gcc. Only pure-logic code with no direct hardware access
# is testable this way (register-header data, driver logic once it takes
# its register block as a parameter instead of reaching for the global
# macro). Separate build dir (tests/build/) so it never touches build/.
# Also runs two host-side, no-hardware consistency checks in the same
# spirit as the Unity suite, both covering a pair of hand-written lists
# with no shared source of truth that nothing else catches drifting
# apart:
#   - tools/check_vector_table.awk cross-checks core/inc/nvic_reg.h's
#     IRQn_e enum against startup/startup_stm32f446re.s's vector table.
#   - tools/check_test_registration.awk cross-checks the test functions
#     defined in tests/unit/*.c against test_runner.c's extern/RUN_TEST
#     pairs. A test that is defined but never registered still compiles
#     and the suite still passes green - it simply never runs. `make
#     coverage` would catch that for a driver or bsp test, but its filter
#     is drivers/src and bsp/src only, so for the test_<peripheral>_reg.c files a
#     forgotten registration is otherwise entirely silent.
# Use case: fast feedback on register/driver logic correctness, no board
# or cross-toolchain required. The pre-commit hook runs it before make
# lint/docs/all; CI runs it after make all and make format-check. It's the
# cheapest real check available.
# ------------------------------------------------------------------------
HOST_CC        := cc
# -MMD -MP: write a .d file beside each host object so editing a header rebuilds the
# test/coverage objects that include it, as the firmware build already does.
HOST_CFLAGS    := -std=c11 $(WARN_FLAGS) -MMD -MP
TEST_DIR       := tests
TEST_BUILD_DIR := $(TEST_DIR)/build

# Driver .c files that are host-testable off-target: pure register-block
# logic with no direct hardware access, because the block is a function
# parameter (e.g. gpio.c's GpioRegisters_t *port, rcc.c's RccRegisters_t
# *rcc) rather than a hardware GPIOx/RCC-style macro. Add a driver (or bsp/)
# file here only once it meets that bar - one that reaches for a hardware
# macro directly won't and shouldn't be compiled with HOST_CC. bsp/src/bsp.c
# is listed too: bspClockInit() takes the RCC/PWR/FLASH blocks as parameters,
# and the gcovr filter below covers bsp/src alongside drivers/src.
#
# This bar turned out to include hardware-ready polling too, not just
# simple register writes: rcc.c/pwr.c's HSI/HSE/PLL/SYSCLK/VOS/over-drive
# ready-waits (and uart.c's TXE/TC/RXNE waits) are bounded iteration-count retries, not wall-clock
# timeouts (SysTick isn't configured this early in clock bring-up - see
# rcc.h's file-level comment), and the ready bit each one polls
# (HSIRDY, VOSRDY, ...) is a field distinct from what the driver itself
# writes. A host test holds the ready bit clear indefinitely in a plain
# in-memory register struct to exercise the timeout branch - no real
# hardware or timer required. An earlier version of this comment (and of
# rcc.h/CHANGELOG.md) stated the opposite for rcc.c's then-unimplemented
# SYSCLK bring-up; that was reconsidered once it was actually written and
# tested, rather than left standing as-is.
TEST_DRIVER_SOURCES := drivers/src/gpio.c drivers/src/rcc.c drivers/src/flash.c drivers/src/pwr.c \
                       drivers/src/uart.c drivers/src/nvic.c drivers/src/systick.c \
                       bsp/src/bsp.c

TEST_SOURCES   := $(wildcard $(TEST_DIR)/unit/*.c) $(TEST_DIR)/unity/unity.c $(TEST_DRIVER_SOURCES)
TEST_INCLUDES  := $(INCLUDES) -I$(TEST_DIR)/unity

# Object paths mirror each source's full path under the build dir (e.g.
# drivers/src/gpio.c -> tests/build/drivers/src/gpio.o), the same way the
# firmware build's OBJECTS does. An earlier version flattened these with
# $(notdir) and resolved them back through a vpath, which worked only as
# long as no two sources anywhere in TEST_SOURCES shared a basename:
# tests/unit/gpio.c and drivers/src/gpio.c would have collapsed onto one
# object file, and whichever vpath entry came first would silently win.
# Mirroring the path removes the collision by construction rather than
# relying on a naming convention nothing enforces.
TEST_OBJECTS   := $(TEST_SOURCES:%.c=$(TEST_BUILD_DIR)/%.o)

$(TEST_BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) $(TEST_INCLUDES) -c $< -o $@

test: $(TEST_OBJECTS)
	$(HOST_CC) $(TEST_OBJECTS) -o $(TEST_BUILD_DIR)/run_tests
	$(TEST_BUILD_DIR)/run_tests
	awk -f tools/check_vector_table.awk core/inc/nvic_reg.h startup/startup_stm32f446re.s
	awk -f tools/check_test_registration.awk $(TEST_DIR)/unit/*.c

# ------------------------------------------------------------------------
# make coverage
# Rebuilds the same test/driver sources as `make test` with GCC's
# --coverage instrumentation (-fprofile-arcs -ftest-coverage, implied by
# --coverage), runs the suite to produce .gcda data, then gates on line
# and branch coverage via gcovr (pip install gcovr; on macOS the Command Line Tools' Python is too old for the pinned 8.6 - use Homebrew) - failing (exit 1)
# below COVERAGE_MIN_LINE/COVERAGE_MIN_BRANCH.
#
# Scoped to TEST_DRIVER_SOURCES only (the --filter below), not
# tests/unit/*_reg.c or the register headers themselves: those are pure
# data - structs and macros, no branches - so "coverage" isn't a
# meaningful concept there and would only pad the reported number toward
# 100% for free. The filter is by directory (drivers/src and bsp/src), so
# another host-testable file in either is covered by this gate the moment
# it's added to TEST_DRIVER_SOURCES; a new directory also needs its own
# --filter below.
#
# Separate build dir (tests/coverage/) so this never touches build/ or
# tests/build/ - instrumented objects are not the same as make test's
# plain ones and must not be mixed with them.
#
# Exactly one region in the project is excluded from this gate, marked
# with GCOVR_EXCL_START/STOP in drivers/src/flash.c: flashSetLatency()'s
# read-back check compares ACR against the value it just wrote to ACR,
# and a plain in-memory register struct always agrees by construction, so
# the failure half is unreachable off-target. That comment carries the
# full reasoning. Treat any proposed second exclusion with suspicion -
# the gate is only worth having if reaching 100% means the tests actually
# exercise the code, and every other branch in drivers/ is reachable
# because the flag a driver polls is always a field distinct from the one
# it writes (see TEST_DRIVER_SOURCES above).
# ------------------------------------------------------------------------
COVERAGE_MIN_LINE   := 100
COVERAGE_MIN_BRANCH := 100
COVERAGE_DIR        := $(TEST_DIR)/coverage
COVERAGE_OBJECTS    := $(TEST_SOURCES:%.c=$(COVERAGE_DIR)/%.o)

$(COVERAGE_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) $(TEST_INCLUDES) --coverage -O0 -c $< -o $@

coverage: $(COVERAGE_OBJECTS)
	@# gcov counters accumulate across runs, so a leftover .gcda from an earlier run
	@# (a deleted or broken test, say) could keep showing hits and let the gate pass
	@# wrongly. Start every run from zero.
	find $(COVERAGE_DIR) -name '*.gcda' -delete
	$(HOST_CC) --coverage $(COVERAGE_OBJECTS) -o $(COVERAGE_DIR)/run_tests_cov
	$(COVERAGE_DIR)/run_tests_cov
	gcovr --root . --object-directory $(COVERAGE_DIR) --filter 'drivers/src/.*' --filter 'bsp/src/.*' \
	  --fail-under-line $(COVERAGE_MIN_LINE) --fail-under-branch $(COVERAGE_MIN_BRANCH) \
	  --print-summary

# ------------------------------------------------------------------------
# make hooks
# One-time per clone: point git at the tracked pre-commit hook in .githooks/
# (see CONTRIBUTING.md). Does not touch the working tree.
# ------------------------------------------------------------------------
hooks:
	git config core.hooksPath .githooks

# ------------------------------------------------------------------------
# make docs
# Runs Doxygen against Doxyfile. WARN_AS_ERROR is on, so this fails the
# moment a public function is undocumented or missing a @param/@return -
# treat a failing `make docs` the same as a failing `make all`.
# Use case: check documentation coverage before committing, or as a CI
# gate alongside the build.
# ------------------------------------------------------------------------
docs:
	doxygen Doxyfile

# Auto-generated per-file header dependencies (from -MMD -MP above) -
# lets Make recompile only the .o files whose included headers changed.
-include $(DEPS) $(TEST_OBJECTS:.o=.d) $(COVERAGE_OBJECTS:.o=.d)
