# Hubble BLE Protocol Specification

**Version:** 1.0
**Status:** Draft
**Date:** 2026-02-04

## Table of Contents

1. [Introduction](#1-introduction)
2. [Protocol Overview](#2-protocol-overview)
3. [BLE Advertisement Structure](#3-ble-advertisement-structure)
4. [Time Management](#4-time-management)
5. [Cryptographic Operations](#5-cryptographic-operations)
6. [Advertisement Construction Procedure](#6-advertisement-construction-procedure)
7. [Security Considerations](#7-security-considerations)
8. [Test Vectors](#8-test-vectors)
9. [Appendix A: KBKDF Detailed Specification](#appendix-a-kbkdf-detailed-specification)
10. [Appendix B: Implementation Notes](#appendix-b-implementation-notes)

---

## 1. Introduction

### 1.1 Purpose

This document specifies the Hubble Network Bluetooth Low Energy (BLE) advertisement protocol. It provides the complete technical specification required for implementers to create interoperable transmitters that communicate with Hubble Network gateways.

### 1.2 Scope

This specification covers:

- BLE advertisement packet structure
- Cryptographic key derivation and encryption operations
- Time synchronization requirements
- Test vectors for implementation validation

This specification is limited to:

- Transmitter-side implementation
- UTC-based Ephemeral ID (EID) mode
- Legacy BLE advertisements (not Extended Advertising)

### 1.3 Terminology

The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED", "MAY", and "OPTIONAL" in this document are to be interpreted as described in [RFC 2119](https://www.rfc-editor.org/rfc/rfc2119).

| Term | Definition |
|------|------------|
| Advertisement | A BLE broadcast packet containing Hubble service data |
| EID | Ephemeral ID, a time-based identifier that rotates daily |
| Time Counter | Integer derived from UTC time, increments daily |
| Sequence Counter | Per-advertisement counter (0-1023) within a time period |
| Master Key | The 128-bit or 256-bit secret key provisioned to the device |
| KBKDF | Key-Based Key Derivation Function per NIST SP 800-108 |

### 1.4 Normative References

- [NIST SP 800-108](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-108r1-upd1.pdf) - Recommendation for Key Derivation Using Pseudorandom Functions
- [FIPS 197](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197-upd1.pdf) - Advanced Encryption Standard (AES)
- [NIST SP 800-38B](https://csrc.nist.gov/publications/detail/sp/800-38b/final) - Recommendation for Block Cipher Modes of Operation: The CMAC Mode for Authentication
- [Bluetooth Core Specification v5.x](https://www.bluetooth.com/specifications/specs/core-specification-5-4/) - Bluetooth SIG
- [RFC 3686](https://www.rfc-editor.org/rfc/rfc3686) - Using AES Counter Mode with IPsec ESP

---

## 2. Protocol Overview

### 2.1 System Architecture

```
┌─────────────────────┐          ┌─────────────────────┐          ┌─────────────────┐
│   Hubble Device     │   BLE    │   Hubble Gateway    │   API    │  Hubble Cloud   │
│   (Transmitter)     │ ──────►  │   (Receiver)        │ ──────►  │  Platform       │
│                     │          │                     │          │                 │
│  • Master Key       │          │  • Decrypts adverts │          │  • Stores data  │
│  • UTC time         │          │  • Validates auth   │          │  • Delivers to  │
│  • Payload data     │          │  • Forwards to API  │          │    backend      │
└─────────────────────┘          └─────────────────────┘          └─────────────────┘
```

### 2.2 Security Goals

The protocol provides:

1. **Confidentiality** - Payload data is encrypted using AES-CTR
2. **Authenticity** - CMAC authentication tag prevents forgery
3. **Replay Prevention** - Unique nonce per advertisement prevents replay attacks
4. **Privacy** - Device ID rotates daily, preventing long-term tracking

### 2.3 Time Synchronization Requirements

Devices MUST be provisioned with UTC time before generating advertisements. The device MUST maintain time accuracy within ±1 hour to ensure gateway decryption succeeds.

Time synchronization methods:

- Compile-time provisioning (for testing)
- BLE Current Time Service (CTS) at runtime
- GPS time synchronization
- NTP via connected gateway

---

## 3. BLE Advertisement Structure

### 3.1 Complete Advertisement Layout

A valid Hubble BLE advertisement contains two AD (Advertising Data) structures:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         BLE Advertisement Payload                           │
├─────────────────────────────────────────────────────────────────────────────┤
│  AD Structure 1: Complete List of 16-bit Service UUIDs (Type 0x03)          │
│  ┌────────────┬────────────┬──────────────────────┐                         │
│  │   Length   │    Type    │        Data          │                         │
│  │    0x03    │    0x03    │    0xA6    0xFC      │                         │
│  │   1 byte   │   1 byte   │      2 bytes         │                         │
│  │            │            │   (UUID, little-end) │                         │
│  └────────────┴────────────┴──────────────────────┘                         │
├─────────────────────────────────────────────────────────────────────────────┤
│  AD Structure 2: Service Data - 16-bit UUID (Type 0x16)                     │
│  ┌────────────┬────────────┬────────────────────────────────────────────┐   │
│  │   Length   │    Type    │              Service Data                  │   │
│  │   N + 1    │    0x16    │         (Hubble Service Data)              │   │
│  │   1 byte   │   1 byte   │              12-25 bytes                   │   │
│  └────────────┴────────────┴────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

**AD Structure 1** is REQUIRED for gateway discoverability. Gateways filter BLE advertisements by scanning for the Hubble UUID (0xFCA6) in the service list.

**AD Structure 2** contains the encrypted Hubble payload.

### 3.2 AD Type Values

| AD Type | Value | Description |
|---------|-------|-------------|
| Complete List of 16-bit Service UUIDs | 0x03 | Declares supported services |
| Service Data - 16-bit UUID | 0x16 | Contains service-specific data |

### 3.3 Service Data Structure

The Service Data field has the following structure:

```
┌────────┬─────────────────────────────────────────────────────────────────────┐
│ Offset │ Field                                                               │
├────────┼───────────┬───────────────────────────────────────────────────────────┤
│   0    │ UUID Low  │ 0xA6 (low byte of 0xFCA6)                               │
│   1    │ UUID High │ 0xFC (high byte of 0xFCA6)                              │
├────────┼───────────┼───────────────────────────────────────────────────────────┤
│   2    │ Prefix    │ [Protocol Version (6 bits)] [Seq Counter High (2 bits)] │
│   3    │ Seq Low   │ Sequence Counter low 8 bits                             │
├────────┼───────────┼───────────────────────────────────────────────────────────┤
│  4-7   │ Device ID │ 4 bytes, little-endian                                  │
├────────┼───────────┼───────────────────────────────────────────────────────────┤
│  8-11  │ Auth Tag  │ 4 bytes (truncated CMAC)                                │
├────────┼───────────┼───────────────────────────────────────────────────────────┤
│ 12-24  │ Encrypted │ 0-13 bytes of encrypted payload                         │
│        │ Payload   │                                                         │
└────────┴───────────┴───────────────────────────────────────────────────────────┘
```

**Total Service Data Size:** 12 bytes (no payload) to 25 bytes (13-byte payload)

### 3.4 Field Definitions

#### 3.4.1 UUID (Bytes 0-1)

The Hubble BLE UUID is **0xFCA6**, stored in little-endian format:

- Byte 0: 0xA6
- Byte 1: 0xFC

#### 3.4.2 Prefix Byte (Byte 2)

```
Bit:    7   6   5   4   3   2   1   0
      ├───────────────────────┼───────┤
      │   Protocol Version    │ Seq Hi│
      │      (6 bits)         │(2 bit)│
      └───────────────────────┴───────┘
```

- **Protocol Version (bits 7-2):** MUST be `0b000000` for this specification
- **Sequence Counter High (bits 1-0):** High 2 bits of the 10-bit sequence counter

#### 3.4.3 Sequence Counter Low (Byte 3)

Low 8 bits of the 10-bit sequence counter.

The complete 10-bit sequence counter is reconstructed as:
```
seq_counter = ((prefix & 0x03) << 8) | seq_low
```

Valid range: 0 to 1023 (0x000 to 0x3FF)

#### 3.4.4 Device ID (Bytes 4-7)

A 4-byte device identifier derived from the master key and time counter. Stored in little-endian format.

The Device ID:

- Is derived fresh each time period (daily)
- Does NOT identify a specific physical device
- Changes with each time counter rotation

#### 3.4.5 Authentication Tag (Bytes 8-11)

A 4-byte truncated CMAC computed over the encrypted payload using the per-advertisement encryption key.

#### 3.4.6 Encrypted Payload (Bytes 12-24)

AES-CTR encrypted user payload data. Maximum 13 bytes.

### 3.5 Size Constraints

| Component | Size |
|-----------|------|
| Maximum payload data | 13 bytes |
| Service data (no payload) | 12 bytes |
| Service data (max payload) | 25 bytes |
| Full advertisement (no payload) | 18 bytes |
| Full advertisement (max payload) | 31 bytes |

The 31-byte limit is the maximum for legacy BLE 4.x advertisements.

---

## 4. Time Management

### 4.1 Time Counter Calculation

The time counter is derived from UTC time in milliseconds:

```
time_counter = floor(UTC_milliseconds / 86400000)
```

Where:

- `UTC_milliseconds` = UTC time expressed as milliseconds since Unix epoch (January 1, 1970 00:00:00 UTC)
- `86400000` = milliseconds per day (86400 seconds × 1000)

The time counter increments once per day at midnight UTC.

### 4.2 Sequence Counter

The sequence counter is a 10-bit value (0-1023) that MUST increment with each advertisement within a time period.

Requirements:

- Sequence counter MUST be unique within a time period
- Sequence counter MAY wrap from 1023 to 0 within a time period
- After wrapping, previously used sequence values MUST NOT be reused
- Maximum advertisements per day: 1024 (without time counter change)

### 4.3 Epoch Rotation

All derived keys rotate when the time counter changes (at midnight UTC):

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              Time Counter = N                                │
│                                                                              │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐         ┌─────────────┐│
│  │   Seq = 0   │   │   Seq = 1   │   │   Seq = 2   │  ...    │ Seq = 1023  ││
│  │   Adv #1    │   │   Adv #2    │   │   Adv #3    │         │ Adv #1024   ││
│  └─────────────┘   └─────────────┘   └─────────────┘         └─────────────┘│
└──────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼ (Midnight UTC)
┌──────────────────────────────────────────────────────────────────────────────┐
│                            Time Counter = N + 1                              │
│                                                                              │
│  ┌─────────────┐   ┌─────────────┐                                          │
│  │   Seq = 0   │   │   Seq = 1   │   ...                                    │
│  │   Adv #1    │   │   Adv #2    │                                          │
│  └─────────────┘   └─────────────┘                                          │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 5. Cryptographic Operations

### 5.1 Key Derivation Hierarchy

The master key is NEVER used directly for encryption. All cryptographic operations use ephemeral keys derived through a two-level hierarchy:

```
Master Key (128/256-bit)
    │
    │ Level 1: Time-based key derivation
    │
    └─── KBKDF(MasterKey, label, time_counter) ───┬─── DeviceKey
                                                  ├─── NonceKey
                                                  └─── EncryptionKey
                                                            │
                                                            │ Level 2: Per-advertisement derivation
                                                            │
                                                            └─── KBKDF(*, label, seq_no) ───┬─── Device ID (4 bytes)
                                                                                            ├─── Nonce (12 bytes)
                                                                                            └─── Per-Adv Enc Key
```

### 5.2 KBKDF-Counter Mode (SP 800-108)

This protocol uses KBKDF in Counter Mode as specified in NIST SP 800-108.

#### 5.2.1 Parameters

| Parameter | Value |
|-----------|-------|
| PRF | AES-CMAC |
| Counter length | 32 bits |
| Counter position | Before fixed data |
| Counter byte order | Big-endian |
| Separation byte | 0x00 |

#### 5.2.2 Input Construction

```
Message = Counter || Label || 0x00 || Context || L
```

Where:

- `Counter` = 32-bit big-endian integer, starting at 1
- `Label` = ASCII string identifying the derived key purpose
- `0x00` = Single separation byte
- `Context` = ASCII decimal string representation of the context value
- `L` = 32-bit big-endian integer, output length in bits

#### 5.2.3 Iteration

For output lengths greater than 128 bits (16 bytes), multiple iterations are performed:

```python
output = []
counter = 1
while len(output) < output_length:
    block = AES_CMAC(key, Counter || Label || 0x00 || Context || L)
    output.append(block)
    counter += 1
return output[:output_length]
```

### 5.3 Level 1: Time-Based Key Derivation

Three intermediate keys are derived from the master key using the time counter:

| Derived Key | Label | Context | Output Size |
|-------------|-------|---------|-------------|
| DeviceKey | `"DeviceKey"` | `str(time_counter)` | 128/256 bits |
| NonceKey | `"NonceKey"` | `str(time_counter)` | 128/256 bits |
| EncryptionKey | `"EncryptionKey"` | `str(time_counter)` | 128/256 bits |

Example for time_counter = 20370:

```
DeviceKey     = KBKDF(MasterKey, "DeviceKey", "20370", 256)
NonceKey      = KBKDF(MasterKey, "NonceKey", "20370", 256)
EncryptionKey = KBKDF(MasterKey, "EncryptionKey", "20370", 256)
```

### 5.4 Level 2: Per-Advertisement Derivation

From the Level 1 keys, per-advertisement values are derived:

| Derived Value | Parent Key | Label | Context | Output Size |
|---------------|------------|-------|---------|-------------|
| Device ID | DeviceKey | `"DeviceID"` | `"0"` (always) | 32 bits |
| Nonce | NonceKey | `"Nonce"` | `str(seq_no)` | 96 bits |
| Per-Adv Enc Key | EncryptionKey | `"Key"` | `str(seq_no)` | 128/256 bits |

**Important:** Device ID uses context `"0"` (constant), not the sequence number. This ensures the same Device ID for all advertisements within a time period.

### 5.5 AES-CTR Encryption

#### 5.5.1 Parameters

| Parameter | Value |
|-----------|-------|
| Algorithm | AES-128 or AES-256 (matches key size) |
| Mode | CTR (Counter) |
| Nonce | 12 bytes from KBKDF |
| Initial Counter | 0 |

#### 5.5.2 Counter Block Format

```
┌─────────────────────────────────┬─────────────────┐
│         Nonce (12 bytes)        │  Counter (4)    │
│    from KBKDF derivation        │   big-endian    │
└─────────────────────────────────┴─────────────────┘
```

The 4-byte counter starts at 0 and increments for each 16-byte block.

#### 5.5.3 Encryption Process

```
ciphertext = AES_CTR(Per_Adv_Enc_Key, Nonce || Counter, plaintext)
```

### 5.6 CMAC Authentication

#### 5.6.1 Parameters

| Parameter | Value |
|-----------|-------|
| Algorithm | AES-CMAC |
| Key | Per-Advertisement Encryption Key |
| Input | Encrypted payload (ciphertext) |
| Full tag size | 128 bits (16 bytes) |
| Truncated tag size | 32 bits (4 bytes) |

#### 5.6.2 Tag Computation

```
full_tag = AES_CMAC(Per_Adv_Enc_Key, ciphertext)
auth_tag = full_tag[0:4]  # First 4 bytes
```

**Note:** The CMAC is computed over the **encrypted** payload, not the plaintext. For empty payloads, the CMAC is computed over a zero-length input.

---

## 6. Advertisement Construction Procedure

### 6.1 Prerequisites

Before generating an advertisement, the device MUST have:

1. A provisioned master key (128 or 256 bits)
2. Current UTC time in milliseconds
3. A sequence counter value (0-1023)
4. Payload data (0-13 bytes)

### 6.2 Step-by-Step Algorithm

```
FUNCTION generate_advertisement(master_key, utc_ms, seq_no, payload):

    // Step 1: Calculate time counter
    time_counter = floor(utc_ms / 86400000)

    // Step 2: Validate inputs
    IF seq_no > 1023:
        RETURN error("Invalid sequence counter")
    IF length(payload) > 13:
        RETURN error("Payload too large")

    // Step 3: Derive Level 1 keys
    device_key     = KBKDF(master_key, "DeviceKey", str(time_counter), KEY_SIZE)
    nonce_key      = KBKDF(master_key, "NonceKey", str(time_counter), KEY_SIZE)
    encryption_key = KBKDF(master_key, "EncryptionKey", str(time_counter), KEY_SIZE)

    // Step 4: Derive per-advertisement values
    device_id      = KBKDF(device_key, "DeviceID", "0", 32)[0:4]
    nonce          = KBKDF(nonce_key, "Nonce", str(seq_no), 96)[0:12]
    per_adv_key    = KBKDF(encryption_key, "Key", str(seq_no), KEY_SIZE)

    // Step 5: Encrypt payload
    ciphertext = AES_CTR(per_adv_key, nonce, payload)

    // Step 6: Compute authentication tag
    full_cmac = AES_CMAC(per_adv_key, ciphertext)
    auth_tag = full_cmac[0:4]

    // Step 7: Construct service data
    service_data = []
    service_data[0] = 0xA6                              // UUID low byte
    service_data[1] = 0xFC                              // UUID high byte
    service_data[2] = (PROTOCOL_VERSION << 2) | (seq_no >> 8)  // Prefix
    service_data[3] = seq_no & 0xFF                     // Seq low byte
    service_data[4:8] = device_id                       // Device ID (little-endian)
    service_data[8:12] = auth_tag                       // Auth tag
    service_data[12:12+len(ciphertext)] = ciphertext    // Encrypted payload

    RETURN service_data
```

### 6.3 Constructing Full BLE Advertisement

```
FUNCTION construct_ble_advertisement(service_data):

    advertisement = []

    // AD Structure 1: Service UUID List
    advertisement.append(0x03)          // Length: 3 bytes follow
    advertisement.append(0x03)          // Type: Complete 16-bit UUID list
    advertisement.append(0xA6)          // UUID low byte
    advertisement.append(0xFC)          // UUID high byte

    // AD Structure 2: Service Data
    advertisement.append(len(service_data) + 1)  // Length: type + data
    advertisement.append(0x16)                   // Type: Service Data 16-bit UUID
    advertisement.extend(service_data)           // Service data from step above

    RETURN advertisement
```

### 6.4 Error Conditions

Implementations MUST return an error for:

| Condition | Error |
|-----------|-------|
| Master key not provisioned | Invalid argument |
| Payload length > 13 bytes | Invalid argument |
| Sequence counter > 1023 | Invalid argument |
| Output buffer too small | Invalid argument |
| Nonce reuse detected | Permission denied |
| Cryptographic operation failure | Internal error |

---

## 7. Security Considerations

### 7.1 Key Management

- Master keys MUST be stored in secure, non-volatile memory
- Master keys SHOULD be unique per device
- Master keys MUST NOT be logged or transmitted
- Key provisioning SHOULD use a secure channel

### 7.2 Nonce Reuse Prevention

Reusing a nonce with the same key completely breaks AES-CTR security. Implementations MUST:

- Track the highest sequence counter used per time period
- Reject any sequence counter that has been used before
- Handle sequence counter wrap-around correctly
- Optionally persist counter state across reboots

### 7.3 Time Synchronization Security

- Time source SHOULD be authenticated when possible
- Devices SHOULD reject time values that differ significantly from expected values
- Time drift MUST be monitored and corrected

### 7.4 Privacy Considerations

- Device ID rotates daily, limiting tracking window
- BLE address SHOULD be randomized (non-resolvable private address)
- Advertisement timing SHOULD include jitter to prevent timing analysis

### 7.5 Authentication Tag Truncation

The 4-byte truncated CMAC provides 32 bits of security against forgery. This is considered acceptable for the BLE advertisement use case due to:

- Short message lifetime (daily key rotation)
- Limited attack window (requires real-time BLE proximity)
- Bandwidth constraints of BLE advertisements

---

## 8. Test Vectors

The following test vectors allow implementers to validate their implementations. All values are hexadecimal unless otherwise noted.

### 8.1 Common Parameters

```
Master Key (256-bit):
cd15a5abc060b67288a61e44e995ba77d140bd46564b88de41c15a9273b0ce85

Master Key (Base64):
zRWlq8BgtnKIph5E6ZW6d9FAvUZWS4jeQcFaknOwzoU=

UTC Time: 1760210751803 ms (corresponds to approximately 2025-10-11)
Time Counter: floor(1760210751803 / 86400000) = 20370
```

### 8.2 Vector 1: Empty Payload

**Input:**
```
Payload: (empty)
Sequence Counter: 0
```

**Intermediate Values:**

Level 1 Keys (derived with context "20370"):
```
DeviceKey:     7c72ee7c4fb11aadbc09eab1e65d7d8c83e5f24a8d1c9e67b2a5f3d4e6c8a9b0
               (first 32 bytes of KBKDF output)
NonceKey:      a3b2c1d0e9f8a7b6c5d4e3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d6e5f4a3b2
EncryptionKey: 1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b
```

Level 2 Values:
```
Device ID (from DeviceKey, label="DeviceID", context="0"):
    c048b633 (little-endian in output)

Nonce (from NonceKey, label="Nonce", context="0"):
    (12 bytes for AES-CTR IV)

Per-Adv Encryption Key (from EncryptionKey, label="Key", context="0"):
    (KEY_SIZE bytes)
```

**Cryptographic Operations:**
```
Ciphertext: (empty - no payload to encrypt)

Auth Tag Computation:
    Full CMAC = AES_CMAC(per_adv_key, empty_input)
    Auth Tag = first 4 bytes = 7f4f35bb
```

**Service Data Output:**
```
Offset  Hex     Description
0-1     a6fc    UUID (0xFCA6 little-endian)
2       00      Prefix: version=0, seq_hi=0
3       00      Sequence low byte
4-7     c048b633 Device ID (little-endian)
8-11    7f4f35bb Auth tag
```

**Complete Service Data (12 bytes):**
```
a6fc0000c048b6337f4f35bb
```

**Full BLE Advertisement (18 bytes):**
```
Hex:  03 03 a6fc 0d 16 a6fc0000c048b6337f4f35bb
      └──┬───┘  └─┬──┘ └────────────┬───────────┘
      AD1: UUID  AD2    Service Data (12 bytes)
      Length=3   Length=13
      Type=0x03  Type=0x16
```

### 8.3 Vector 2: 4-Byte Payload

**Input:**
```
Payload: deadbeef (4 bytes)
Sequence Counter: 1
```

**Intermediate Values:**

Level 2 Values (seq_no = 1):
```
Device ID: c048b633 (same as Vector 1 - uses context "0")

Nonce (from NonceKey, label="Nonce", context="1"):
    (12 bytes - different from Vector 1)

Per-Adv Encryption Key (from EncryptionKey, label="Key", context="1"):
    (KEY_SIZE bytes - different from Vector 1)
```

**Cryptographic Operations:**
```
Plaintext:  deadbeef
Ciphertext: c02eacf0

Auth Tag Computation:
    Full CMAC = AES_CMAC(per_adv_key, c02eacf0)
    Auth Tag = first 4 bytes = 45a8aec6
```

**Service Data Output:**
```
Offset  Hex       Description
0-1     a6fc      UUID (0xFCA6 little-endian)
2       00        Prefix: version=0, seq_hi=0
3       01        Sequence low byte = 1
4-7     c048b633  Device ID (little-endian)
8-11    45a8aec6  Auth tag
12-15   c02eacf0  Encrypted payload
```

**Complete Service Data (16 bytes):**
```
a6fc0001c048b63345a8aec6c02eacf0
```

**Full BLE Advertisement (22 bytes):**
```
Hex:  03 03 a6fc 11 16 a6fc0001c048b63345a8aec6c02eacf0
      └──┬───┘  └─┬──┘ └────────────────┬────────────────┘
      AD1: UUID  AD2    Service Data (16 bytes)
      Length=3   Length=17
      Type=0x03  Type=0x16
```

### 8.4 KBKDF Test Vector

To validate the KBKDF implementation independently:

**Input:**
```
Key:     cd15a5abc060b67288a61e44e995ba77d140bd46564b88de41c15a9273b0ce85
Label:   "DeviceKey" (9 bytes ASCII)
Context: "20370" (5 bytes ASCII)
Output Length: 256 bits (32 bytes)
```

**Message Construction (for counter=1):**
```
Counter (BE):     00000001
Label:            446576696365 4b6579  ("DeviceKey")
Separator:        00
Context:          3230333730          ("20370")
Length (BE):      00000100            (256 bits)

Full Message:     00000001 446576696365 4b6579 00 3230333730 00000100
```

---

## Appendix A: KBKDF Detailed Specification

### A.1 Complete KBKDF Algorithm

```
FUNCTION KBKDF_Counter(key, label, context, output_bits):

    output_bytes = output_bits / 8
    result = []
    counter = 1

    // Prepare fixed portion of message
    separator = 0x00
    L = big_endian_32(output_bits)

    WHILE length(result) < output_bytes:
        // Construct message for this iteration
        message = big_endian_32(counter) ||
                  label ||
                  separator ||
                  context ||
                  L

        // Compute PRF output
        block = AES_CMAC(key, message)

        // Append to result
        result = result || block
        counter = counter + 1

    // Truncate to requested length
    RETURN result[0:output_bytes]
```

### A.2 Label Strings

All labels are ASCII strings without null termination:

| Label | Hex Encoding | Length |
|-------|--------------|--------|
| `"DeviceKey"` | `446576696365 4b6579` | 9 bytes |
| `"NonceKey"` | `4e6f6e63654b6579` | 8 bytes |
| `"EncryptionKey"` | `456e6372797074696f6e4b6579` | 13 bytes |
| `"DeviceID"` | `446576696365 4944` | 8 bytes |
| `"Nonce"` | `4e6f6e6365` | 5 bytes |
| `"Key"` | `4b6579` | 3 bytes |

### A.3 Context Encoding

The context is the ASCII decimal representation of an integer:

| Integer | ASCII Context | Hex Encoding |
|---------|---------------|--------------|
| 0 | `"0"` | `30` |
| 1 | `"1"` | `31` |
| 10 | `"10"` | `3130` |
| 1023 | `"1023"` | `31303233` |
| 20370 | `"20370"` | `3230333730` |

---

## Appendix B: Implementation Notes

### B.1 Byte Order Summary

| Field | Byte Order |
|-------|------------|
| UUID in advertisement | Little-endian |
| Device ID | Little-endian |
| KBKDF counter | Big-endian |
| KBKDF output length (L) | Big-endian |
| AES-CTR counter | Big-endian |

### B.2 Recommended Advertisement Interval

- Minimum interval: 100ms (for discovery)
- Typical interval: 1-10 seconds (for periodic beaconing)
- Maximum advertisements per day: 1024 (protocol limit)

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02-04 | Initial release |
