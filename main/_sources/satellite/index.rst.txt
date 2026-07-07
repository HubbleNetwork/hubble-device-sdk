.. _hubble_satellite_introduction:

Satellite Network Overview
###########################

.. warning::

   The satellite functionality is currently in **pre-production** and is not yet
   ready for production deployments. APIs and behavior may change in future
   releases.

Introduction
************

The Hubble Satellite Network lets devices send messages through Hubble's
satellite network without adding a cellular modem or gateway. It is designed
for devices that need connectivity beyond the terrestrial coverage.

The application is responsible for deciding *when* to transmit. Two common
patterns are supported:

* **Continuous transmission** for devices that are constantly powered or are in a
  test environment.
* **Pass-predicted transmission** for devices that should transmit only when a
  satellite pass is expected.

Satellite packets can carry payloads of ``0``, ``4``, ``9``, or ``13`` bytes.
The SDK encrypts the payload using the configured Hubble device key and derives
the packet identifiers from the SDK time counter.

How It Works
************

At a high level, a satellite transmission follows this sequence:

1. Initialize the SDK with :c:func:`hubble_init`.
2. Build a satellite packet with :c:func:`hubble_sat_packet_get`.
3. Send the packet with :c:func:`hubble_sat_packet_send`.
4. The platform port enables the radio, converts the packet into frames, sends
   each frame, and disables the radio when transmission is complete.

The SDK uses the configured counter source for key derivation and packet
identity:

* ``CONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME`` uses Unix time in milliseconds.
* ``CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME`` uses a device-uptime-derived
  counter and does not require UTC time.

Use Unix time when the device can provision or synchronize time and needs pass
prediction. Use device uptime for simple continuous transmission workflows that
do not need wall-clock scheduling.

Packet Generation
=================

:c:func:`hubble_sat_packet_get` creates the encrypted satellite packet. The
input payload length must be one of the supported sizes: ``0``, ``4``, ``9``, or
``13`` bytes.

.. code-block:: c

   struct hubble_sat_packet packet;
   uint8_t payload[4] = { 0x01, 0x02, 0x03, 0x04 };

   err = hubble_sat_packet_get(&packet, payload, sizeof(payload));
   if (err != 0) {
           /* Handle error. */
   }

An empty payload is valid and is useful when the device only needs to report its
presence:

.. code-block:: c

   err = hubble_sat_packet_get(&packet, NULL, 0);

Transmission
============

:c:func:`hubble_sat_packet_send` sends a prepared packet with the selected
reliability mode:

.. code-block:: c

   err = hubble_sat_packet_send(&packet, HUBBLE_SAT_RELIABILITY_NORMAL);

The function is blocking. It returns after the full transmission sequence has
completed or after an error is reported by the platform radio port.

The reliability mode controls the trade-off between delivery probability and
energy use. See :ref:`hubble_satellite_reliability` for guidance on selecting a
mode.

Pass Prediction
***************

Pass prediction lets the application transmit only when a satellite is expected
to be visible from the device location. This reduces radio-on time and is the
recommended workflow for battery-powered devices.

.. warning::

   Pass prediction requires ``CONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME``. Never use
   pass prediction with ``CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME``. Both pass
   prediction and the drift-based retry compensation rely on synchronized Unix
   time, which the device-uptime counter source does not provide.

The pass prediction API needs three inputs:

* Current Unix time in milliseconds when asking for a pass.
* Device location as latitude and longitude in degrees.
* One or more satellite orbital parameter records.

Orbital Parameters (Satellites information)
===========================================

The SDK stores a pointer to the orbital parameter array passed to
:c:func:`hubble_sat_satellites_set`; it does not copy the array. Keep the array
valid for as long as pass prediction is used.

.. code-block:: c

   #define MAX_SATELLITES 6

   static struct hubble_sat_orbital_params satellites[MAX_SATELLITES];
   static size_t satellite_count;

   err = hubble_sat_satellites_set(satellites, satellite_count);
   if (err != 0) {
           /* Handle invalid provisioning data. */
   }

Each ``struct hubble_sat_orbital_params`` describes one satellite well enough
for the SDK to predict when it can be reached from the device location.
Provision these values from a trusted source such as a companion application,
BLE provisioning flow, manufacturing data, or another backend channel.

The dual-stack samples provision orbital parameters over BLE. The provisioning
payload maps directly to ``struct hubble_sat_orbital_params`` fields:

