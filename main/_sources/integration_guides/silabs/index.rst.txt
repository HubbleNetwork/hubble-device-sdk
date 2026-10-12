.. _silabs_freertos_integration_guide:

SiLabs (FreeRTOS) Integration Guide
###################################

This guide walks through integrating the Hubble Network dual-stack Satellite
and BLE application with the `Silicon Labs Simplicity SDK`_ on FreeRTOS.

By the end of this guide you will know how to:

* Integrate the Hubble Satellite and Terrestrial (BLE) network stacks into a
  Simplicity SDK application.
* Obtain ephemeris data and use pass prediction to schedule satellite
  transmissions.
* Transmit data to the Hubble satellite network from a Silicon Labs device.

.. _silabs_freertos_supported_devices:

Supported Devices and SDK Version
**********************************

The Hubble Device SDK currently supports **Simplicity SDK 2026.6.1**, with
`Simplicity Studio 6`_ or the command line (SLC CLI). If you require support
for a different version, `contact us <mailto:support@hubble.com>`_.

The following Silicon Labs device families are supported:

.. list-table::
   :widths: 20 80
   :header-rows: 1

   * - Family
     - Notes
   * - EFR32xG24
     - Choose a part with the +20 dBm integrated PA (e.g. EFR32MG24B220, used
       on BRD4187C); otherwise an external amplifier is required to reach
       20 dBm
   * - EFR32xG26
     - Choose a part with the +20 dBm integrated PA (e.g. EFR32MG26B420, used
       on BRD4121A); otherwise an external amplifier is required to reach
       20 dBm


.. include:: ../common/prerequisites.rst
   :start-after: hubble-integration-prerequisites

.. include:: ../common/account-setup.rst
   :start-after: hubble-integration-account-setup


SDK Setup
*********

Clone the Hubble Device SDK:

.. code-block:: bash

   git clone https://github.com/HubbleNetwork/hubble-device-sdk.git

Installing the Tools and the Extension
======================================

The Hubble Device SDK ships as a Simplicity SDK extension.

.. tabs::

   .. tab:: Simplicity Studio 6

      #. Install `Simplicity Studio 6`_ with the Simplicity SDK.
      #. Go to **Settings** → **SDKs** and select the Simplicity SDK version
         you want to use.
      #. Click **Add Extension**, then **Browse** to the directory where you
         cloned the Hubble Device SDK, and click **Finish**.
      #. In the **Verify SDK Extensions** dialog, click **Trust**.

   .. tab:: Terminal

      Install the Simplicity SDK and the tools with the
      `Silicon Labs Tool (SLT)`_:

      .. code-block:: bash

         export SLT=/path/to/slt
         $SLT install simplicity-sdk slc-cli gcc-arm-none-eabi commander

      SLC discovers extensions under the Simplicity SDK's ``extension``
      directory. Set the paths, then link the Hubble Device SDK there and
      register it as trusted:

      .. code-block:: bash

         export HUBBLE_SDK=/path/to/hubble-device-sdk
         export SISDK=/path/to/simplicity_sdk
         export SLC=/path/to/slc_cli/slc
         export COMMANDER=/path/to/commander

         mkdir -p $SISDK/extension
         ln -s $HUBBLE_SDK $SISDK/extension/hubble-device-sdk
         $SLC signature trust -extpath $SISDK/extension/hubble-device-sdk

.. tip::

   ``$SLT where <package>`` prints where a package was installed. See
   `Useful paths`_ for the default install locations of Simplicity Studio
   and SLT.


.. _silabs_freertos_sat_project_config:

Project Configuration
*********************

Enable the Hubble dual-stack functionality by adding the Hubble components to
your project's ``.slcp`` file and configuring them.

