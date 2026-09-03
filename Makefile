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
# Source discovery - picks up every .c/.s under these dirs automatically,
# so new drivers/api files don't need to be added to the Makefile by hand
##########################################################################
SRC_DIRS    := core device drivers api bsp rtos app startup
C_SOURCES   := $(shell find $(SRC_DIRS) -name '*.c' 2>/dev/null)
ASM_SOURCES := $(shell find $(SRC_DIRS) -name '*.s' 2>/dev/null)

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
# Switching BUILD without `make clean` first mixes object files built
# under the other profile's flags into the same build/ dir - `make re`
# (clean + all) is the safe way to switch.
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
CFLAGS  := $(MCU_FLAGS) $(INCLUDES) -std=c11 -Wall -Wextra \
           -Werror=unused-result \
           -ffunction-sections -fdata-sections $(OPT_FLAGS) -MMD -MP
# -Werror=unused-result: promotes __attribute__((warn_unused_result))
# (see drivers/inc/driver_status.h's DRIVER_MUST_CHECK) from a warning
# that scrolls by unnoticed to a build failure. Scoped to this one
# warning class rather than a blanket -Werror, so an unrelated -Wall/
# -Wextra finding doesn't block the build on its own.

ASFLAGS := $(MCU_FLAGS) -g3 -MMD -MP

LDSCRIPT := linker/STM32F446RE.ld

LDFLAGS := $(MCU_FLAGS) -T$(LDSCRIPT) -Wl,--gc-sections \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
           --specs=nosys.specs
# Note: --specs=nano.specs omitted - Homebrew's arm-none-eabi-gcc ships
# standard newlib only, not the newlib-nano variant. nosys.specs (syscall
# stubs) is unaffected. Binary is slightly larger without nano, no
# functional difference for this project.

##########################################################################
# Object files - mirrors the source tree under build/, e.g.
# drivers/src/gpio.c -> build/drivers/src/gpio.o
##########################################################################
OBJECTS := $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJECTS += $(ASM_SOURCES:%.s=$(BUILD_DIR)/%.o)
DEPS    := $(OBJECTS:.o=.d)

##########################################################################
# Targets
##########################################################################
.PHONY: all clean flash erase debug size re docs format format-check lint test coverage

# ------------------------------------------------------------------------
# make / make all
# Compiles + links everything, produces .elf/.bin/.hex, prints size.
# Does NOT touch the board - no OpenOCD involved.
# Use case: confirm the code builds and check its size, with or without
# the board connected. This is the "does it compile" check.
# ------------------------------------------------------------------------
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex size

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
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
# ------------------------------------------------------------------------
debug: $(BUILD_DIR)/$(TARGET).elf
	openocd -f tools/openocd.cfg & \
	$(GDB) $< -ex "target extended-remote :3333"

# ------------------------------------------------------------------------
# make re
# Runs clean, then all, back to back.
# Use case: you changed something Make's dependency tracking might not
# catch cleanly - a Makefile variable, a compiler flag, the linker
# script itself - and you want zero risk of a half-stale build rather
# than trusting the incremental rebuild.
# ------------------------------------------------------------------------
re: clean all

# ------------------------------------------------------------------------
# make clean
# Deletes build/ entirely. No compiling happens.
# Use case: you suspect a stale object file (renamed a function but an
# old .o still has the old symbol, or a .d file didn't catch a change)
# and want to start completely fresh.
# ------------------------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DOXYGEN_DOCS_DIR)

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
# make lint
# Runs cppcheck across the project's source/include dirs, failing (exit 1)
# on any finding - same severity as a compile error. Catches classes of
# bug -Wall/-Wextra don't (deeper dataflow), plus a MISRA C:2012 subset via
# --addon=misra (bundled with cppcheck itself, so no extra install step).
# Findings print as e.g. "[misra-c2012-8.4]" without the rule prose - MISRA
# doesn't allow redistributing rule text, only cppcheck's own summary. Look
# the number up in the MISRA C:2012 document if the summary isn't enough.
# Only checks .c files cppcheck discovers under $(SRC_DIRS) (headers are
# checked in the context of whichever .c includes them, not standalone) -
# so this stays quiet today since drivers/api/bsp/rtos/app are still empty,
# and starts finding real things the moment a .c file includes a register
# header.
#
# Two rules are suppressed, scoped to specific files, not disabled project-
# wide: misra-c2012-2.5 (unused macro) on device/inc/gpio_reg.h and
# misra-c2012-8.7 (external linkage used in only one translation unit) on
# drivers/src/gpio.c. Both are real findings today - nothing in api/bsp/app
# calls gpio.c/gpio_reg.h yet - but neither is a code defect, and a project-
# wide suppression would blind this check to a genuinely dead macro or a
# function that should be static in any future file, not just these two.
# Remove both suppressions the moment api/ gives this driver a real caller.
# ------------------------------------------------------------------------
lint:
	cppcheck --addon=misra \
	  --enable=warning,style,performance,portability \
	  --std=c11 --error-exitcode=1 --inline-suppr \
	  --suppress=missingIncludeSystem \
	  --suppress=misra-c2012-2.5:device/inc/gpio_reg.h \
	  --suppress=misra-c2012-8.7:drivers/src/gpio.c \
	  $(INCLUDES) $(SRC_DIRS)

