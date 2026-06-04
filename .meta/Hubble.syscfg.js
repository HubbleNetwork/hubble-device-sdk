/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

"use strict";

const topModuleDisplayName = "Hubble Network SDK";
const topModuleDescription = "Hubble Network SDK Configuration";

function validate(mod, validation)
{
    //TODO
}

var sdkFiles = [
    "src/hubble.c",
    "src/hubble_crypto.c",
    "port/freertos/hubble_freertos.c"
];

var sdkSatFiles = [
    "port/freertos/hubble_sat_freertos.c",
    "src/hubble_sat.c",
    "src/hubble_sat_packet.c",
    "src/hubble_sat_pass_prediction.c",
    "src/reed_solomon_encoder.c",
    "src/utils/bitarray.c"
];

var sdkBleFiles = [
    "src/hubble_ble.c"
];

var cc23xxDeviceFiles = [
    "port/freertos/boards/ti/cc23xx_cc27xx/radio.c",
    "port/freertos/boards/ti/cc23xx_cc27xx/radio_config/ti_radio_config_cc23.c"
];

var cc23xxDMMDeviceFiles = [
    "port/freertos/boards/ti/cc23xx_cc27xx/radio.c",
    "port/freertos/boards/ti/cc23xx_cc27xx/dmm/dmm_priority_ble_custom.c",
    "port/freertos/boards/ti/cc23xx_cc27xx/dmm/rcl_override_dmm.c",
    "port/freertos/boards/ti/cc23xx_cc27xx/radio_config/ti_radio_config_cc23_dmm.c"
];

var cc27xxDeviceFiles = [
    "port/freertos/boards/ti/cc23xx_cc27xx/radio.c",
    "port/freertos/boards/ti/cc23xx_cc27xx/radio_config/ti_radio_config_cc27.c"
];

var cc27xxDMMDeviceFiles = [
    "port/freertos/boards/ti/cc23xx_cc27xx/radio.c",
    "port/freertos/boards/ti/cc23xx_cc27xx/dmm/dmm_priority_ble_custom.c",
    "port/freertos/boards/ti/cc23xx_cc27xx/dmm/rcl_override_dmm.c",
    "port/freertos/boards/ti/cc23xx_cc27xx/radio_config/ti_radio_config_cc27_dmm.c"
];

function getSDKRootDir()
{
    const product = system.getProducts().find(p => p.name == "Hubble Network SDK");
    if (product) {
        let sdkPath = system.utils.path.parse(
            system.utils.path.parse(product.path).dir
        ).dir;

        sdkPath = system.utils.path.resolve(sdkPath).replaceAll("\\", "/");
        return `${sdkPath}`;
    }

    return "";
}