.. tabs::

   .. tab:: Simplicity Studio 6

      #. Open your project's ``.slcp`` file to launch the Project
         Configurator.

         .. image:: ../../quickstart/silabs/img/QuickStart_Silabs_OpenProject.webp
            :alt: Opening the project's .slcp file in Simplicity Studio
            :align: center
            :class: step-image

      #. Go to **Software Components** and search for **Hubble**.

         .. image:: ../../quickstart/silabs/img/QuickStart_Silabs_SoftwareComponents.webp
            :alt: Searching for Hubble in Software Components
            :align: center
            :class: step-image

      #. Install **Hubble Device SDK Terrestrial (BLE)** (under **Hubble** →
         **BLE**) and **Hubble Device SDK Satellite** (under **Hubble** →
         **Satellite**).

         .. image:: ../../quickstart/silabs/img/QuickStart_Silabs_Install.webp
            :alt: Installing the Hubble Device SDK components
            :align: center
            :class: step-image

      #. Select **Hubble Device SDK Common** (under **Hubble**), click
         **Configure**, and set:

         - **Hubble Network key size** to match your device key.
         - **Counter source** to **Unix time**.

         .. image:: ../../quickstart/silabs/img/QuickStart_Silabs_Configure.webp
            :alt: Configuring the Hubble Device SDK components
            :align: center
            :class: step-image

      #. Select **Hubble Device SDK Satellite**, click **Configure**, and set
         **Device time drift retry rate in PPM** to your oscillator's PPM
         rating (check your crystal datasheet). See
         :ref:`hubble_satellite_clock_drift` for details.
      #. Hover over each option to learn more, or refer to
         :ref:`hubble_configuration` for the complete options reference.

   .. tab:: Terminal

      Declare the extension, add the Hubble components, and set the options
      in your project's ``.slcp`` file:

      .. code-block:: yaml

         sdk_extension:
           # change '3.1.0' to the version of the Hubble Device SDK you installed
           - id: hubble-device-sdk
             vendor: hubble-network
             version: 3.1.0

         component:
           - id: hubble-device-sdk-ble
             from: hubble-device-sdk
           - id: hubble-device-sdk-sat
             from: hubble-device-sdk

         configuration:
           # 32 for AES-256, 16 for AES-128
           - name: SISDK_HUBBLE_KEY_SIZE
             value: "32"
           # 0 = Unix time, 1 = device uptime
           - name: SISDK_HUBBLE_COUNTER_SOURCE
             value: "0"
           # Check your oscillator's PPM rating
           - name: SISDK_HUBBLE_SAT_NETWORK_DEVICE_TDR
             value: "10"

      See :ref:`hubble_satellite_clock_drift` for details on TDR and how it
      affects retransmissions.

.. note::

   On Simplicity SDK, the options are named ``SISDK_HUBBLE_*`` and map to the
   ``CONFIG_HUBBLE_*`` options in :ref:`hubble_configuration`, e.g.
   ``SISDK_HUBBLE_KEY_SIZE`` sets ``CONFIG_HUBBLE_KEY_SIZE``.

.. caution::

   The Bluetooth stack reserves only **1** advertising set by default
   (``SL_BT_CONFIG_USER_ADVERTISERS``, Max number of advertising sets
   reserved for user in the **Advertising Base Feature** component). If
   your application advertises more than one set at a time, for example a
   connectable set for BLE provisioning alongside the Hubble beacon, raise it
   accordingly. Otherwise creating the extra set fails.

Application Components
======================

Besides the Hubble components, a dual-stack application needs the following
components in its ``.slcp``:

.. code-block:: yaml

   component:
     - id: freertos
     - id: bluetooth_stack
     - id: gatt_configuration
     - id: bluetooth_feature_gatt_server
     - id: bluetooth_feature_connection
     - id: bluetooth_feature_connection_role_peripheral
     - id: bluetooth_feature_legacy_advertiser
     - id: bluetooth_feature_system
     - id: mbedtls_base64

``bluetooth_stack`` and the ``bluetooth_feature_*`` components provide BLE
advertising and the GATT server used for provisioning; Hubble uses legacy
advertising. ``gatt_configuration`` adds the ``.btconf`` GATT database.
``mbedtls_base64`` is only required if your application decodes a
base64-encoded key at runtime, as described in
:ref:`silabs_freertos_device_key`.

.. note::

   See ``samples/freertos/silabs/sat-dual-stack`` for a complete reference
   ``.slcp``, including the GATT provisioning service in
   ``config/btconf/gatt_configuration.btconf``.

.. include:: ../common/data-requirements.rst
   :start-after: hubble-integration-data-requirements


.. _silabs_freertos_device_key:

.. include:: ../common/handling-device-key.rst
   :start-after: hubble-integration-device-key

To supply the key from configuration, declare a string option in a config
header of your project and set it in the Project Configurator, or with
``--configuration`` when generating with ``slc``:

