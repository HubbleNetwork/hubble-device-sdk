<!--
Template for sample READMEs.

Copy this file to samples/{port}/{sample}/README.md, fill in every {placeholder},
and delete each "(If ...)" line along with any section that does not apply.
Keep the sections in this order.

Conventions:
- Fence every code block with a language: sh, ps1, bat or text.
- Write tables with "| --- |" separators and no column padding.
- End every Requirements bullet with a period.
- Keep the satellite Requirements (PHY blob, SDR) together, at the end of the list.
- Link to docs in this repository with paths relative to the README, so the
  links match the revision the reader has checked out.
- Give paths to tools/ relative to the sample folder.
-->

# Hubble Network {Feature} Sample on {Port}

{One sentence: what the sample demonstrates.}

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  (If the sample uses [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands or `dual-stack-companion.py`).
- {Toolchain, linked to its install guide}.
- {Boards: the tested boards, or every supported board for a board-specific sample.
  For satellite, the link needs [at least +20 dBm](../docs/integration_guides/common/prerequisites.rst)
  of transmit power: mark each board that needs an external amplifier to reach it}.
- A computer{ with Bluetooth LE}{ and internet access} that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
  (If a host script or pyhubblenetwork is used).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
  (If the Test section uses `hubblenetwork` commands).
- The Hubble Connect app for
  [iOS](https://apps.apple.com/us/app/hubble-connect/id6751236191) or
  [Android](https://play.google.com/store/apps/details?id=com.hubble.connect)
  (optional), for the backend test.
  (If the Test section has a Backend Test).
- {Satellite PHY Blob}. (If the port needs one).
- A software defined radio (SDR) for validation (suggested, but not required).
  (If the sample transmits to satellite).

## Overview

{What the sample does, as a short paragraph or bullet list.}

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/{port}/{sample}`

## Environment Setup

First, set up the environment. This step assumes you've {installed the toolchain
at a given location}. If you haven't, follow {link to the install guide}.
Use the following command to {load the toolchain environment}.

### Windows

```ps1
{command}
```

### Linux & macOS

```sh
{command}
```

### Python Dependencies

(If a host script or pyhubblenetwork is used.)

Then install {the Python dependencies for the `{script}` provisioning script, and}
the [pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install {-r {relative path}/tools/requirements-{name}.txt} pyhubblenetwork
```

### Satellite PHY Blob

(If the port needs a blob.)

{How to fetch and install it, or a link to the steps.}

## BLE Advertising Parameters

(If the sample advertises over BLE.)

The sample advertises with the following parameters by default. To change one,
edit the macro or option in the *Set by* column and rebuild.

| Parameter | Value | Set by |
| --- | --- | --- |
| Beacon interval | {min}–{max} ms | `{macro}` in [{file}]({file}) |
| Tx power | {value} dBm | `{option}` in [{file}]({file}) |

## Configurations

(If the sample exposes options.)

This sample exposes the following options ({where they are set, e.g. the menuconfig menu name}):

| Option | Default | Description |
| --- | --- | --- |
| `{option}` | `{default}` | {Description.} |

## Build-time Provisioning

(If the sample needs a device key at build time.)

{How to supply the device key.} The key must be registered as
**{AES-128 or AES-256}** with the **{Unix time or Device uptime}** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).

## Build and Flash

{Build command, then a concrete example, then the flash command and how to view the log.}

```sh
{build command}
```

e.g.
```sh
{build command for a real board}
```

## Runtime Provisioning

(If the device is provisioned after flashing, e.g. by `dual-stack-companion.py` or shell commands.)

{Steps, with Windows and Linux & macOS commands where they differ, and the
message that confirms success.}

## Program Flow

(If the sample has a non-obvious life-cycle, e.g. dual-stack.)

{Numbered steps, then a text diagram of the life-cycle.}

## Test

{How to confirm the sample works, one subsection per check. Each check gives the
command or steps, then the output that shows it worked (a `text` block of sample
output, or what to look for). Typical checks:}

### Local BLE Scanning

{`hubblenetwork ble scan` command, then the decrypted packets it prints.}

### Backend Test

{Hubble Connect app and Hubble Dash, then where the device appears.}

### Satellite Network

{`hubblenetwork sat scan` with an SDR, linking to RF Verification with an SDR,
then the decoded packets it prints. DTM samples instead describe the serial
prompt, then a Shell Commands table and an Example Session.}