function getOpts(mod) {
    const result = [];

    const sysconfig_products = ["simplelink_lowpower_f3_sdk", "SIMPLELINK_LOWPOWER_F3_TEST_SDK"];
    let sdkPath = undefined;
    for(let i = 0; i < sysconfig_products.length; i++) {
        sdkPath = system.getProducts().find(p => p.name === sysconfig_products[i]);
        if(sdkPath !== undefined) {
            // WARNING using system.utils.path.posix breaks the paths on windows
            sdkPath = system.utils.path.parse(system.utils.path.parse(sdkPath.path).dir).dir;
            sdkPath = system.utils.path.resolve(sdkPath).replaceAll('\\','/');
            break;
        }
    }

    let hubble = system.modules["/Hubble"].$static;

    if (hubble.useSatellite) {
        result.push(`-DCONFIG_HUBBLE_SAT_NETWORK`);
        result.push(`-DCONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR=${hubble.deviceTDR}`);

        if (hubble.dtmMode) {
            result.push(`-DCONFIG_HUBBLE_SAT_NETWORK_DTM_MODE`);
        }
    }

    if (hubble.useTerrestrial) {
        result.push(`-DCONFIG_HUBBLE_BLE_NETWORK`);
    }

    const keySize = hubble.keySize;
    if (keySize === "128") {
        result.push(`-DCONFIG_HUBBLE_NETWORK_KEY_128`);
        result.push(`-DCONFIG_HUBBLE_KEY_SIZE=16`);
    } else if (keySize === "256") {
        result.push(`-DCONFIG_HUBBLE_NETWORK_KEY_256`);
        result.push(`-DCONFIG_HUBBLE_KEY_SIZE=32`);
    }

    const eidRotationPeriod = hubble.eidRotationPeriod;
    result.push(`-DCONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC=${eidRotationPeriod}`);

    const counterSource = hubble.counterSource;
    if (counterSource == "unix-time") {
        result.push(`-DCONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME`);
    } else if (counterSource == "device-uptime") {
        result.push(`-DCONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME`);
    }

    if (hubble.enforceNonceCheck) {
        result.push(`-DCONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK`);
    }

    if (hubble.useCustomNonceSequence) {
        result.push(`-DCONFIG_HUBBLE_NETWORK_SEQUENCE_NONCE_CUSTOM`);
    }

    if (hubble.useCustomUptime) {
        result.push(`-DCONFIG_HUBBLE_UPTIME_CUSTOM`);
    }

    const crypto = hubble.crypto;
    if (crypto == "custom") {
        result.push(`-DCONFIG_HUBBLE_NETWORK_CRYPTO_CUSTOM`);
    } else if (crypto == "psa") {
        result.push(`-DCONFIG_HUBBLE_NETWORK_CRYPTO_PSA`);
    } else if (crypto == "mbedtls") {
        result.push(`-DCONFIG_HUBBLE_NETWORK_CRYPTO_MBEDTLS`);
    }

    result.push(`-I${getSDKRootDir()}/src/`);
    result.push(`-I${getSDKRootDir()}/include/`);
    result.push(`-I${getSDKRootDir()}/port/freertos/`);

    if (hubble.useSatellite && hubble.useTerrestrial) {
        result.push(`-DCC23X0`);
        if (system.deviceData.deviceId.match(/CC27../)) {
            result.push(`-DCC27`);
        }
        result.push(`-DUSE_DMM_OVRDE`);
        result.push(`-DUSE_DMM`);
        result.push(`-DUSE_DMM_DYNAMIC_PRIORITY`);
        result.push(`-I${getSDKRootDir()}/port/freertos/boards/ti/cc23xx_cc27xx/dmm/`)
        result.push(`-I${sdkPath}/source/ti/dmm`);
    }

    return result;
}

function getSDKFiles()
{
    let hubble = system.modules["/Hubble"].$static;
    let sdkPath = getSDKRootDir();
    let files = [...sdkFiles];

    // Sat
    if (hubble.useSatellite) {
        files.push(...sdkSatFiles);
        if (system.deviceData.deviceId.match(/CC27../)) {
            if (hubble.useTerrestrial) {
                files.push(...cc27xxDMMDeviceFiles);
            } else {
                files.push(...cc27xxDeviceFiles);
            }
        } else if (system.deviceData.deviceId.match(/CC23../)) {
            if (hubble.useTerrestrial) {
                files.push(...cc23xxDMMDeviceFiles);
            } else {
                files.push(...cc23xxDeviceFiles);
            }
        }
    }

    // Terrestrial
    if (hubble.useTerrestrial) {
        files.push(...sdkBleFiles);
    }

    return files.map(f => `${sdkPath}/${f}`)
}

