##########################################################################
# STM32F446RE bare-metal RTOS. Commands: README.md, "Make commands".
##########################################################################

TARGET    := stm32_rtos
BUILD_DIR := build
DOXYGEN_DOCS_DIR := docs/html

# --- Toolchain ----------------------------------------------------------
PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
AS      := $(PREFIX)gcc
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size
GDB     := $(PREFIX)gdb

MCU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# --- Sources --------------------------------------------------------------
# Every .c, .s and .S under these directories is built; no list to maintain.
# .S goes through the C preprocessor, .s does not. Sorted so the link order,
# and therefore the image, does not depend on the filesystem.
SRC_DIRS    := core device drivers api bsp rtos app startup
C_SOURCES   := $(sort $(shell find $(SRC_DIRS) -name '*.c' 2>/dev/null))
ASM_SOURCES := $(sort $(shell find $(SRC_DIRS) \( -name '*.s' -o -name '*.S' \) 2>/dev/null))

INC_DIRS := core/inc device/inc drivers/inc api/inc bsp/inc \
            rtos/kernel/inc rtos/api/inc
INCLUDES := $(addprefix -I,$(INC_DIRS))

# --- Build profile --------------------------------------------------------
# debug (-O0) is the default; release (-O2) is what timing should be measured on.
BUILD ?= debug

ifeq ($(BUILD),debug)
  OPT_FLAGS := -O0 -g3
else ifeq ($(BUILD),release)
  OPT_FLAGS := -O2 -g
else
  $(error Unknown BUILD '$(BUILD)' - use 'debug' or 'release')
endif

# --- Flags ----------------------------------------------------------------
# Every warning is an error, including DRIVER_MUST_CHECK. -Wconversion and
# -Wundef catch the mask/shift and undefined-macro mistakes register code invites.
WARN_FLAGS := -Wall -Wextra -Wshadow -Wconversion -Wundef -Werror

CFLAGS  := $(MCU_FLAGS) $(INCLUDES) -std=c11 \
           $(WARN_FLAGS) \
           -ffunction-sections -fdata-sections $(OPT_FLAGS) -MMD -MP
ASFLAGS := $(MCU_FLAGS) -g3 -MMD -MP

LDSCRIPT := linker/STM32F446RE.ld

# -nostartfiles: Reset_Handler does crt0's job, and linking crt0 pulled in
# newlib stubs that warned on every link. With the link silent,
# --fatal-warnings makes any new linker warning an error.
# --orphan-handling=error: a section the linker script does not name fails
# the link instead of being placed by heuristic. Nothing runs .init_array,
# so a C constructor is one of those.
LDFLAGS := $(MCU_FLAGS) -T$(LDSCRIPT) -Wl,--gc-sections -Wl,--fatal-warnings \
           -Wl,--orphan-handling=error \
           -nostartfiles --specs=nosys.specs

# Objects mirror the source tree: drivers/src/gpio.c -> build/drivers/src/gpio.o
OBJECTS := $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJECTS += $(patsubst %.S,$(BUILD_DIR)/%.o,$(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES)))
DEPS    := $(OBJECTS:.o=.d)

# --- Targets --------------------------------------------------------------
.DEFAULT_GOAL := all

.PHONY: all clean flash erase debug size smoke smoke-flash re docs format format-check lint test coverage check-version check-pr hooks FORCE

# Never leave a truncated output that a later run would treat as up to date.
.DELETE_ON_ERROR:

# Flags stamp: holds the compile and link command lines and is rewritten only
# when they change, so switching BUILD (or any flag) rebuilds everything.
# STAMP_WAIT runs before a rewrite: GNU Make 3.81 (macOS's /usr/bin/make)
# compares whole-second mtimes, so a stamp rewritten in the second the last
# build finished would not count as newer and the old objects would be kept.
FLAGS_STAMP := $(BUILD_DIR)/.flags
STAMP_WAIT  := sleep 1

$(FLAGS_STAMP): FORCE
	@mkdir -p $(BUILD_DIR)
	@printf '%s\n' '$(CC) $(CFLAGS) | $(AS) $(ASFLAGS) | $(LD) $(LDFLAGS)' | cmp -s - $@ || \
	  { $(STAMP_WAIT); printf '%s\n' '$(CC) $(CFLAGS) | $(AS) $(ASFLAGS) | $(LD) $(LDFLAGS)' > $@; }

FORCE:

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex size