.. code-block:: c

   memcpy(&sat->t0,           p + 0,  sizeof(uint64_t));
   memcpy(&sat->n0,           p + 8,  sizeof(double));
   memcpy(&sat->ndot,         p + 16, sizeof(double));
   memcpy(&sat->raan0,        p + 24, sizeof(double));
   memcpy(&sat->raandot,      p + 32, sizeof(double));
   memcpy(&sat->aop0,         p + 40, sizeof(double));
   memcpy(&sat->aopdot,       p + 48, sizeof(double));
   memcpy(&sat->inclination,  p + 56, sizeof(double));
   memcpy(&sat->eccentricity, p + 64, sizeof(double));
   memcpy(&sat->satellite_id, p + 72, sizeof(uint32_t));

When receiving binary provisioning data, copy each field separately as shown
above. This avoids unaligned accesses on MCUs that do not support them.

For devices that do not receive orbital parameters at runtime, you can generate
a C source file with hard-coded satellite parameters and build it into the
application. The ``tools/orbital_params_fetch.py`` helper fetches current
orbital parameters from the Hubble API and writes ``sat_params.c``.

Set ``HUBBLE_API_TOKEN`` to a Hubble API bearer token, then run the tool from the
SDK repository:

.. code-block:: console

   export HUBBLE_API_TOKEN=<token>
   python tools/orbital_params_fetch.py path/to/output

The generated file contains an array named ``sats``:

.. code-block:: c

   #include <hubble/sat/pass_prediction.h>

   struct hubble_sat_orbital_params sats[] = {
           /* Generated satellite orbital parameters. */
   };

Compile the generated ``sat_params.c`` into your application and register the
array before computing passes:

.. code-block:: c

   extern struct hubble_sat_orbital_params sats[];

   err = hubble_sat_satellites_set(sats, ARRAY_SIZE(sats));
   if (err != 0) {
           /* Handle error. */
   }

Hard-coded parameters are simple and work well for fixed firmware images or
devices without a provisioning channel. Refresh and rebuild the generated file
when the orbital data used by your product needs to be updated.

Computing the Next Pass
=======================

After the SDK has time, location, and orbital data, use
:c:func:`hubble_sat_next_pass_get` to find the next pass for one location:

.. code-block:: c

   struct hubble_sat_device_pos pos = {
           .lat = 47.0,
           .lon = -122.0,
   };
   struct hubble_sat_pass_info pass;
   uint64_t now_ms = hubble_time_get();

   err = hubble_sat_next_pass_get(now_ms, &pos, &pass);
   if (err != 0) {
           /* No pass found or invalid input. */
   }

``pass.start`` is the recommended time to begin the satellite transmission
sequence. ``pass.culmination`` is the time of maximum pass alignment.
``pass.duration`` is the expected transmission window duration in milliseconds.
``pass.max_elevation_angle`` reports the maximum elevation angle for the pass.

If a pass is already in progress, compute the next one by starting the search
after the current pass window:

.. code-block:: c

   if (pass.start <= now_ms) {
           err = hubble_sat_next_pass_get(pass.start + pass.duration,
                                          &pos, &pass);
   }

Common API Workflow
*******************

Continuous Transmission
=======================

