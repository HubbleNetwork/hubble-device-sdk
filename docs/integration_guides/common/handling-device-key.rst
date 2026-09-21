:orphan:

.. hubble-integration-device-key

Handling the Hubble Device Key
******************************

Every device authenticates to the Hubble Network with a cryptographic key
issued by the Hubble Platform API. The key is either AES-128 or AES-256, and
its size must match the size the SDK was built for
(``CONFIG_HUBBLE_NETWORK_KEY_128`` or ``CONFIG_HUBBLE_NETWORK_KEY_256``,
exposed in bytes as ``CONFIG_HUBBLE_KEY_SIZE``). Requesting a key of one size
and building for the other fails at initialization.

The key reaches the SDK through ``hubble_init()``:

.. code-block:: c

   int hubble_init(uint64_t initial_time, const void *key);

.. important::

   The SDK stores the pointer you hand it and does **not** copy the key
   material. The buffer must stay valid for as long as the SDK is in use, so
   it cannot be a stack or otherwise temporary buffer. Give it static storage
   duration.

Passing ``NULL`` defers the key; supply it later with ``hubble_key_set()``
before requesting any advertisement.

There are two common ways to get a key onto a device. Not every platform
supports both, and some offer further options; see the platform-specific notes
that follow.

Embedding the Key at Build Time
===============================

This is the recommended method for production applications. It is the simplest
option to automate in a build, and the only one that provisions the initial
time in the same step. The key material still lives in the firmware image, so
treat the image as a secret.

To embed the key at build time, point ``embed_key_time.py`` at <your_src_dir>/:

.. code-block:: bash

   python tools/embed_key_time.py -b /path/to/key -o <your_src_dir>/

This writes ``<your_src_dir>/key.c`` and ``<your_src_dir>/time.c``, defining ``master_key`` and
``unix_time``. Both symbols are declared ``static``, so the generated ``.c``
files are meant to be included into the translation unit that uses them rather
than compiled separately. Include them into the translation unit that calls
``hubble_init()``:

.. code-block:: c

   #include "key.c"
   #include "time.c"

   /* ... */

   err = hubble_init(unix_time, master_key);

See any of the ``ble-beacon`` samples for a complete example.

Supplying the Key from Project Configuration
============================================

Use this for debugging and testing only. Production applications should embed
the key at build time instead.

Rather than generating source, the application can carry the key as a
base64-encoded string in its own build configuration and decode it at runtime
before calling ``hubble_init()``. Decode into a buffer with static storage
duration, and verify that the decoded length matches ``CONFIG_HUBBLE_KEY_SIZE``
before using it.

This supplies only the key. The initial time passed to ``hubble_init()`` has to
be obtained separately, by whatever means suits the device: a real-time clock,
a network time source, or provisioning from a companion application.

.. note::

   The SDK does not define a configuration option for the key. Declare one in
   your own application and name it to suit your project; the option names
   used in the platform notes that follow are illustrative.
