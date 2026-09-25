.. _silabs_quick_start:

SiLabs Simplicity SDK
=====================

This guide explains how to integrate the Hubble Device SDK into a
`Simplicity SDK <https://www.silabs.com/developer-tools/simplicity-studio>`_
(SiSDK) project. The SDK ships as a Simplicity SDK extension
(``hubble-network.hubble-device-sdk``). Projects add it through their
``.slcp`` file, and the SLC tooling pulls in the SDK sources, include paths,
and configuration.

.. note::

   Only the Hubble Terrestrial (BLE) Network is currently supported on
   Simplicity SDK FreeRTOS.

Prerequisites
*************

.. tabs::

   .. tab:: Simplicity Studio

      - `Simplicity Studio 6 <https://www.silabs.com/developer-tools/simplicity-studio>`_
        with the Simplicity SDK installed.
      - The ``slc-cli`` tool, which is still needed to install the extension
        (see :ref:`silabs_install_extension`).

   .. tab:: Command line

      `Simplicity SDK <https://github.com/SiliconLabs/simplicity_sdk>`_,
      ``slc-cli``, the Arm GNU toolchain (``gcc-arm-none-eabi``), and
      Simplicity Commander (``commander``), installed with the
      `Silicon Labs Tool (SLT) <https://docs.silabs.com/simplicity-installer-slt/latest/slt-getting-started-slt-cli/first-time-installation-setup>`_.

- A Silicon Labs board with Bluetooth® Low Energy support.
- The Hubble Device SDK cloned or added as a submodule.
- Device ``key`` (generated when you register a new device to your organization
  through the Hubble Cloud API).

Set the following environment variables before continuing, e.g.:

.. code-block:: bash

   export SISDK=/path/to/simplicity_sdk
   export SLC=/path/to/slc_cli/slc
   export COMMANDER=/path/to/commander
   export HUBBLE_SDK=/path/to/hubble-device-sdk

.. tip::

   See `Useful paths <https://docs.silabs.com/ssv6ug/6.2.0/ssv6-tricks-tips/#useful-paths>`_
   for where Simplicity Studio and SLT install these tools.


.. _silabs_install_extension:

Installing the Extension
************************

SLC discovers extensions under the Simplicity SDK's ``extension`` directory.
Link the Hubble Device SDK there and register both the SDK and the extension
as trusted:

.. code-block:: bash

   mkdir -p $SISDK/extension
   ln -s $HUBBLE_SDK $SISDK/extension/hubble-device-sdk

   $SLC configuration --sdk $SISDK
   $SLC signature trust --sdk $SISDK
   $SLC signature trust -extpath $SISDK/extension/hubble-device-sdk


Adding Hubble Network to a Project
**********************************

.. tabs::

   .. tab:: Simplicity Studio

      1. Open your project's ``.slcp`` file to launch the Project Configurator.
      2. Go to **Software Components** and search for **Hubble**.
      3. Select **Hubble Device SDK Component** (under **Hubble** → **BLE**)
         and click **Install**.

   .. tab:: Command line

      Declare the extension in your project's ``.slcp`` file and add the
      Hubble component:

      .. code-block:: yaml

         sdk_extension:
           # change '3.1.0' to the version of the Hubble Device SDK you installed
           - id: hubble-device-sdk
             vendor: hubble-network
             version: 3.1.0

         component:
           - id: hubble-device-sdk-component
             from: hubble-device-sdk


Configuration
*************

The SDK is configured through ``hubble_config.h``. See
``$HUBBLE_SDK/port/sisdk/hubble_config.h`` for the available options and
their defaults.

.. tabs::

   .. tab:: Simplicity Studio

      In the Project Configurator, open **Software Components**, select
      **Hubble Device SDK Component**, and click **Configure**.

   .. tab:: Command line

      Set the options you want to change in your project's ``.slcp`` file,
      e.g. to provide your own sequence counter:

      .. code-block:: yaml

         configuration:
           - name: CONFIG_HUBBLE_NETWORK_SEQUENCE_NONCE_CUSTOM
             value: "1"


Building and Running Your First Application
*******************************************

The following steps build and flash the ``ble-beacon`` sample. Replace
``brd4187c`` with your target board.

Embed the device key and the current time into the sample sources:

.. code-block:: bash

   cd $HUBBLE_SDK/samples/freertos/silabs/ble-beacon
   $HUBBLE_SDK/tools/embed_key_time.py -b master.key -o src/

.. tabs::

   .. tab:: Simplicity Studio

      Generate a Simplicity Studio project:

      .. code-block:: bash

         $SLC generate -tlcn gcc -np -p ble_beacon.slcp \
             -d /path/to/workspace/ble-beacon --with brd4187c

      Then in Simplicity Studio go to **PROJECTS** → **Open Project(s)**,
      select the generated ``ble-beacon`` directory, and build and flash as
      usual.

   .. tab:: Command line

      Build the application:

      .. code-block:: bash

         $SLC generate -tlcn gcc -o makefile -cp -p ble_beacon.slcp \
             -d generated_sample_app --with brd4187c
         cd generated_sample_app
         make -f ble_beacon.Makefile

      Flash the application:

      .. code-block:: bash

         $COMMANDER flash build/debug/ble_beacon.s37

See the sample's ``README.md`` for key formats and troubleshooting.

