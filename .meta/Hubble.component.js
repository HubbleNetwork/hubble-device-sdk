/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

"use strict";

let topModules;

const displayName = "Hubble Device SDK";
const description = "Hubble Device SDK Options";

topModules = [
    {
        displayName: displayName,
        description: description,
        modules: [
            "/Hubble",
        ]
    }
];

let templates = [
    {
        "name": "/hubble_sdk_sources.c.xdt",
        "outputPath": "HubbleSDKSources.c",
        "alwaysRun": true
    },
];

exports = {
    displayName: displayName,
    topModules: topModules,
    templates: templates
};