Use continuous transmission when the device is constantly powered, when power is
not the primary constraint, or when validating radio integration. The sample
applications under ``samples/*/sat-continuous`` follow this pattern.

The example below initializes the SDK with ``hubble_init(0, master_key)``. This
requires ``CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME`` so the time counter is
derived from device uptime.

Typical workflow:

1. Provision the Hubble key.
2. Initialize the SDK.
3. Build a packet.
4. Send the packet.
5. Sleep for an application-defined interval.
6. Repeat.

.. code-block:: c

   err = hubble_init(0, master_key);
   if (err != 0) {
           return err;
   }

   for (;;) {
           err = hubble_sat_packet_get(&packet, NULL, 0);
           if (err != 0) {
                   return err;
           }

           err = hubble_sat_packet_send(&packet,
                                        HUBBLE_SAT_RELIABILITY_NORMAL);
           if (err != 0) {
                   return err;
           }

           sleep_until_next_application_interval();
   }

This mode is simple, but it can spend energy transmitting when no satellite is
in view. For production battery-powered devices, prefer pass prediction when
time and orbital data are available.

Dual Stack with Pass Prediction
===============================

The dual-stack samples combine BLE provisioning/beaconing with satellite
transmission. The device uses BLE while waiting for the next satellite pass,
then stops BLE and transmits over the satellite radio during the pass window.

Typical workflow:

1. Start BLE provisioning.
2. Receive Unix time, device location, and satellite orbital parameters.
3. Initialize the Hubble SDK with the provisioned Unix time and key.
4. Register orbital parameters with :c:func:`hubble_sat_satellites_set`.
5. Compute the next pass with :c:func:`hubble_sat_next_pass_get`.
6. Start BLE beaconing while waiting for ``pass.start``.
7. Stop BLE when the pass timer expires.
8. Build and send a satellite packet.
9. Repeat from pass calculation (step 5).

.. code-block:: c

   err = hubble_init(unix_time_ms, master_key);
   if (err != 0) {
           return err;
   }

   err = hubble_sat_satellites_set(orb_params, orb_params_count);
   if (err != 0) {
           return err;
   }

   for (;;) {
           uint64_t now_ms = hubble_time_get();

           err = hubble_sat_next_pass_get(now_ms, &device_pos, &pass);
           if (err != 0) {
                   return err;
           }

           schedule_timer_for(pass.start - now_ms);
           ble_adv_start();
           wait_for_pass_timer();
           ble_adv_stop();

           err = hubble_sat_packet_get(&packet, NULL, 0);
           if (err != 0) {
                   return err;
           }

           err = hubble_sat_packet_send(&packet,
                                        HUBBLE_SAT_RELIABILITY_NORMAL);
           if (err != 0) {
                   return err;
           }
   }

This workflow is appropriate for devices that already use BLE for local
connectivity, provisioning, or nearby network operation and only need satellite
transmission during predicted windows.

Transmission Logic
******************

The reliability mode controls how many times the SDK asks the platform radio
port to send the same packet and how far apart those transmissions are spaced.

.. list-table:: Satellite Reliability Modes
   :widths: 30 20 20 30
   :header-rows: 1

   * - Mode
     - Baseline transmissions
     - Interval
     - Intended use
   * - ``HUBBLE_SAT_RELIABILITY_NONE``
     - 1
     - 0 seconds
     - Testing or externally managed retries
   * - ``HUBBLE_SAT_RELIABILITY_NORMAL``
     - 8
     - 20 seconds
     - Default balance of reliability and power
   * - ``HUBBLE_SAT_RELIABILITY_HIGH``
     - 16
     - 10 seconds
     - Higher reliability with higher energy cost

Clock Drift Compensation
========================

For modes with retries, the SDK may add extra transmissions to account for
estimated clock drift since the last time synchronization. The drift estimate is
based on ``CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR`` in parts per million (PPM).

The SDK computes the elapsed time since the last time sync, estimates drift, and
adds one retry for each full retransmission interval of drift. This widens the
effective transmission coverage when the device clock may be early or late.

Pass prediction also accounts for drift by starting the calculated transmission
window earlier. Keep Unix time synchronized when possible to reduce unnecessary
extra retries.

Understanding PPM
-----------------

PPM (parts per million) is the maximum frequency deviation of an oscillator from
its nominal frequency. A clock rated at 50 PPM can drift at most 50 microseconds
per second, which accumulates to:

* 50 ms per 1,000 seconds (~17 minutes)
* 180 ms per hour
* 4.3 seconds per day

The SDK uses ``CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR`` to estimate the total
accumulated drift since the last time synchronization:

.. code-block:: none

   drift_ms = (elapsed_since_last_sync_ms × TDR_ppm) / 1,000,000

One additional retry is then added for each full retransmission interval covered
by that drift. For example, with ``HUBBLE_SAT_RELIABILITY_NORMAL`` (20-second
interval) and a 100 PPM clock that has not synced for 3 hours:

.. code-block:: none

   drift_ms          = 10,800,000 ms × 100 / 1,000,000 = 1,080 ms
   additional_retries = 1,080 ms / 20,000 ms            = 0

At 30 hours without a sync:

.. code-block:: none

   drift_ms          = 108,000,000 ms × 100 / 1,000,000 = 10,800 ms
   additional_retries = 10,800 ms / 20,000 ms            = 0  (rounds down)

At 56 hours without a sync:

.. code-block:: none

   drift_ms          = 201,600,000 ms × 100 / 1,000,000 = 20,160 ms
   additional_retries = 20,160 ms / 20,000 ms            = 1

For most devices syncing time at least once per day, drift compensation adds
zero or one extra retries. Higher-PPM oscillators or longer sync gaps produce
more extra retries.

Maximum Additional Retries
~~~~~~~~~~~~~~~~~~~~~~~~~~

Two caps bound the total number of retries:

* The drift estimate is capped at **90 minutes** regardless of how long the
  device has gone without a time sync.
* The total retry count (baseline + additional) is capped to 90 min / retry interval.

In practice these ceilings are only reached by high-PPM oscillators that have
not synchronized time for many days. At 100 PPM, reaching 90 minutes of
accumulated drift requires roughly 625 days without a sync.

Finding the TDR for Your Hardware
----------------------------------

The Time Drift Rate (TDR) value to configure comes directly from the oscillator
used as the device clock source. The typical lookup path is:

1. **Identify the clock source.** Check the board schematic or SoC reference
   manual to find which oscillator drives the timekeeping peripheral (RTC or
   low-frequency oscillator). It is usually a 32.768 kHz crystal or, for
   lower-cost designs, an internal RC oscillator.

2. **Read the datasheet.** Open the crystal or oscillator datasheet and look
   for the **frequency accuracy** or **frequency tolerance** specification,
   typically in a table labelled *Electrical Characteristics* or *AC
   Characteristics*. The value is expressed as ±X ppm over a stated temperature
   and supply-voltage range.

3. **Use the worst-case figure.** Datasheets often list both an initial
   tolerance at room temperature and a wider tolerance over the full operating
   temperature range. Use the larger value.

Typical ranges by oscillator type:

.. list-table::
   :widths: 40 20 40
   :header-rows: 1

   * - Oscillator type
     - Typical PPM
     - Notes
   * - TCXO (temperature-compensated crystal)
     - 0.5 – 5
     - Best accuracy; used in communication-grade designs
   * - External crystal (XTAL)
     - 10 – 100
     - Most common; check datasheet for your specific part
   * - Internal RC oscillator
     - 200 – 5,000
     - Factory calibration at room temperature only; degrades with temperature

If you cannot obtain the exact figure, use a conservative (higher) estimate.
Underestimating PPM causes the device to transmit fewer extra retries than
needed, which reduces delivery probability when the clock has drifted
significantly.

Radio Timing
============

:c:func:`hubble_sat_packet_send` is synchronous. During the call, the platform
port typically:

1. Acquires the satellite transmission lock or semaphore.
2. Enables the satellite radio front-end.
3. Builds radio frames from the packet for each retry.
4. Sends the frames through the board radio implementation.
5. Sleeps until the next retry interval when more retries remain.
6. Disables the satellite radio front-end.
7. Releases the transmission lock or semaphore.

Applications should treat the call as a blocking radio operation and avoid
starting conflicting radio activity until it returns. Dual-stack applications
should stop BLE before satellite transmission unless the platform explicitly
supports concurrent operation.

Supporting New Boards
*********************

See :ref:`board_support` for integration paths, required APIs, SoC-specific
guidance, and the board support checklist.

.. _hubble_satellite_reliability:

Power Consumption and Reliability
*********************************

Satellite reliability and power consumption are directly related. More retries
increase the chance that a satellite receives the packet, but they keep the
radio active for longer and increase total energy use.

Use these guidelines when selecting a mode:

* Use ``HUBBLE_SAT_RELIABILITY_NONE`` only for testing, lab validation, or
  applications that implement their own scheduling and retry policy.
* Use ``HUBBLE_SAT_RELIABILITY_NORMAL`` as the default production setting.
* Use ``HUBBLE_SAT_RELIABILITY_HIGH`` when delivery probability is more
  important than energy consumption.
* Use pass prediction to avoid transmitting when a satellite is unlikely to be
  visible.
* Keep time synchronized to minimize drift-compensation retries.
* Avoid very short continuous-transmission intervals on battery-powered devices.

For battery-powered products, the most power-efficient design is usually a
pass-predicted workflow: sleep or use low-power BLE between passes, wake before
``pass.start``, transmit with the lowest reliability mode that meets the product
delivery requirement, then return to the low-power state.

Configuration Notes
*******************

Enable the satellite network with ``CONFIG_HUBBLE_SAT_NETWORK``.

Important related options include:

* ``CONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME`` for Unix-time-based operation.
* ``CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME`` for uptime-counter operation.
* ``CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR`` for clock-drift retry compensation.
* ``CONFIG_HUBBLE_NETWORK_KEY_128`` or ``CONFIG_HUBBLE_NETWORK_KEY_256`` for key
  size selection.
* ``CONFIG_HUBBLE_NETWORK_CRYPTO_*`` for crypto backend selection.

See :ref:`hubble_configuration` for the complete configuration reference.

API Reference
*************

.. toctree::
   :maxdepth: 1
   :glob:

   api.rst
   board-support
