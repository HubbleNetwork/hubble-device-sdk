# Simplified makefile for tone_test - no HubbleNetwork SDK dependency
# Usage: make -f tone_test.mk SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR=/path/to/sdk TICLANG_ARMCOMPILER=/path/to/compiler

NAME = tone_test
BUILD_DIR ?= build

# These must be set - either via environment or command line
SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR ?= $(error Set SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR to your SDK path)
TICLANG_ARMCOMPILER ?= $(error Set TICLANG_ARMCOMPILER to your TI ARM compiler path)

CC = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"
LNK = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"

# SysConfig - standalone installation (not bundled with SDK)
SYSCONFIG_TOOL ?= /Applications/ti/sysconfig_1.23.2/sysconfig_cli.sh
SYSCFG_CMD_STUB = $(SYSCONFIG_TOOL) --compiler ticlang --product $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/.metadata/product.json

# Verbose mode
V := @
ifeq ($(VERBOSE), 1)
  V :=
endif

# Sysconfig generated files
SYSCFG_C_FILES = \
    $(BUILD_DIR)/ti_drivers_config.c \
    $(BUILD_DIR)/ti_radio_config.c \
    $(BUILD_DIR)/ti_devices_config.c \
    $(BUILD_DIR)/ti_freertos_config.c \
    $(BUILD_DIR)/ti_freertos_portable_config.c

SYSCFG_H_FILES = \
    $(BUILD_DIR)/ti_drivers_config.h \
    $(BUILD_DIR)/ti_radio_config.h \
    $(BUILD_DIR)/FreeRTOSConfig.h

# Object files
OBJECTS = \
    $(BUILD_DIR)/tone_test.obj \
    $(BUILD_DIR)/ti_drivers_config.obj \
    $(BUILD_DIR)/ti_radio_config.obj \
    $(BUILD_DIR)/ti_devices_config.obj \
    $(BUILD_DIR)/ti_freertos_config.obj \
    $(BUILD_DIR)/ti_freertos_portable_config.obj

CFLAGS += -I../.. \
    -I. \
    -Isrc/ \
    -I$(BUILD_DIR) \
    -DDeviceFamily_CC23X0R5 \
    -DTONE_DURATION_MS=5000 \
    @$(BUILD_DIR)/ti_utils_build_compiler.opt \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source" \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/kernel/freertos" \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/posix/ticlang" \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/third_party/freertos/include" \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/third_party/freertos/portable/GCC/ARM_CM0" \
    -DCC23X0 \
    -gdwarf-3 \
    -mcpu=cortex-m0plus \
    -march=thumbv6m \
    -mfloat-abi=soft \
    -mthumb

LFLAGS += "-L$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source" \
    "-L$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/kernel/freertos" \
    -L$(BUILD_DIR) \
    $(BUILD_DIR)/ti_utils_build_linker.cmd.genlibs \
    $(BUILD_DIR)/lpf3_freertos.cmd \
    "-Wl,-m,$(BUILD_DIR)/$(NAME).map" \
    -Wl,--rom_model \
    -Wl,--warn_sections \
    "-L$(TICLANG_ARMCOMPILER)/lib" \
    -llibc.a

all: $(BUILD_DIR)/$(NAME).out
	@echo "Build complete: $(BUILD_DIR)/$(NAME).out"

# Create build directory and copy linker script
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@cp lpf3_freertos.cmd $(BUILD_DIR)

# Generate sysconfig files - this must run first
.PHONY: syscfg
syscfg: $(BUILD_DIR)
	@echo Generating configuration files...
	$(V) $(SYSCFG_CMD_STUB) --output $(BUILD_DIR) sat-continuous-cc23.syscfg

# Mark sysconfig output files as dependent on syscfg target
$(SYSCFG_C_FILES) $(SYSCFG_H_FILES) $(BUILD_DIR)/ti_utils_build_linker.cmd.genlibs $(BUILD_DIR)/ti_utils_build_compiler.opt: syscfg

# Sysconfig generated C files
$(BUILD_DIR)/ti_drivers_config.obj: $(BUILD_DIR)/ti_drivers_config.c $(SYSCFG_H_FILES)
	@echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ti_radio_config.obj: $(BUILD_DIR)/ti_radio_config.c $(SYSCFG_H_FILES)
	@echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ti_devices_config.obj: $(BUILD_DIR)/ti_devices_config.c $(SYSCFG_H_FILES)
	@echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ti_freertos_config.obj: $(BUILD_DIR)/ti_freertos_config.c $(SYSCFG_H_FILES)
	@echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ti_freertos_portable_config.obj: $(BUILD_DIR)/ti_freertos_portable_config.c $(SYSCFG_H_FILES)
	@echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Main application
$(BUILD_DIR)/tone_test.obj: src/tone_test.c $(SYSCFG_H_FILES)
	@echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(NAME).out: $(OBJECTS) $(BUILD_DIR)/ti_utils_build_linker.cmd.genlibs
	@echo Linking $@
	$(V) $(LNK) -Wl,-u,_c_int00 $(OBJECTS) $(LFLAGS) -o $@

# Generate hex file
hex: $(BUILD_DIR)/$(NAME).out
	$(TICLANG_ARMCOMPILER)/bin/tiarmobjcopy -O ihex $(BUILD_DIR)/$(NAME).out $(BUILD_DIR)/$(NAME).hex

clean:
	@echo Cleaning...
	$(V) rm -rf $(BUILD_DIR)

.PHONY: all clean hex