$(BUILD_DIR)/%.o: %.c $(FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s $(FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S $(FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT) $(FLAGS_STAMP)
	$(LD) $(LDFLAGS) -Wl,-Map=$(@:.elf=.map) $(OBJECTS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

# --- On-target smoke test --------------------------------------------------
# A second image with tests/target/ in place of app/ (see tests/target/smoke.c).
# SMOKE_FAULT=1 ends it with a deliberate divide by zero for Fault_Handler.
SMOKE_SOURCES := $(sort $(wildcard tests/target/*.c))
SMOKE_OBJECTS := $(filter-out $(BUILD_DIR)/app/%,$(OBJECTS)) $(SMOKE_SOURCES:%.c=$(BUILD_DIR)/%.o)
SMOKE_ELF     := $(BUILD_DIR)/smoke.elf

ifeq ($(SMOKE_FAULT),1)
  CFLAGS += -DSMOKE_FAULT=1
endif

$(SMOKE_ELF): $(SMOKE_OBJECTS) $(LDSCRIPT) $(FLAGS_STAMP)
	$(LD) $(LDFLAGS) -Wl,-Map=$(@:.elf=.map) $(SMOKE_OBJECTS) -o $@

smoke: $(SMOKE_ELF)
	$(SIZE) $<

# --- Board (OpenOCD over the on-board ST-LINK) ----------------------------
flash: $(BUILD_DIR)/$(TARGET).elf
	openocd -f tools/openocd.cfg -c "program $< verify reset exit"

smoke-flash: $(SMOKE_ELF)
	openocd -f tools/openocd.cfg -c "program $< verify reset exit"

erase:
	openocd -f tools/openocd.cfg \
	  -c "init" -c "reset halt" -c "stm32f4x mass_erase 0" \
	  -c "reset run" -c "exit"

# Starts OpenOCD, waits (up to ~10 s) until its GDB port answers, then loads
# the ELF and halts at reset. The trap kills OpenOCD when GDB exits, so the
# ST-LINK is free for the next flash.
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

re:
	$(MAKE) clean
	$(MAKE) all

# tests/build and tests/coverage are left alone; the flags stamps keep them consistent.
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DOXYGEN_DOCS_DIR)

# --- Checks ---------------------------------------------------------------
check-version:
	awk -v tag="$(TAG)" -f tools/check_version.awk Doxyfile CHANGELOG.md

BASE ?= origin/develop

check-pr:
	tools/check_pr_files.sh $(BASE)

# CI uses clang-format 18, and other major versions format differently.
# Homebrew's llvm@18 is keg-only (not on PATH), so look for it there first.
CLANG_FORMAT_CI_MAJOR := 18
CLANG_FORMAT_PINNED   := $(firstword $(wildcard /opt/homebrew/opt/llvm@18/bin/clang-format \
                                                /usr/local/opt/llvm@18/bin/clang-format))
CLANG_FORMAT          ?= $(if $(CLANG_FORMAT_PINNED),$(CLANG_FORMAT_PINNED),clang-format)
FORMAT_FILES           = $(shell git ls-files '*.c' '*.h' | grep -v '^tests/unity/')

define check-clang-format-version
	@v=$$($(CLANG_FORMAT) --version | sed -n 's/.*version \([0-9]*\).*/\1/p'); \
	if [ "$$v" != "$(CLANG_FORMAT_CI_MAJOR)" ]; then \
	  echo "make $@: warning: clang-format $$v is not CI's $(CLANG_FORMAT_CI_MAJOR), so the result may differ from CI - install clang-format 18 (macOS: brew install llvm@18)" >&2; \
	fi
endef

format:
	$(check-clang-format-version)
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

format-check:
	$(check-clang-format-version)
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

# CI pins cppcheck 2.13.0, and other versions report different MISRA findings.
# tools/install_cppcheck.sh builds it into CPPCHECK_PINNED, which lint then uses.
CPPCHECK_CI_VERSION := 2.13.0
CPPCHECK_PINNED     := $(HOME)/.local/share/cppcheck-$(CPPCHECK_CI_VERSION)/bin/cppcheck
CPPCHECK            ?= $(if $(wildcard $(CPPCHECK_PINNED)),$(CPPCHECK_PINNED),cppcheck)

# Each suppression is justified in docs/DESIGN.md, "MISRA C:2012 deviations":
# 11.4 for the register-pointer casts, and 2.3/2.4/2.5/8.7 per file until the
# last unused item gets a user. 12.2 is never suppressed.
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
	  --suppress=misra-c2012-2.5:device/inc/exti_reg.h \
	  --suppress=misra-c2012-2.5:device/inc/syscfg_reg.h \
	  --suppress=misra-c2012-2.5:device/inc/iwdg_reg.h \
	  --suppress=misra-c2012-2.5:device/inc/dbgmcu_reg.h \
	  --suppress=misra-c2012-2.5:core/inc/nvic_reg.h \
	  --suppress=misra-c2012-2.5:core/inc/scb_reg.h \
	  --suppress=misra-c2012-2.5:core/inc/systick_reg.h \
	  --suppress=misra-c2012-2.5:bsp/inc/bsp.h \
	  --suppress=misra-c2012-2.3:core/inc/scb_reg.h \
	  --suppress=misra-c2012-2.4:core/inc/scb_reg.h \
	  --suppress=misra-c2012-8.7:drivers/src/gpio.c \
	  --suppress=misra-c2012-8.7:drivers/src/uart.c \
	  --suppress=misra-c2012-8.7:drivers/src/iwdg.c \
	  $(INCLUDES) $(SRC_DIRS)

# --- Host tests (native compiler, in-memory register structs) -------------
# See docs/DESIGN.md, "Host testing and its limits".
HOST_CC        := cc
HOST_CFLAGS    := -std=c11 $(WARN_FLAGS) -MMD -MP
HOST_LDFLAGS   :=

# make test SANITIZE=1: the same suite under ASan and UBSan, any report fatal.
ifeq ($(SANITIZE),1)
  HOST_CFLAGS  += -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
  HOST_LDFLAGS += -fsanitize=address,undefined
endif
TEST_DIR       := tests
TEST_BUILD_DIR := $(TEST_DIR)/build

# Sources compiled into the host tests: each takes its register block as a
# parameter. Coverage gates drivers/src and bsp/src at 100%.
TEST_DRIVER_SOURCES := drivers/src/gpio.c drivers/src/rcc.c drivers/src/flash.c drivers/src/pwr.c \
                       drivers/src/uart.c drivers/src/nvic.c drivers/src/systick.c drivers/src/fault.c drivers/src/exti.c drivers/src/iwdg.c \
                       bsp/src/bsp.c

TEST_SOURCES   := $(wildcard $(TEST_DIR)/unit/*.c) $(TEST_DIR)/unity/unity.c $(TEST_DRIVER_SOURCES)
TEST_INCLUDES  := $(INCLUDES) -I$(TEST_DIR)/unity

TEST_OBJECTS   := $(TEST_SOURCES:%.c=$(TEST_BUILD_DIR)/%.o)

# One flags stamp per host build directory, like FLAGS_STAMP above.
HOST_STAMP     := $(TEST_BUILD_DIR)/.flags
COVERAGE_STAMP := $(TEST_DIR)/coverage/.flags

$(HOST_STAMP): FORCE
	@mkdir -p $(dir $@)
	@printf '%s\n' '$(HOST_CC) $(HOST_CFLAGS) $(TEST_INCLUDES)' | cmp -s - $@ || \
	  { $(STAMP_WAIT); printf '%s\n' '$(HOST_CC) $(HOST_CFLAGS) $(TEST_INCLUDES)' > $@; }

$(COVERAGE_STAMP): FORCE
	@mkdir -p $(dir $@)
	@printf '%s\n' '$(HOST_CC) $(HOST_CFLAGS) $(TEST_INCLUDES) --coverage -O0' | cmp -s - $@ || \
	  { $(STAMP_WAIT); printf '%s\n' '$(HOST_CC) $(HOST_CFLAGS) $(TEST_INCLUDES) --coverage -O0' > $@; }

$(TEST_BUILD_DIR)/%.o: %.c $(HOST_STAMP)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) $(TEST_INCLUDES) -c $< -o $@

# Also checks IRQn_e against the vector table, and that every test is registered.
test: $(TEST_OBJECTS)
	$(HOST_CC) $(HOST_LDFLAGS) $(TEST_OBJECTS) -o $(TEST_BUILD_DIR)/run_tests
	$(TEST_BUILD_DIR)/run_tests
	awk -f tools/check_vector_table.awk core/inc/nvic_reg.h startup/startup_stm32f446re.s
	awk -f tools/check_test_registration.awk $(TEST_DIR)/unit/*.c

# The only exclusion is flashSetLatency()'s read-back branch (see flash.c).
COVERAGE_MIN_LINE   := 100
COVERAGE_MIN_BRANCH := 100
COVERAGE_DIR        := $(TEST_DIR)/coverage
COVERAGE_OBJECTS    := $(TEST_SOURCES:%.c=$(COVERAGE_DIR)/%.o)

$(COVERAGE_DIR)/%.o: %.c $(COVERAGE_STAMP)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) $(TEST_INCLUDES) --coverage -O0 -c $< -o $@

coverage: $(COVERAGE_OBJECTS)
	@# Counters accumulate across runs: start from zero so a stale hit cannot pass the gate.
	find $(COVERAGE_DIR) -name '*.gcda' -delete
	$(HOST_CC) --coverage $(COVERAGE_OBJECTS) -o $(COVERAGE_DIR)/run_tests_cov
	$(COVERAGE_DIR)/run_tests_cov
	gcovr --root . --object-directory $(COVERAGE_DIR) --filter 'drivers/src/.*' --filter 'bsp/src/.*' \
	  --fail-under-line $(COVERAGE_MIN_LINE) --fail-under-branch $(COVERAGE_MIN_BRANCH) \
	  --print-summary

# --- Misc -----------------------------------------------------------------
hooks:
	git config core.hooksPath .githooks

# Fails on any Doxygen warning (WARN_AS_ERROR in the Doxyfile).
docs:
	doxygen Doxyfile

-include $(DEPS) $(SMOKE_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) $(COVERAGE_OBJECTS:.o=.d)
