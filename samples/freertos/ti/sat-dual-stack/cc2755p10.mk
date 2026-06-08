SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR ?= $(abspath ../../../../../../..)

HUBBLE_NETWORK_SDK ?= $(abspath ../../../../)
NAME = sat-dual-stack

# Debug flag
DEBUG ?= 0

# Build directory for all artifacts
BUILD_DIR ?= build

include $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/imports.mak

CC = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"
LNK = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"

SYSCONFIG_GUI_TOOL = $(dir $(SYSCONFIG_TOOL))sysconfig_gui$(suffix $(SYSCONFIG_TOOL))
SYSCFG_CMD_STUB = $(SYSCONFIG_TOOL) --compiler ticlang -s $(HUBBLE_NETWORK_SDK)/.metadata/product.json --product $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_GUI_CMD_STUB = $(SYSCONFIG_GUI_TOOL) --compiler ticlang -s $(HUBBLE_NETWORK_SDK)/.metadata/product.json --product $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_FILES := $(shell $(SYSCFG_CMD_STUB) --listGeneratedFiles --listReferencedFiles --output $(BUILD_DIR) sat-dual-stack-cc27.syscfg)

SYSCFG_C_FILES = $(filter %.c,$(SYSCFG_FILES))
SYSCFG_H_FILES = $(filter %.h,$(SYSCFG_FILES))
SYSCFG_OPT_FILES = $(filter %.opt,$(SYSCFG_FILES))

# Enable verbose output by setting VERBOSE=1
V := @
ifeq ($(VERBOSE), 1)
  V :=
endif

OBJECTS = $(addprefix $(BUILD_DIR)/, \
	app_main.obj \
	app_hubble_ti_crypto.obj \
	app_ble.obj \
	common_Services_dev_info_dev_info_service.obj \
	common_BLEAppUtil_bleapputil_task.obj \
	common_BLEAppUtil_bleapputil_init.obj \
	common_BLEAppUtil_bleapputil_process.obj \
	common_BLEAppUtil_bleapputil_stack_callbacks.obj \
	common_lib_opt_ctrl_opt_connectable.obj \
	common_lib_opt_ctrl_opt_adv_conn.obj \
	common_lib_opt_ctrl_opt_adv_nconn.obj \
	common_Startup_osal_icall_ble.obj \
	common_Startup_rom_init.obj \
	common_Drivers_NV_crc.obj \
	common_Drivers_NV_nvocmp.obj \
	common_iCall_icall.obj \
	common_iCall_icall_user_config.obj \
	common_iCall_icall_POSIX.obj \
	common_config_ble_user_config.obj \
	common_Profiles_simple_gatt_simple_gatt_profile.obj \
	common_lib_opt_ctrl_opt_dmm.obj \
	common_lib_opt_ctrl_opt_dmm_dynamic_priority.obj \
	common_BLE_SysStat_blesysstat.obj \
	$(patsubst %.c,%.obj,$(notdir $(SYSCFG_C_FILES))))


CFLAGS += -I. \
	-I../.. \
	-Isrc/ \
	-I$(BUILD_DIR) \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/common/cc26xx/flash_interface/" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/posix/ticlang" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/third_party/freertos/include" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/third_party/freertos/portable/GCC/ARM_CM33_NTZ/non_secure" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/kernel/freertos" \
	$(addprefix @,$(SYSCFG_OPT_FILES)) \
	"@$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/config/build_components.opt" \
	"@$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/config/factory_config.opt" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/drivers" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/drivers/dpl/" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/drivers/rcl" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/common/" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/common/nv" \
	"-I$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/dmm/apps/mac_sensor_ble" \
	-DSYNC_WRITE \
	-DICALL_NO_APP_EVENTS \
	-mthumb \
	-mlittle-endian \
	-ffunction-sections \
	-g \
	-Oz \
	-DUSE_HSM \
	-DNVOCMP_NWSAMEITEM=1 \
	-DFREERTOS \
	-DNVOCMP_POSIX_MUTEX \
	-gdwarf-3 \
	-mcpu=cortex-m33 \
	-mfloat-abi=hard \
	-mfpu=fpv5-sp-d16

ifeq ($(DEBUG),1)
	CFLAGS += -DCONFIG_DEBUG=1
endif

LFLAGS += -Wl,--diag_wrap=off \
	-Wl,--display_error_number \
	-Wl,--reread_libs  \
	-Oz \
	-Wl,-m,$(BUILD_DIR)/$(NAME).map \
	-Wl,--rom_model \
	-Wl,--warn_sections \
	-L$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source \
	-L$(TICLANG_ARMCOMPILER)/lib \
	$(BUILD_DIR)/ti_utils_build_linker.cmd.genlibs \
	$(BUILD_DIR)/cc27xx_freertos.cmd \
	-llibc.a

all: $(BUILD_DIR) postbuild

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@cp cc27xx_freertos.cmd $(BUILD_DIR)