let base = {
    staticOnly: true,
    displayName: "HubbleNetwork",
    moduleStatic: {
        name: "HubbleNetworkSDK",
        validate: validate,
        config: [
            {
                name: "useSatellite",
                displayName: "Enable Hubble Satellite Network",
                description: `Enable or disable Hubble Satellite Network`,
                longDescription: `
When set to false, Hubble Satellite Network is disabled and not available.`,
                default: false,
                onChange: onUseSatelliteChange
            },
            {
                name: "useTerrestrial",
                displayName: "Enable Hubble Terrestrial Network",
                description: `Enable or disable Hubble Terrestrial Network`,
                longDescription: `
When set to false, Hubble Terrestrial Network is disabled and not available.`,
                default: true

            },
            {
                name: "deviceTDR",
                displayName: "Device time drift retry rate in PPM",
                longDescription: `
Compensates for clock drift by adding retransmission
retries proportional to time elapsed since the last
time sync. Value is in parts per million (PPM).

Higher values add more retries for the same drift,
increasing reliability but also power consumption.
Default of 10 PPM is suitable for typical crystal
oscillators.
`,
                default: 10

            },
            {
            name: "dtmMode",
            displayName: "Enable DTM Mode",
            description: `Enable Direct Test Mode`,
            longDescription: `
Enable DTM (Direct Test Mode) in the SDK, this is intended
to test the operation of the radio
`,
            default: false,
            hidden: true
        },
            {
                name: "eidRotationPeriod",
                displayName: "EID rotation period",
                description: `EID rotation period in seconds`,
                default: 86400
            },
            {
                name: "counterSource",
                displayName: "Counter source",
                description: "Counter source",
                longDescription: `
Selects how the SDK computes the time counter used for
EID rotation and key derivation.
`,
                default: "unix-time",
                options: [
                    {
                        name: "unix-time",
                        displayName: "Unix time counter source"
                    },
                    {
                        name: "device-uptime",
                        displayName: "Device uptime counter source"
                    }
                ],
            },
            {
                name: "keySize",
                displayName: "Key size - 128 or 256",
                default: "128",
                options: [
                    {
                        name: "128",
                        displayName: "128 bits key"
                    },
                    {
                        name: "256",
                        displayName: "256 bits key"
                    }
                ],
            },
            {
                name: "crypto",
                displayName: "Cryptographic implementation to use",
                default: "custom",
                options: [
                    {
                        name: "custom",
                        displayName: "Custom crypto implementation",
                        description: `
Provide a custom cryptographic backend. The application
must implement all functions declared in
<hubble/port/crypto.h>
`,
                    },
                    {
                        name: "psa",
                        displayName: "Use PSA Crypto",
                        description: `
Use the PSA Crypto API for AES-CTR and CMAC. This is
the recommended backend for Zephyr and nRF platforms.
When building with TF-M, cryptographic operations are
delegated to the secure processing environment.
`,
                    },
                    {
                        name: "mbedtls",
                        displayName: "Use MbedTLS",
                        description: `
Use the MbedTLS library directly for AES-CTR and CMAC.
`,
                    }
                ],
            },
            {
                name: "enforceNonceCheck",
                displayName: "Enforce check in nonce used in cryptography",
                description: `
Validates that the encryption nonce is unique before
encrypting. Prevents nonce reuse, which would compromise
AES-CTR confidentiality.

Enabled by default. Disabling removes the runtime check
but risks catastrophic security failure if nonces are
ever repeated. Only disable for testing or when the
application guarantees uniqueness by other means.
`,
                default: true
            },
            {
                name: "useCustomNonceSequence",
                displayName: "Application-defined sequence counter",
                description: `
Override the default auto-incrementing sequence counter
with an application-provided implementation. Useful for
deterministic testing or persisting counters across
reboots.
`,
                default: false
            },
            {
                name: "useCustomUptime",
                displayName: "Application-defined uptime implementation",
                description: `
Override the default platform uptime function with an
application-provided implementation. Primarily useful
for unit testing where controlled time values are
needed to verify time-dependent behavior.
`,
                default: false
            },
        ]
    },
    templates: {
        "/ti/utils/build/GenOpts.opt.xdt": {
            modName: "/Hubble",
            getOpts: getOpts
        }
    },
    getSDKFiles: getSDKFiles,
}

/*
 * Show or hide satellite sub-options
 * @param inst  - Module instance containing the config that changed
 * @param ui    - The User Interface object
 */
function onUseSatelliteChange(inst, ui)
{
    ui.dtmMode.hidden = !inst.useSatellite;

    // reset so a stale "true" can't leak when sat is disabled
    if (!inst.useSatellite) {
        inst.dtmMode = false;
    }
}

/* export the module */
exports = base;
