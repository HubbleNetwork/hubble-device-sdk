# Hubble Device SDK

Add global connectivity to your device using its Bluetooth® chip.

## What This Does

The SDK encodes your data into Bluetooth Low Energy (BLE) advertisements. Hubble's gateways pick them up and deliver your custom payload and device location data to your backend via API.

No cellular modem. No SIM card. No gateway hardware to deploy. Just add the SDK to your application firmware.

## Supported Platforms

Works with any Bluetooth LE 5.0+ chip.

→ [Full compatibility list](https://docs.hubble.com/docs/guides/supported-devices)

## Quick Start

**Fastest path:** Use a supported dev kit and follow the [Dash Quick Start](https://docs.hubble.com/docs/guides/dashboard/dash-quick-start) in your Hubble Dashboard. You'll have a device transmitting on the network in minutes.

**Reference apps:** [clone an app repository](https://docs.hubble.com/docs/guides/device-integration/reference-apps) for complete working examples.

**Integrating into existing firmware:** follow the guide for your RTOS.
   - [Zephyr](https://docs.hubble.com/docs/guides/device-integration/quick-start-terrestrial/zephyr)
   - [FreeRTOS](https://docs.hubble.com/docs/guides/device-integration/quick-start-terrestrial/freertos)
   - [ESP-IDF (Espressif)](https://docs.hubble.com/docs/guides/device-integration/quick-start-terrestrial/espressif)
   - [Bare-metal](https://docs.hubble.com/docs/guides/device-integration/quick-start-terrestrial/bare-metal)

#### Sign up for your Hubble Dashboard to get started.
1. [Sign up for free](https://dash.hubble.com/login).
2. [Register your device](https://docs.hubble.com/docs/guides/cloud-integration/register-devices) to get device encryption key.
3. Flash your device and start transmitting.
4. Get packet data via API or webhook.

## Resources

| | |
|---|---|
| **Documentation** | [docs.hubble.com/docs/device-sdk/intro](https://docs.hubble.com/docs/device-sdk/intro) |
| **Cloud API** | [docs.hubble.com/docs/api-specification/hubble-platform-api](https://docs.hubble.com/docs/api-specification/hubble-platform-api) |
| **Network & Security** | [docs.hubble.com/docs/network/terrestrial/transmission-guidance](https://docs.hubble.com/docs/network/terrestrial/transmission-guidance) |
| **Device Dashboard** | [dash.hubble.com](https://dash.hubble.com) |

## Questions & Support

- [GitHub Discussions](https://github.com/HubbleNetwork/hubble-device-sdk/discussions) — ask questions, share projects
- [GitHub Issues](https://github.com/HubbleNetwork/hubble-device-sdk/issues) — report bugs, request features
- [Contact Us](https://hubble.com/contact-us) — get a demo
