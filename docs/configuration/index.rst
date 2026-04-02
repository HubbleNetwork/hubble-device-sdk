.. _hubble_configuration:

Configuration Reference
#######################

The Hubble Network SDK is configured entirely at compile time. There is no
runtime configuration API. All options are set before building and compiled
into the firmware binary.

The configuration mechanism depends on the platform:

- **Zephyr**: Options are set in ``prj.conf`` or board overlay files via the
  Kconfig system. All options are available.
- **ESP-IDF**: Options are set in ``sdkconfig.defaults`` or via
  ``idf.py menuconfig``. Most options are available.
- **FreeRTOS**: Options are set as C ``#define`` macros in a configuration
  header file. Point ``HUBBLENETWORK_SDK_CONFIG`` to your config file in the
  build system (defaults to ``port/freertos/config.h``).
- **Other Platforms** (bare metal, custom RTOS): Options are set as C ``#define``
  macros in a configuration header file or passed via compiler flags (``-D``).

For initial setup instructions, see :ref:`zephyr_quick_start` or
:ref:`freertos_quick_start`.


Network Selection
*****************

``CONFIG_HUBBLE_BLE_NETWORK``
   :Type: bool
   :Default: ``n`` (Zephyr, ESP-IDF) / ``1`` (FreeRTOS default config)
   :Platforms: Zephyr, ESP-IDF, FreeRTOS, Other Platforms

   Enable the BLE network module. Required for generating BLE advertisements
   via :c:func:`hubble_ble_advertise_get`.

``CONFIG_HUBBLE_SAT_NETWORK``
   :Type: bool
   :Default: ``n``
   :Platforms: Zephyr, ESP-IDF, FreeRTOS, Other Platforms

   Enable the satellite network module.

   .. note::

      Satellite functionality is experimental and not yet ready for production
      deployments.

   On Zephyr, enabling this option automatically selects ``EXPERIMENTAL`` and
   ``ENTROPY_GENERATOR``.

At least one network module must be enabled. Both can be enabled simultaneously.


EID Configuration
*****************

The Ephemeral ID (EID) determines how the SDK generates unique identifiers for
each advertisement epoch. Two mutually exclusive modes are available.

``CONFIG_HUBBLE_EID_TIME_BASED``
   :Type: bool (choice, default)
   :Default: selected

   EID is derived from Unix time. The application must provision time via
   :c:func:`hubble_init` or :c:func:`hubble_time_set` before generating
   advertisements. See :ref:`hubble_timing` for best practices.

   This is the default mode. Mutually exclusive with
   ``CONFIG_HUBBLE_EID_COUNTER_BASED``.

``CONFIG_HUBBLE_EID_COUNTER_BASED``
   :Type: bool (choice)
   :Default: not selected

   EID is derived from an internal counter. No Unix time provisioning is
   required. The initial counter value is provided via :c:func:`hubble_init`.

   Mutually exclusive with ``CONFIG_HUBBLE_EID_TIME_BASED``.

   On FreeRTOS and other platforms, define ``CONFIG_HUBBLE_EID_COUNTER_BASED``
   in the config header. Do not define both modes simultaneously (the build
   will error).

``CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC``
   :Type: int
   :Default: ``86400``
   :Range: ``86400`` -- ``86400``

   EID rotation period in seconds. Currently locked to daily rotation
   (86400 seconds).

``CONFIG_HUBBLE_EID_POOL_SIZE``
   :Type: int
   :Default: ``128``
   :Range: ``16`` -- ``1024``
   :Depends on: ``CONFIG_HUBBLE_EID_COUNTER_BASED``

   Number of unique EIDs in the pool for counter-based mode. The counter wraps
   at this value. Only relevant when ``CONFIG_HUBBLE_EID_COUNTER_BASED`` is
   enabled.

   For the range of allowable EID pool sizes see the backend registration API
   `here <https://hubble.com/docs/api-specification/register-new-devices#optional-set-pool-size-for-eid-rotation>`__.


Security and Key Configuration
*******************************