.PHONY: postbuild
postbuild: $(BUILD_DIR)/$(NAME).out
	$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/tools/common/crc_tool/crc_tool patch-image --elf $(BUILD_DIR)/$(NAME).out --symbol-prefix ti_utils_build_GenMap_sym_CRC_CCFG -o $(BUILD_DIR)/$(NAME).out
	$(TICLANG_ARMCOMPILER)/bin/tiarmobjcopy -O ihex $(BUILD_DIR)/$(NAME).out $(BUILD_DIR)/$(NAME).hex
	$(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/tools/common/crc_tool/crc_tool patch-image --elf $(BUILD_DIR)/$(NAME).out --symbol-prefix ti_utils_build_GenMap_sym_CRC --output $(BUILD_DIR)/$(NAME).out
	$(TICLANG_ARMCOMPILER)/bin/tiarmobjcopy -O ihex $(BUILD_DIR)/$(NAME).out $(BUILD_DIR)/$(NAME).hex

.INTERMEDIATE: syscfg
$(SYSCFG_FILES): syscfg
	@ echo generation complete

syscfg: sat-dual-stack-cc27.syscfg $(BUILD_DIR)
	@ echo Generating configuration files...
	$(V) $(SYSCFG_CMD_STUB) --output $(BUILD_DIR) $<


# Helpful hint that the user needs to use a standalone SysConfig installation
$(SYSCONFIG_GUI_TOOL):
	$(error $(dir $(SYSCONFIG_TOOL)) does not contain the GUI framework \
        necessary to launch the SysConfig GUI.  Please set SYSCONFIG_TOOL \
        (in your SDK's imports.mak) to a standalone SysConfig installation \
        rather than one inside CCS)

syscfg-gui: sat-dual-stack-cc27.syscfg $(SYSCONFIG_GUI_TOOL)
	@ echo Opening SysConfig GUI
	$(V) $(SYSCFG_GUI_CMD_STUB) $<

define C_RULE
$(BUILD_DIR)/$(basename $(notdir $(1))).obj: $(1) $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $$@
	$(V) $(CC) $(CFLAGS) -c $$< -o $$@
endef

$(foreach c_file,$(SYSCFG_C_FILES),$(eval $(call C_RULE,$(c_file))))


$(BUILD_DIR)/app_hubble_ti_crypto.obj: src/hubble_ti_crypto.c $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/app_main.obj: src/main.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/app_ble.obj: src/app_ble.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_BLE_SysStat_blesysstat.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/health_toolkit/src/ble_sys_stat.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_Services_dev_info_dev_info_service.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/services/dev_info/src/dev_info_service.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_BLEAppUtil_bleapputil_task.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/app_util/framework/src/bleapputil_task.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_BLEAppUtil_bleapputil_init.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/app_util/framework/src/bleapputil_init.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_BLEAppUtil_bleapputil_process.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/app_util/framework/src/bleapputil_process.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_BLEAppUtil_bleapputil_stack_callbacks.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/app_util/framework/src/bleapputil_stack_callbacks.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_lib_opt_ctrl_opt_connectable.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/lib_opt/src/ctrl_opt_connectable.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_lib_opt_ctrl_opt_adv_conn.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/lib_opt/src/ctrl_opt_adv_conn.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_lib_opt_ctrl_opt_adv_nconn.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/lib_opt/src/ctrl_opt_adv_nconn.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_Startup_osal_icall_ble.obj: src/common/Startup/osal_icall_ble.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_Startup_rom_init.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/lib_opt/rom_init.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_Drivers_NV_crc.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/common/nv/crc.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_Drivers_NV_nvocmp.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/common/nv/nvocmp.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_iCall_icall.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/icall/app/src/icall.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_iCall_icall_user_config.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/icall/app/src/icall_user_config.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_config_ble_user_config.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/app_util/config/src/ble_user_config.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_Profiles_simple_gatt_simple_gatt_profile.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/profiles/simple_gatt/src/simple_gatt_profile.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_iCall_icall_POSIX.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/icall/app/src/icall_POSIX.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_lib_opt_ctrl_opt_dmm.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/lib_opt/src/ctrl_opt_dmm.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common_lib_opt_ctrl_opt_dmm_dynamic_priority.obj: $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/source/ti/ble/stack_util/lib_opt/src/ctrl_opt_dmm_dynamic_priority.c $(SYSCFG_H_FILES) $(BUILD_DIR)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(NAME).out: $(OBJECTS)
	@ echo linking $@
	$(V) $(LNK) -Wl,-u,_c_int00 $(OBJECTS)  $(LFLAGS) -o $@

clean:
	@ echo Cleaning...
	$(V) $(RM) -rf $(BUILD_DIR) > $(DEVNULL) 2>&1
	$(V) $(RM) $(call SLASH_FIXUP,$(SYSCFG_FILES)) > $(DEVNULL) 2>&1
