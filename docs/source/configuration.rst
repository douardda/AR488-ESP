.. _Compilation

Compilation
===========

The recommended tool to compile and upload the AR488-ESP32 firmware is
`platformio`_ either using the VSCode based IDE or using direcly the `command
line tools`_ (solution used in this document).

.. _platformio: https://platformio.org
.. _`command line tools`: https://docs.platformio.org/en/latest/core/index.html

Configuration
-------------

The entry point for choosing the target board to compile the formware for, as
well as configuring the compile-time options is the ``platformio.ini`` file
located at the root of the source code directory.

Besides the main ``[platformio]`` section, any ``[env:xxx]`` section defines a
new target for with the firmware can be built.

The main configuration for a given target is achieved via a number of compiler
variable given as build flags.

Possible variables are:

- ``AR488_CUSTOM``: if set, the pin mapping must be provided via a series of
  build flags.

- ``AR488_WIFI_ENABLE``: if set, enable the ESP32 wifi for wireless
  communication protocol.

- ``AR488_BT_ENABLE``: if set, enable the ESP32 Bluetooth for wireless
  communication protocol.

- ``USE_MACROS``: enable support for run-time configurable macros (see
  :ref:`macos`_)

- ``HAS_HELP_COMMAND``: enable support for the ``++help`` command; on limited
  boards (like some AVR boards), this can consume too much storage memory and
  make the firmmware not fit in the flash storage. On ESP32 boards, there is no
  reason not to enable it.

- ``BOARD_HAS_PSRAM``: activate usage od PSRAM on ESP32 boards that have
  support for it; this is generally required if you have many features enabled
  (thus need mode RAM than the stock one, especially when using wifi etc.).

- ``SN7516X``: enable support for SN76160 and SN75161/2 transceivers between
  the ESP32 board and the GPIB bus. If set, you need to configure also the
  control pins of the transceivers (see below).

- ``DIOX=nn``: set the pin mapping for the GPIB data pin ``Dx`` (used if
  ``AR488_CUSTOM`` is set).

- ``REN|IFC|NDAC|NRFD|DAV|EOI|ATN|SRQ=nn``: set the pin mapping for GPIB
  control pins (idem).

- ``SN7516X_TE|SN7516X_DC=nn``: pin mapping for the SN7616x TE and DC pins

- ``SN7516X_SC=nn``: pin mapping for the SN76162 SC pin (only required if using
  the SN75162B transceiver).


TO enable one of the features listed above, just add the ``-D XXXFEATURE`` in
the ``build_flags`` section of your build target.

The ``esp32dev`` example below is a configuration section for a ESP32-devkitc
based board with SN75160/2 transceiver and all options enabled:

.. code-block:: ini

   [env:esp32dev]
   platform = espressif32
   framework = arduino
   board = esp32dev
   board_build.partitions = ttgo.csv
   build_flags =
        -D AR488_CUSTOM
        -D USE_MACROS
        -D HAS_HELP_COMMAND
        -D AR488_WIFI_ENABLE
        -D BOARD_HAS_PSRAM
        -D AR488_BT_ENABLE
        -D SN7516X -D SN7516X_TE=15 -D SN7516X_DC=2
        -D SN7516X_SC=0  # for 75162
        -D DIO1=33 -D DIO2=32 -D DIO3=26  -D DIO4=25
        -D DIO5=14 -D DIO6=27 -D DIO7=13  -D DIO8=12
        -D REN=23  -D IFC=22  -D NDAC=21  -D NRFD=19
        -D DAV=18  -D EOI=17  -D ATN=4    -D SRQ=16

Note: this example uses a custom partition scheme for the flash memory
(otherwise the full-featured firmware does not fit in a sigle A/B partition).


Building
--------


Once the target is properly configured in the ``platformio.ini`` file, you can
build it using the standard platformio environment. Using the cli tools,
building the ``esp32dev`` target would be:

.. code-block:: bash

   AR488-ESP32$ pio run -e esp32dev
   Processing esp32dev (board: esp32dev; platform: espressif32; framework: arduino)
   ----------------------------------------------------------------------------------------------
   Verbose mode can be enabled via `-v, --verbose` option
   CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/esp32dev.html
   PLATFORM: Espressif 32 (6.0.0) > Espressif ESP32 Dev Module
   HARDWARE: ESP32 240MHz, 320KB RAM, 4MB Flash
   DEBUG: Current (cmsis-dap) External (cmsis-dap, esp-bridge, esp-prog, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa)
   PACKAGES:
    - framework-arduinoespressif32 @ 3.20006.221224 (2.0.6)
    - tool-esptoolpy @ 1.40400.0 (4.4.0)
    - toolchain-xtensa-esp32 @ 8.4.0+2021r2-patch5
   Converting AR488.ino
   LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
   LDF Modes: Finder ~ chain, Compatibility ~ soft
   Found 33 compatible libraries
   Scanning dependencies...
   Dependency Graph
   |-- EEPROM @ 2.0.0
   |-- Preferences @ 2.0.0
   |-- WiFi @ 2.0.0
   |-- BluetoothSerial @ 2.0.0
   Building in release mode
   Compiling .pio/build/esp32dev/src/AR488.ino.cpp.o
   Retrieving maximum program size .pio/build/esp32dev/firmware.elf
   Checking size .pio/build/esp32dev/firmware.elf
   Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
   RAM:   [==        ]  17.2% (used 56464 bytes from 327680 bytes)
   Flash: [=======   ]  74.0% (used 1503421 bytes from 2031616 bytes)
   ================================ [SUCCESS] Took 3.67 seconds ================================

   Environment    Status    Duration
   -------------  --------  ------------
   esp32dev       SUCCESS   00:00:03.671
   ================================ 1 succeeded in 00:00:03.671 ================================


The generated firmware file is located in ``.pio/build/esp32dev/firmware.bin``.

Note: if you do not specify a target (``-e xxx``) then all the targets defined
in the ``platformio.ini`` file will be built, which can take quite a while.

Uploading this to the board is then a simple matter of using the command:

.. code-block:: bash

   AR488-ESP32$ pio run -e esp32dev -t upload
   [...]