``CONFIG_HUBBLE_NETWORK_KEY_256``
   :Type: bool (choice)
   :Default: selected (first in choice group)
   :Platforms: Zephyr, ESP-IDF

   Use 256-bit (32-byte) encryption keys.

   On FreeRTOS and other platforms, set ``CONFIG_HUBBLE_KEY_SIZE`` to ``32``
   instead.

``CONFIG_HUBBLE_NETWORK_KEY_128``
   :Type: bool (choice)
   :Default: not selected
   :Platforms: Zephyr, ESP-IDF

   Use 128-bit (16-byte) encryption keys.

   On FreeRTOS and other platforms, set ``CONFIG_HUBBLE_KEY_SIZE`` to ``16``
   instead.

``CONFIG_HUBBLE_KEY_SIZE``
   :Type: int
   :Default: ``32`` if ``CONFIG_HUBBLE_NETWORK_KEY_256``, ``16`` if ``CONFIG_HUBBLE_NETWORK_KEY_128``
   :Platforms: FreeRTOS and other platforms (set directly), Zephyr/ESP-IDF (derived automatically)

   Encryption key size in bytes. On Zephyr and ESP-IDF this is derived
   from the key size choice above and should not be set directly. On FreeRTOS
   and other platforms, set this to ``16`` (128-bit) or ``32`` (256-bit).

   .. note::

      The FreeRTOS default config (``port/freertos/config.h``) defaults to
      ``16`` (128-bit), while Zephyr defaults to ``32`` (256-bit) since
      ``CONFIG_HUBBLE_NETWORK_KEY_256`` is listed first in the choice group.

``CONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK``
   :Type: bool
   :Default: ``y``
   :Depends on: ``CONFIG_HUBBLE_BLE_NETWORK`` or ``CONFIG_HUBBLE_SAT_NETWORK``
   :Platforms: Zephyr, ESP-IDF

   Enforce that nonce parameters are not reused to encrypt different messages.
   When enabled, the SDK persists the nonce counter to non-volatile storage
   (NVS) and checks for reuse on each encryption operation.

   .. warning::

      Disabling this option has severe security implications. Reusing a nonce
      with the same encryption key enables replay attacks and may compromise
      the confidentiality of encrypted data.

   See :ref:`hubble_ble_security` for details on nonce generation.


Crypto Backend
**************

The SDK requires a cryptographic backend for AES-CTR encryption and CMAC
authentication. Three mutually exclusive options are available on Zephyr.

``CONFIG_HUBBLE_NETWORK_CRYPTO_PSA``
   :Type: bool (choice, default)
   :Default: selected
   :Platforms: Zephyr

   Use the PSA Crypto API for cryptographic operations. This is the
   recommended backend for Zephyr platforms. Automatically selects the
   required PSA algorithm capabilities (``PSA_WANT_KEY_TYPE_AES``,
   ``PSA_WANT_ALG_CMAC``, ``PSA_WANT_ALG_CTR``).

``CONFIG_HUBBLE_NETWORK_CRYPTO_MBEDTLS``
   :Type: bool (choice)
   :Default: not selected
   :Depends on: ``!NRF_SECURITY``
   :Platforms: Zephyr

   Use MbedTLS directly for cryptographic operations. Not available when
   Nordic's ``NRF_SECURITY`` module is enabled. Automatically selects
   ``MBEDTLS``, ``MBEDTLS_CMAC``, ``MBEDTLS_CIPHER``, and related options.

``CONFIG_HUBBLE_NETWORK_CRYPTO_CUSTOM``
   :Type: bool (choice)
   :Default: not selected
   :Platforms: Zephyr

   Use a custom cryptographic implementation. The application must implement:

   - :c:func:`hubble_crypto_init`
   - :c:func:`hubble_crypto_cmac`
   - :c:func:`hubble_crypto_aes_ctr`
   - :c:func:`hubble_crypto_zeroize`

.. note::

   **ESP-IDF** handles crypto through its own component system and does not
   expose the crypto backend choice via Kconfig.

   **FreeRTOS and other platforms** have no standard crypto API. The
   application must always implement the crypto interface functions listed
   above, or enable PSA support by defining
   ``CONFIG_HUBBLE_NETWORK_CRYPTO_PSA`` in the config header.
   See :ref:`freertos_quick_start` for details.


