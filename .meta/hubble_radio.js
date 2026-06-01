/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

function config() {
    const RCL      = scripting.addModule("/ti/drivers/RCL");
    const custom   = scripting.addModule("/ti/devices/radioconfig/custom");

    custom.ble                                              = ["ble_gen"];
    custom.radioConfigble_gen.$name                         = "RF_BLE_Setting";
    custom.radioConfigble_gen.codeExportConfig.$name        = "ti_devices_radioconfig_code_export_param0";
    custom.radioConfigble_gen.codeExportConfig.cmdList_ble  = ["cmdGenericTxTest"];
    custom.radioConfigble_gen.codeExportConfig.symGenMethod = "Automatic";

    /* Exclude the generated radio config file since we use our own */
    scripting.excludeFromBuild("ti_radio_config.c");
}

function config_dual_stack(role) {
    const RCL      = scripting.addModule("/ti/drivers/RCL");
    const custom   = scripting.addModule("/ti/devices/radioconfig/custom");
    const dmm      = scripting.addModule("/ti/dmm/dmm");

    custom.ble                                              = ["ble_gen"];
    custom.radioConfigble_gen.$name                         = "ti_devices_radioconfig_phy_groups_ble0";
    custom.radioConfigble_gen.txPower                       = "20";
    custom.radioConfigble_gen.codeExportConfig.$name        = "ti_devices_radioconfig_code_export_param3";
    custom.radioConfigble_gen.codeExportConfig.symGenMethod = "Automatic";
    custom.radioConfigble_gen.codeExportConfig.cmdList_ble  = ["cmdGenericTxTest"];

    dmm.stackRoles                         = [role, "custom1"];
    dmm.policyArray[0].$name               = "ti_dmm_policy_dmm_policy0";
    dmm.policyArray[0].custom1.$name       = "ti_dmm_policy_stack_dmm_stack_custom0";
    dmm.policyArray[0][role].$name         = "ti_dmm_policy_stack_dmm_stack_custom1";

    /* Exclude the generated radio config file since we use our own */
    scripting.excludeFromBuild("ti_radio_config.c");
}

exports = {
    config: config,
    config_dual_stack: config_dual_stack,
}
