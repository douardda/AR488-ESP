# ESP32-488 ESP32/Arduino based GPIB Interface


The ESP32-488 GPIB controller is an
[ESP32-based](https://www.espressif.com/en/products/socs/esp32) controller for
interfacing with IEEE488 GPIB devices via USB using the Arduino developement
environment. This project is a friendly fork from the
[AR488](https://github.com/Twilight-Logic/AR488) project aiming at supporting the ESP32
family platform and improving the source code of the firmware. The original AR488 itself
was inspired by and has been based on the work originally released by Emanuele Girlando.

This sketch represents a major refactoring of the orignal AR488 source code.

It implements the full set of Prologix ++ commands in both controller and device mode.
Secondary GPIB addressing is not yet supported. A number of additional features are
provided, for example, a macro feature is provided to allow automation of frequently
used command sequences was well as controller and instrument initialisation at startup.
Interfacing with SN75160 and SN75161 GPIB transceiver integrated circuits is supported.

## Features

All original AR488 features:

- use Prologix command syntax
- support most Prologix commands
- optionnaly support SN7516x transceivers

plus a number of new features:

- wireless connections (wifi and/or bluetooth, depending on the EPS32 core used),
- embedded help message
- run-time configurable wifi connection settings (stored in the nvram),
- up to 10 run-time editable macros (stored in the nvram), macro 0 being executed at startup
- `++allspoll` command (implement the IEEE-488.2 ``ALLSPOLL`` protocol)
- `++findlstn` command (implement the IEEE-488.2 ``FINDLSTN`` protocol)
- `++findrqs` command (implement the IEEE-488.2 ``FINDRQS`` protocol)
- `++tct` command (implement the IEEE-488 ``TCT (Take Control)`` message)


## Supported boards and interface design

While targeting the ESP32 platform, this firmware reamains compatible with standard
Arduino boards. However, some features are not supported on old boards, and 328p based
boards should not be considered as they are very limited in flash and RAM space.

To build an interface, at least one Arduino board will be required to act as the
interface hardware. Arduinos provide a low cost alternative to other commercial
interfaces. Currently the following boards are supported:

| MCU  | Board | Serial Ports | Layouts |
| ---- | ----- | ------------ | ------- |
| ESP32 | Any  | UART over USB, Bluetooth, Wifi | configurable |
| ESP32S2 | Any  | UART over USB, Wifi | configurable |

Generally speaking, any ESP32 based board should work. When choosing a layout,
pay attention to pins which have special purpose on the ESP32 as well as pins
that are inputs only.

Including the SN7516x chipset into the interface design will naturally add to
the cost, but has the advantage of providing the full 48mA drive current
capacity regardless of the capability of the board being used, as well as
providing proper tri-state output with Hi-Z when the board is powered down. The
latter isolates the micro-controller from the GPIB bus when the interface is
powered down, preventing GPIB bus communication problems due to 'parasitic
power' from signals present on the GPIB bus, thereby allowing the interface to
be safely powered down while not in use.

An example design for this configuration is [provided
here](https://git.sr.ht/~douardda/ESP32-GPIB-pcb)

Unless some form of shield or custom design with integral IEEE488 connector is
used, connecting to an instrument will require a 16 core cable and a suitable
IEEE488 connector. This can be salvaged from an old GPIB cable or purchased
from various electronics parts suppliers. Searching for a 'centronics 24-way
connector' sometimes yields better results than searching for 'IEEE 488
connector' or 'GPIB connector'. Details of interface construction and the
mapping of Arduino pins to GPIB control signals and data bus are explained in
the "Building an AR488 GPIB Interface" section of the [AR488-ESP32
Manual](https://douardda.srht.site/ar488-esp32/build.html).

Commands generally adhere closely to the Prologix syntax, however there are
some minor differences, additions and enhancements. Some commands have been
enhanced with additional options and a number of new custom commands have been
added to provide new features that are not found in the standard Prologix
implementation. Details of all commands and features can be found in the
Command Reference section of the [AR488-ESP32
Manual](https://douardda.srht.site/ar488-esp32/commands.html).

Building the firmware image is using the [platformio](https://platformio.org/)
development environment.

Using the cli flavor of platformio, once installed, building the firmware for a
generic ESP32 WRover board is a matter of:

```
AR488-ESP32$ pio run -e esp32dev
[...]
================================ [SUCCESS] Took 3.39 seconds ================================

Environment    Status    Duration
-------------  --------  ------------
esp32dev       SUCCESS   00:00:03.386
================================ 1 succeeded in 00:00:03.386 ================================
```

Uploading is a matter of using the command with the extra `-t upload` option:

```
AR488-ESP32$ pio run -e esp32dev -t upload
```

Once uploaded, the firmware should respond to the ``++ver`` command with its
version information.

Please see the
[documentation](https://douardda.srht.site/ar488-esp32/configuration.html) for
more details on the configuration and compilation of the firmware for a given
ESP32 board.

## Wireless Communication

Depending on the ESP32 board you are using, it may come with support for wifi,
bluetooth or both. You can enable or disable support for wireless communication
using compile-time options defined in the ``platformio.ini`` file.

See this [documentation](https://douardda.srht.site/ar488-esp32/bluetooth.html)
for more details on how to use wireless communication with the AR488-ESP32.

## Obtaining support

In the event that a problem is found, this can be logged via the Issues feature
on the AR488-ESP32 sourcehut page. Please provide at minimum:

- the firmware version number
- the type of board being used
- the make and model of instrument you are trying to control
- a description of the issue including;
- what steps are required to reproduce the issue

## Acknowledgements

| ---- | ----- |
| Emanuelle Girlando | Original project for the Arduino Uno |
| Luke Mester | Testing of original Uno/Nano verions against Prologix |
| Artag | Porting to the Arduino Micro (32u4) board |
| Tom DG8SAQ | Plotting and printing |
| Twilight-Logic | Author and maintainer of the AR488 project |

Also, thank you to all the contributors to the AR488 EEVblog thread for their
suggestions and support.

The original work by Emanuele Girlando is found here:
http://egirland.blogspot.com/2014/03/arduino-uno-as-usb-to-gpib-controller.html