Custom Overrides
****************

The SDK provides weak function implementations that applications can override
for testing or custom behavior.

``CONFIG_HUBBLE_NETWORK_SEQUENCE_NONCE_CUSTOM``
   :Type: bool
   :Default: ``n``
   :Depends on: ``CONFIG_HUBBLE_BLE_NETWORK`` or ``CONFIG_HUBBLE_SAT_NETWORK``
   :Platforms: Zephyr, ESP-IDF

   Allow the application to override ``hubble_sequence_counter_get()`` with a
   custom implementation. Useful for deterministic testing where specific nonce
   values are needed.

``CONFIG_HUBBLE_UPTIME_CUSTOM``
   :Type: bool
   :Default: ``n``
   :Depends on: ``CONFIG_HUBBLE_BLE_NETWORK`` or ``CONFIG_HUBBLE_SAT_NETWORK``
   :Platforms: Zephyr

   Allow the application to override ``hubble_uptime_get()`` with a custom
   implementation. Primarily useful for unit testing where controlled time
   values are needed to verify time-dependent behavior.

.. note::

   On FreeRTOS and other platforms, both ``hubble_sequence_counter_get()`` and
   ``hubble_uptime_get()`` are declared as weak functions and can be overridden
   directly without a configuration option.


Satellite Configuration
***********************

These options are only available when ``CONFIG_HUBBLE_SAT_NETWORK`` is enabled.

.. note::

   Satellite functionality is experimental and not yet included in stable
   releases. See :ref:`hubble_introduction` for the current status.

``CONFIG_HUBBLE_SAT_NETWORK_SMALL``
   :Type: bool
   :Default: ``n``
   :Depends on: ``CONFIG_HUBBLE_SAT_NETWORK``
   :Platforms: Zephyr, ESP-IDF, FreeRTOS, Other Platforms

   Use polynomial approximation for trigonometric functions in satellite
   pass (fly-by) calculations. Reduces code size at the cost of some
   numerical precision.

``CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR``
   :Type: int
   :Default: ``500``
   :Depends on: ``CONFIG_HUBBLE_SAT_NETWORK``
   :Platforms: Zephyr, ESP-IDF, FreeRTOS, Other Platforms

   Device time drift retry rate in parts per million (PPM). Adds additional
   transmission retries proportional to the time elapsed since the device
   last had its time synchronized.

``CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1``
   :Type: bool (choice, default)
   :Default: selected
   :Depends on: ``CONFIG_HUBBLE_SAT_NETWORK``
   :Platforms: Zephyr, ESP-IDF, FreeRTOS, Other Platforms

   Use V1 satellite protocol with channel hopping within transmissions.
   Currently the only protocol version available.


Logging
*******

``CONFIG_HUBBLE_LOG_LEVEL``
   :Type: int
   :Default: platform-dependent
   :Platforms: Zephyr

   On Zephyr, the SDK uses the standard Zephyr logging subsystem. Log levels
   follow the Zephyr convention:

   - ``0`` (``LOG_LEVEL_NONE``): Logging disabled
   - ``1`` (``LOG_LEVEL_ERR``): Errors only
   - ``2`` (``LOG_LEVEL_WRN``): Warnings and errors
   - ``3`` (``LOG_LEVEL_INF``): Informational, warnings, and errors
   - ``4`` (``LOG_LEVEL_DBG``): Debug and all above

   Set via ``CONFIG_HUBBLE_LOG_LEVEL_DBG=y``, ``CONFIG_HUBBLE_LOG_LEVEL_INF=y``,
   etc. in ``prj.conf``.

   On ESP-IDF, logging uses the ESP-IDF logging framework.

   On FreeRTOS and other platforms, ``hubble_log()`` is a weak function that
   can be overridden by the application to direct log output as needed.

   .. warning::

      Never log encryption keys or decrypted payloads in production builds.
      Use ``CONFIG_HUBBLE_LOG_LEVEL_NONE=y`` or equivalent to disable logging
      in production firmware.


Common Configuration Examples
******************************