.. code-block:: c

   // <s.44 HUBBLE_DEVICE_KEY> Base64-encoded Hubble master key
   // <d> ""
   #define HUBBLE_DEVICE_KEY ""

Decode it before handing it to ``hubble_init()``. This requires the
``mbedtls_base64`` component in your ``.slcp``:

.. code-block:: c

   #include "mbedtls/base64.h"

   static uint8_t _hubble_key[CONFIG_HUBBLE_KEY_SIZE];

   /* ... */

   if (strlen(HUBBLE_DEVICE_KEY) != 0) {
       size_t outlen = 0;

       err = mbedtls_base64_decode(
           _hubble_key, sizeof(_hubble_key), &outlen,
           (const unsigned char *)HUBBLE_DEVICE_KEY,
           strlen(HUBBLE_DEVICE_KEY));

       app_assert(err == 0 && outlen == sizeof(_hubble_key),
                  "Invalid key provided!\n");
   }

See ``samples/freertos/silabs/sat-dual-stack`` for a complete example.


.. _silabs_freertos_sat_sdk_init:

.. include:: ../common/sdk-init.rst
   :start-after: hubble-integration-sdk-init


The Pass Prediction Loop
************************

.. include:: ../common/pass-prediction.rst
   :start-after: hubble-integration-pass-prediction

Beacon over BLE While Waiting
==============================

Schedule a timer for the pass window and start BLE advertising while the
device waits. The example below uses ``sl_sleeptimer``; any timer that can
schedule a future callback works equally well.

.. code-block:: c

       /*
        * Schedule a sleeptimer for the satellite pass window.
        * Verify the wait does not exceed the maximum timeout the
        * sleeptimer supports.
        */
       status = sl_sleeptimer_start_timer_ms(&sat_pass_timer,
                                             pass_info.start - now_ms,
                                             sat_pass_timer_cb, NULL, 0, 0);
       if (status != SL_STATUS_OK) {
           app_log_error("Failed to start pass timer (status 0x%04lx)\n",
                         status);

           /*
            * Clean up the task and free its resources
            * (BLE state, timers, semaphores).
            */
           goto cleanup;
       }

       /* Get the Hubble beacon payload and start advertising */
       hubble_ble_advertise_get(inputBuf, inputBufLen, outBuf, &outBufLen);
       ble_adv_start();

       /*
        * Block until the pass timer fires. Common approaches are
        * a semaphore, task notification, or event flag.
        */
       xSemaphoreTake(sat_pass_sem, portMAX_DELAY);

Transmit to the Satellite
==========================

Stop advertising, then build and send the packet:

.. code-block:: c

       ble_adv_stop();

       err = hubble_sat_packet_get(&packet, NULL, 0);
       if (err != 0) {
           app_log_error("Failed to build packet (err %d)\n", err);
           goto cleanup;
       }

       /* Blocking call. Retries are handled internally by the SDK */
       err = hubble_sat_packet_send(&packet, HUBBLE_SAT_RELIABILITY_NORMAL);
       if (err != 0) {
           app_log_error("Failed to send packet (err %d)\n", err);
           goto cleanup;
       }
   } /* end while loop, back to compute the next pass and restart beacon */

:c:func:`hubble_sat_packet_send` is blocking. It returns only after the full
transmission sequence completes, including all retries. See
:ref:`hubble_satellite_reliability` for guidance on reliability modes and
their effect on power consumption.


Building and Flashing
*********************

.. tabs::

   .. tab:: Simplicity Studio 6

      #. Click the **Build** icon to build the project.
      #. Click the **Flash** icon to program the device.
      #. Click the **Debug** icon to launch the debugger.

      See the Silicon Labs `Build, Flash, or Debug a Project`_ guide for more
      details.

   .. tab:: Terminal

      Generate a Makefile project, then build it:

      .. code-block:: bash

         $SLC generate --sdk $SISDK -tlcn gcc -o makefile -cp \
             -p <your-app>.slcp -d <output-dir> --with <board>

         cd <output-dir>
         make -f <your-app>.Makefile

      Flash the built firmware with Simplicity Commander:

      .. code-block:: bash

         $COMMANDER flash build/debug/<your-app>.s37


Verifying the Application
**************************

Expected Log Output
===================

