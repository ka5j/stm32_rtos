##########################################################################
# Makefile - STM32F446RE Nucleo, bare-metal, no HAL/CubeMX
##########################################################################

TARGET    := stm32_rtos
BUILD_DIR := build

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
# Compile / assemble / link flags
##########################################################################
CFLAGS  := $(MCU_FLAGS) $(INCLUDES) -std=c11 -Wall -Wextra \
           -ffunction-sections -fdata-sections -O0 -g3 -MMD -MP

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
.PHONY: all clean flash erase debug size re docs format format-check lint

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

# ------------------------------------------------------------------------
# make format
# Applies .clang-format to every tracked .c/.h file in place.
# Use case: fix formatting drift before committing.
# ------------------------------------------------------------------------
format:
	clang-format -i $(shell git ls-files '*.c' '*.h')

# ------------------------------------------------------------------------
# make format-check
# Same as format, but non-mutating: fails (exit 1) if any tracked file
# would be reformatted. Safe for CI and the pre-commit hook.
# ------------------------------------------------------------------------
format-check:
	clang-format --dry-run --Werror $(shell git ls-files '*.c' '*.h')

# ------------------------------------------------------------------------
# make lint
# Runs cppcheck across the project's source/include dirs, failing (exit 1)
# on any finding - same severity as a compile error. Catches classes of
# bug -Wall/-Wextra don't (deeper dataflow, some MISRA-adjacent checks
# once the addon is configured).
# ------------------------------------------------------------------------
lint:
	cppcheck --enable=warning,style,performance,portability \
	  --std=c11 --error-exitcode=1 --inline-suppr \
	  --suppress=missingIncludeSystem \
	  $(INCLUDES) $(SRC_DIRS)

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