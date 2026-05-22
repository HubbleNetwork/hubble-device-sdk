.. _ti_quick_start:

TI SDK Quick Start (SysConfig)
==============================

This guide explains how to integrate the Hubble Network SDK into a
`SimpleLink Low Power F3 SDK <https://www.ti.com/tool/SIMPLELINK-LOWPOWER-F3-SDK>`_
project using `SysConfig <https://www.ti.com/tool/SYSCONFIG>`_. SysConfig
generates driver and peripheral initialization code from a ``.syscfg`` script,
and the Hubble Network SDK provides its own SysConfig product so its module
can be configured alongside the TI SDK drivers in the same script.

.. note::

   SysConfig is the recommended integration path for TI SDK projects. If you
   prefer to manage Hubble Network SDK build manually or are working in an
   environment without SysConfig, follow the generic
   :ref:`freertos_quick_start` instead — it describes how to include the SDK
   sources and flags directly via the provided Makefile fragment.

Prerequisites
*************

- `SimpleLink Low Power F3 SDK <https://www.ti.com/tool/SIMPLELINK-LOWPOWER-F3-SDK>`_ installed.
- `TI ARM LLVM Compiler <https://www.ti.com/tool/CCSTUDIO>`_ installed.
- SysConfig CLI tool (``sysconfig_cli.sh`` / ``sysconfig_cli.bat``) available.
- The Hubble Network SDK cloned or added as a submodule.

Set the following environment variables before building:

.. code-block:: bash

   export TICLANG_ARMCOMPILER=/path/to/ti-cgt-armllvm
   export SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR=/path/to/simplelink_lowpower_f3_sdk
   export SYSCONFIG_TOOL=/path/to/sysconfig_cli.sh
   export HUBBLE_NETWORK_SDK=/path/to/hubble-device-sdk


Adding the Hubble Network SDK SysConfig Product
***********************************************

The Hubble Network SDK ships a SysConfig product descriptor at
``.metadata/product.json``. Pass it to the SysConfig CLI alongside the TI SDK
product so both products are available in the same ``.syscfg`` script:

.. code-block:: Makefile

   SYSCFG_CMD_STUB = $(SYSCONFIG_TOOL) --compiler ticlang \
       --product $(SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR)/.metadata/product.json \
       -s $(HUBBLE_NETWORK_SDK)/.metadata/product.json

Generate the configuration files into your build directory:

.. code-block:: Makefile

   SYSCFG_FILES := $(shell $(SYSCFG_CMD_STUB) \
       --listGeneratedFiles --listReferencedFiles \
       --output $(BUILD_DIR) your-app.syscfg)

   SYSCFG_C_FILES   = $(filter %.c,$(SYSCFG_FILES))
   SYSCFG_H_FILES   = $(filter %.h,$(SYSCFG_FILES))
   SYSCFG_OPT_FILES = $(filter %.opt,$(SYSCFG_FILES))


Configuring the Hubble Module in a ``.syscfg`` Script
******************************************************

Load the Hubble module in your ``.syscfg`` script with:

.. code-block:: javascript

   var hubble = scripting.addModule("/Hubble");

This is all that is required. The module registers the Hubble Network SDK
sources and include paths with the SysConfig framework so the generated
``.opt`` files carry the right compiler flags.

A minimal ``.syscfg`` script for a BLE application on CC23xx looks like:

.. code-block:: javascript

   // @cliArgs --board /ti/boards/LP_EM_CC2340R5 --rtos freertos

   /* TI drivers */
   var AESCCM = scripting.addModule("/ti/drivers/AESCCM");
   AESCCM.addInstance().$name = "CONFIG_AESCCM0";

   var RCL   = scripting.addModule("/ti/drivers/RCL");
   var Power = scripting.addModule("/ti/drivers/Power");

   /* Hubble Network SDK */
   var hubble = scripting.addModule("/Hubble");

   /* BLE stack and FreeRTOS */
   var ble      = scripting.addModule("/ti/ble/ble");
   ble.basicBLE = true;

   const FreeRTOS = scripting.addModule("/freertos/FreeRTOS");
   FreeRTOS.heapSize = 0x00004D50;


Using Code Composer Studio (CCS)
*********************************

If you are using `Code Composer Studio <https://www.ti.com/tool/CCSTUDIO>`_
and prefer not to write a Makefile manually, you can add the Hubble Network SDK
SysConfig product directly from the IDE:

1. Right-click the project folder in the **Project Explorer** and select
   **Properties**.
2. Navigate to **General** → **SysConfig**.
3. In the **SysConfig Flags** field, append the following flag:

   .. code-block:: none

      -s "/path/to/hubble-device-sdk/.metadata/product.json"

   Replace ``/path/to/hubble-device-sdk`` with the actual path to your Hubble
   Network SDK checkout.

4. Click **Apply and Close**. CCS will pass this flag to the SysConfig CLI
   during the build, making the ``/Hubble`` module available in your
   ``.syscfg`` script just as described in the section above.


Building the Application
************************

Run SysConfig to generate the configuration files and then build:

.. code-block:: bash

   make -f Makefile

The ``Makefile`` must pass the generated ``.opt`` files to the
compiler using the ``@`` prefix so that SysConfig-generated flags (include
paths, preprocessor definitions) are applied:

.. code-block:: Makefile

   CFLAGS += $(addprefix @,$(SYSCFG_OPT_FILES))


See the ``samples/freertos/ti/ble_beacon`` sample for a complete working example.