BLE with 256-bit keys (recommended)
====================================

Zephyr
^^^^^^

.. code-block::

   CONFIG_HUBBLE_BLE_NETWORK=y
   CONFIG_HUBBLE_NETWORK_KEY_256=y
   CONFIG_BT=y
   CONFIG_BT_PERIPHERAL=y

FreeRTOS / Other Platforms
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   #define CONFIG_HUBBLE_BLE_NETWORK  1
   #define CONFIG_HUBBLE_KEY_SIZE     32

BLE with counter-based EID (no UTC required)
=============================================

Zephyr
^^^^^^

.. code-block::

   CONFIG_HUBBLE_BLE_NETWORK=y
   CONFIG_HUBBLE_NETWORK_KEY_256=y
   CONFIG_HUBBLE_EID_COUNTER_BASED=y
   CONFIG_HUBBLE_EID_POOL_SIZE=64
   CONFIG_BT=y
   CONFIG_BT_PERIPHERAL=y

FreeRTOS / Other Platforms
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   #define CONFIG_HUBBLE_BLE_NETWORK       1
   #define CONFIG_HUBBLE_KEY_SIZE           32
   #define CONFIG_HUBBLE_EID_COUNTER_BASED  1
   #define CONFIG_HUBBLE_EID_POOL_SIZE      64

BLE with MbedTLS crypto backend
================================

.. code-block::

   CONFIG_HUBBLE_BLE_NETWORK=y
   CONFIG_HUBBLE_NETWORK_KEY_256=y
   CONFIG_HUBBLE_NETWORK_CRYPTO_MBEDTLS=y
   CONFIG_BT=y
   CONFIG_BT_PERIPHERAL=y

This example is Zephyr-only. On FreeRTOS and other platforms, the application
always provides its own crypto implementation.


Platform Availability Summary
******************************

.. list-table::
   :widths: 35 13 13 13 13
   :header-rows: 1

   * - Option
     - Zephyr
     - ESP-IDF
     - FreeRTOS
     - Other [5]_
   * - ``CONFIG_HUBBLE_BLE_NETWORK``
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_SAT_NETWORK``
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_EID_TIME_BASED``
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_EID_COUNTER_BASED``
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC``
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_EID_POOL_SIZE``
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_NETWORK_KEY_256``
     - Yes
     - Yes
     - No [1]_
     - No [1]_
   * - ``CONFIG_HUBBLE_NETWORK_KEY_128``
     - Yes
     - Yes
     - No [1]_
     - No [1]_
   * - ``CONFIG_HUBBLE_KEY_SIZE``
     - Derived
     - Derived
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK``
     - Yes
     - Yes
     - No
     - No
   * - ``CONFIG_HUBBLE_NETWORK_CRYPTO_PSA``
     - Yes
     - No [2]_
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_NETWORK_CRYPTO_MBEDTLS``
     - Yes
     - No [2]_
     - No
     - No
   * - ``CONFIG_HUBBLE_NETWORK_CRYPTO_CUSTOM``
     - Yes
     - No [2]_
     - No
     - No
   * - ``CONFIG_HUBBLE_NETWORK_SEQUENCE_NONCE_CUSTOM``
     - Yes
     - Yes
     - No [3]_
     - No [3]_
   * - ``CONFIG_HUBBLE_UPTIME_CUSTOM``
     - Yes
     - No
     - No [3]_
     - No [3]_
   * - ``CONFIG_HUBBLE_SAT_NETWORK_SMALL``
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR``
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1``
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``CONFIG_HUBBLE_LOG_LEVEL``
     - Yes
     - No [4]_
     - No [3]_
     - No [3]_

.. [1] FreeRTOS and other platforms use ``CONFIG_HUBBLE_KEY_SIZE`` directly
   (set to ``16`` or ``32``) instead of the Kconfig choice group.
.. [2] ESP-IDF handles crypto through its own component system.
.. [3] Weak function overrides are available directly without a configuration
   option.
.. [4] ESP-IDF uses its own logging framework.
.. [5] Other platforms (bare metal, custom RTOS). Define configuration macros
   in a custom header file or via compiler flags (``-D``).