# ------------------------------------------------------------------------
# make test
# Compiles and runs the host-side unit tests (tests/unit/) against Unity
# (vendored in tests/unity/), using the NATIVE compiler - not
# arm-none-eabi-gcc. Only pure-logic code with no direct hardware access
# is testable this way (register-header data, driver logic once it takes
# its register block as a parameter instead of reaching for the global
# macro). Separate build dir (tests/build/) so it never touches build/.
# Also runs tools/check_vector_table.awk, a host-side, no-hardware
# consistency check in the same spirit as the Unity suite: it cross-checks
# core/inc/nvic_reg.h's IRQn_e enum against startup/startup_stm32f446re.s's
# vector table, since the two are hand-written independently with no
# shared source of truth and nothing else catches them drifting apart.
# Use case: fast feedback on register/driver logic correctness, no board
# or cross-toolchain required. Run this before make docs/make all in the
# pre-commit hook and CI - it's the cheapest real check available.
# ------------------------------------------------------------------------
HOST_CC        := cc
TEST_DIR       := tests
TEST_BUILD_DIR := $(TEST_DIR)/build

# Driver .c files that are host-testable off-target: pure register-block
# logic with no direct hardware access, because the block is a function
# parameter (e.g. gpio.c's GpioRegisters_t *port) rather than a hardware
# GPIOx-style macro. Add a driver file here only once it meets that bar -
# one that blocks on real timing/interrupts, or reaches for a hardware
# macro directly, won't and shouldn't be compiled with HOST_CC.
TEST_DRIVER_SOURCES := drivers/src/gpio.c

TEST_SOURCES   := $(wildcard $(TEST_DIR)/unit/*.c) $(TEST_DIR)/unity/unity.c $(TEST_DRIVER_SOURCES)
TEST_INCLUDES  := $(INCLUDES) -I$(TEST_DIR)/unity
TEST_OBJECTS   := $(patsubst %.c,$(TEST_BUILD_DIR)/%.o,$(notdir $(TEST_SOURCES)))
vpath %.c $(TEST_DIR)/unit $(TEST_DIR)/unity drivers/src

$(TEST_BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror=unused-result $(TEST_INCLUDES) -c $< -o $@

test: $(TEST_OBJECTS)
	$(HOST_CC) $(TEST_OBJECTS) -o $(TEST_BUILD_DIR)/run_tests
	$(TEST_BUILD_DIR)/run_tests
	awk -f tools/check_vector_table.awk core/inc/nvic_reg.h startup/startup_stm32f446re.s

# ------------------------------------------------------------------------
# make coverage
# Rebuilds the same test/driver sources as `make test` with GCC's
# --coverage instrumentation (-fprofile-arcs -ftest-coverage, implied by
# --coverage), runs the suite to produce .gcda data, then gates on line
# and branch coverage via gcovr (pip install gcovr) - failing (exit 1)
# below COVERAGE_MIN_LINE/COVERAGE_MIN_BRANCH.
#
# Scoped to TEST_DRIVER_SOURCES only (the --filter below), not
# tests/unit/*_reg.c or the register headers themselves: those are pure
# data - structs and macros, no branches - so "coverage" isn't a
# meaningful concept there and would only pad the reported number toward
# 100% for free. The filter grows automatically as TEST_DRIVER_SOURCES
# does, so a second host-testable driver file is covered by this gate
# the moment it's added there, no Makefile change needed.
#
# Separate build dir (tests/coverage/) so this never touches build/ or
# tests/build/ - instrumented objects are not the same as make test's
# plain ones and must not be mixed with them.
# ------------------------------------------------------------------------
COVERAGE_MIN_LINE   := 100
COVERAGE_MIN_BRANCH := 100
COVERAGE_DIR        := $(TEST_DIR)/coverage
COVERAGE_OBJECTS    := $(patsubst %.c,$(COVERAGE_DIR)/%.o,$(notdir $(TEST_SOURCES)))

$(COVERAGE_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror=unused-result $(TEST_INCLUDES) --coverage -O0 -c $< -o $@

coverage: $(COVERAGE_OBJECTS)
	$(HOST_CC) --coverage $(COVERAGE_OBJECTS) -o $(COVERAGE_DIR)/run_tests_cov
	$(COVERAGE_DIR)/run_tests_cov
	gcovr --root . --object-directory $(COVERAGE_DIR) --filter 'drivers/src/.*' \
	  --fail-under-line $(COVERAGE_MIN_LINE) --fail-under-branch $(COVERAGE_MIN_BRANCH) \
	  --print-summary

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
-include $(DEPS)