The SDK logs through the ``app_log`` component, which the Hubble components
pull in, to the virtual COM port. The log level is set to debug by default.

After a successful :c:func:`hubble_init` call, the SDK logs:

.. code-block:: none

   Hubble Device SDK initialized (HDCV:1.0/E:256/CS:UT/RP:S86400/N:TS/TV:0/SV:0)

At debug level, once pass prediction runs and a transmission is scheduled:

.. code-block:: none

   Time drift since last sync: 20000 ms
   Number of additional retries due TDR: 1
   Number of retries: 9 - interval: 20 seconds

After :c:func:`hubble_sat_packet_send` completes:

.. code-block:: none

   Hubble Satellite packet sent

If this line appears without any preceding error, the device has
successfully transmitted to the satellite network.

Verify BLE
==========

Use the SDK's scan script to confirm BLE advertising is working. Install the
dependencies and run:

.. code-block:: bash

   pip install -r tools/requirements-scan.txt
   python tools/scan.py --key "<your-device-key>"

The script scans for Hubble BLE beacons (UUID 0xFCA6) and prints decoded
packets in real time. Running without ``--key`` prints raw packets without
decryption.

Alternatively, use the **Hubble Connect** mobile app to see nearby Hubble
devices:

* `Hubble Connect on the App Store`_
* `Hubble Connect on Google Play`_

Verify Satellite RF
===================

To verify the satellite RF output before a live pass, use an ADALM-PLUTO SDR
and the ``pyhubblenetwork`` scan tool. See the **RF Verification with an SDR**
section in **Next Steps** below for full instructions.


Troubleshooting
***************

.. include:: ../common/troubleshooting-common.rst
   :start-after: hubble-integration-troubleshooting-common

Creating an advertising set fails
=================================

**Symptom:** ``sl_bt_advertiser_create_set`` fails when the application
creates a second advertising set, e.g. for BLE provisioning alongside the
Hubble beacon.

**Cause:** The Bluetooth stack reserves only one advertising set by default.

**Fix:** Raise ``SL_BT_CONFIG_USER_ADVERTISERS`` (**Max number of
advertising sets reserved for user** in the **Advertising Base Feature**
component) to the number of sets your application uses.

Application stalls when no terminal is connected
================================================

**Symptom:** The application stops making progress, e.g. no BLE advertising
or satellite transmission, while no serial terminal is reading the virtual
COM port.

**Cause:** Hardware flow control (CTS/RTS) is enabled on the virtual COM
port by default on some boards. Logging blocks while nothing reads the port.

**Fix:** Disable flow control by setting
``SL_IOSTREAM_EUSART_VCOM_FLOW_CONTROL_TYPE`` to
``SL_IOSTREAM_EUSART_UART_FLOW_CTRL_NONE``, or disable logging
(``APP_LOG_ENABLE`` = ``0``).


.. _silabs_freertos_sat_next_steps:

.. include:: ../common/next-steps.rst
   :start-after: hubble-integration-next-steps

Further Reading
===============

* :ref:`hubble_satellite_introduction`: satellite protocol details,
  reliability modes, and power trade-offs.
* :ref:`hubble_configuration`: full configuration reference for all
  ``CONFIG_HUBBLE_*`` options.
* :ref:`hubble_timing`: time management best practices for devices
  with and without a real-time clock.
* `Silicon Labs Simplicity SDK`_: Silicon Labs' SDK documentation,
  examples, and release notes.


.. _Silicon Labs Simplicity SDK: https://www.silabs.com/software-and-tools/simplicity-software-development-kit?tab=overview
.. _Simplicity Studio 6: https://www.silabs.com/developer-tools/simplicity-studio
.. _Silicon Labs Tool (SLT): https://docs.silabs.com/simplicity-installer-slt/latest/slt-getting-started-slt-cli/first-time-installation-setup
.. _Useful paths: https://docs.silabs.com/ssv6ug/6.2.0/ssv6-tricks-tips/#useful-paths
.. _Build, Flash, or Debug a Project: https://docs.silabs.com/ss-vscode/latest/ss-vscode-projects/build-flash-debug-project#build-flash-or-debug-a-project
.. _Hubble Connect on the App Store: https://apps.apple.com/us/app/hubble-connect/id6751236191
.. _Hubble Connect on Google Play: https://play.google.com/store/apps/details?id=com.hubble.connect
