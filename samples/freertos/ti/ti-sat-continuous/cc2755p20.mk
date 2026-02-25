SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR ?= /Applications/ti/simplelink_lowpower_f3_sdk_9_20_00_10_ea
HUBBLE_NETWORK_SDK ?= /Users/hunterpatchett/hitl-endpoint/zephyr-workspace/hubble-device-sdk
NAME = sat-continuous

# Build directory for all artifacts
BUILD_DIR ?= build

include $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/imports.mak

CC = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"
LNK = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"

SYSCONFIG_GUI_TOOL = $(dir $(SYSCONFIG_TOOL))sysconfig_gui$(suffix $(SYSCONFIG_TOOL))
SYSCFG_CMD_STUB = $(SYSCONFIG_TOOL) --compiler ticlang --product $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_GUI_CMD_STUB = $(SYSCONFIG_GUI_TOOL) --compiler ticlang --product $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_FILES := $(shell $(SYSCFG_CMD_STUB) --listGeneratedFiles --listReferencedFiles --output $(BUILD_DIR) sat-continuous-cc27.syscfg)

SYSCFG_C_FILES = $(filter %.c,$(SYSCFG_FILES))
SYSCFG_H_FILES = $(filter %.h,$(SYSCFG_FILES))
SYSCFG_OPT_FILES = $(filter %.opt,$(SYSCFG_FILES))

# Enable verbose output by setting VERBOSE=1
V := @
ifeq ($(VERBOSE), 1)
  V :=
endif

OBJECTS = $(addprefix $(BUILD_DIR)/, \
    freertos_main_freertos.obj \
    $(patsubst %.c,%.obj,$(notdir $(SYSCFG_C_FILES))))


CFLAGS += -I../.. \
    -I. \
	-Isrc/ \
	-I$(BUILD_DIR) \
	"-I$(HUBBLE_NETWORK_SDK)/include" \
	"-I$(HUBBLE_NETWORK_SDK)/port/freertos" \
	-DENODATA=61 \
	-DDeviceFamily_CC27XXX10 \
    $(addprefix @,$(SYSCFG_OPT_FILES)) \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source" \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/kernel/freertos" \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/posix/ticlang" \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/third_party/freertos/include" \
    "-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/third_party/freertos/portable/GCC/ARM_CM33_NTZ/non_secure" \
    -gdwarf-3 \
    -mcpu=cortex-m33 \
    -mfpu=fpv5-sp-d16 \
    -mfloat-abi=hard \
    -mthumb

LFLAGS += "-L$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source" \
    $(BUILD_DIR)/ti_utils_build_linker.cmd.genlibs \
    $(BUILD_DIR)/lpf3_freertos.cmd \
    "-Wl,-m,$(BUILD_DIR)/$(NAME).map" \
    -Wl,--rom_model \
    -Wl,--warn_sections \
    "-L$(TICLANG_ARMCOMPILER)/lib" \
    -llibc.a

all: $(BUILD_DIR) postbuild

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@cp lpf3_freertos.cmd $(BUILD_DIR)

.PHONY: postbuild
postbuild: $(BUILD_DIR)/$(NAME).out
	$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/tools/common/crc_tool/crc_tool patch-image --elf $(BUILD_DIR)/$(NAME).out --symbol-prefix ti_utils_build_GenMap_sym_CRC --output $(BUILD_DIR)/$(NAME).out
	$(TICLANG_ARMCOMPILER)/bin/tiarmobjcopy -O ihex $(BUILD_DIR)/$(NAME).out $(BUILD_DIR)/$(NAME).hex

.INTERMEDIATE: syscfg
$(SYSCFG_FILES): syscfg
	@ echo generation complete

syscfg: sat-continuous-cc27.syscfg $(BUILD_DIR)
	@ echo Generating configuration files...
	$(V) $(SYSCFG_CMD_STUB) --output $(BUILD_DIR) $<


# Helpful hint that the user needs to use a standalone SysConfig installation
$(SYSCONFIG_GUI_TOOL):
	$(error $(dir $(SYSCONFIG_TOOL)) does not contain the GUI framework \
        necessary to launch the SysConfig GUI.  Please set SYSCONFIG_TOOL \
        (in your SDK's imports.mak) to a standalone SysConfig installation \
        rather than one inside CCS)

syscfg-gui: sat-continuous-cc27.syscfg $(SYSCONFIG_GUI_TOOL)
	@ echo Opening SysConfig GUI
	$(V) $(SYSCFG_GUI_CMD_STUB) $<


define C_RULE
$(BUILD_DIR)/$(basename $(notdir $(1))).obj: $(1) $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $$@
	$(V) $(CC) $(CFLAGS) -c "$$<" -o $$@
endef

$(foreach c_file,$(SYSCFG_C_FILES),$(eval $(call C_RULE,$(c_file))))

# HubbleNetwork SDK

include $(HUBBLE_NETWORK_SDK)/port/freertos/hubblenetwork-sdk.mk

OBJECTS += $(BUILD_DIR)/hubble_ti_crypto.obj $(BUILD_DIR)/radio.obj $(patsubst %.c,$(BUILD_DIR)/%.obj,$(notdir $(HUBBLENETWORK_SDK_SOURCES)))

define HUBBLE_RULE
$(BUILD_DIR)/$(basename $(notdir $(1))).obj: $(1) $(BUILD_DIR)
	@ echo Building $$@
	$(V) $(CC) $(CFLAGS) $(HUBBLENETWORK_SDK_FLAGS) -c $$< -o $$@
endef

$(foreach c_file,$(HUBBLENETWORK_SDK_SOURCES),$(eval $(call HUBBLE_RULE,$(c_file))))

$(BUILD_DIR)/hubble_ti_crypto.obj: src/hubble_ti_crypto.c $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $(HUBBLENETWORK_SDK_FLAGS) -c $< -o $@

$(BUILD_DIR)/radio.obj: src/radio.c $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $(HUBBLENETWORK_SDK_FLAGS) -c $< -o $@

# HubbleNetwork SDK END

$(BUILD_DIR)/freertos_main_freertos.obj: src/main.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $(HUBBLENETWORK_SDK_FLAGS) -c $< -o $@

$(BUILD_DIR)/$(NAME).out: $(OBJECTS)
	@ echo linking $@
	$(V) $(LNK) -Wl,-u,_c_int00 $(OBJECTS)  $(LFLAGS) -o $@

clean:
	@ echo Cleaning...
	$(V) $(RM) -rf $(BUILD_DIR) > $(DEVNULL) 2>&1
	$(V) $(RM) $(call SLASH_FIXUP,$(SYSCFG_FILES)) > $(DEVNULL) 2>&1
