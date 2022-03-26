# ESP32-488 ESP32/Arduino based GPIB Interface


The ESP32-488 GPIB controller is an
[ESP32-based](https://www.espressif.com/en/products/socs/esp32) controller for
interfacing with IEEE488 GPIB devices via USB using the Arduino developement
environment. This project is afriendly fork from the
[AR488](https://github.com/Twilight-Logic/AR488) project aiming at supporting the ESP32
family platform and improving the source code of the firmware. The original AR488 itself
was inspired by and has been based on the work originally released by Emanuele Girlando.

This sketch represents a major refactoring of the orignal AR488 source code.

It implements the full set of Prologix ++ commands in both controller and device mode.
Secondary GPIB addressing is not yet supported. A number of additional features are
provided, for example, a macro feature is provided to allow automation of frequently
used command sequences was well as controller and instrument initialisation at startup.
Interfacing with SN75160 and SN75161 GPIB transceiver integrated circuits is supported.

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
| 328p | Uno R3 |Single UART shared with USB | Layout as per original project by Emanuelle Girlando |
| 328p | Nano | USB/Single UART shared with USB | Identical to Uno |
| 32u4 | Micro | USB/CDC+1 UART | Compact layout by Artag, designed for his back-of-IEEE488-plug adapter board< |
| 32u4 | Leonardo R3 | USSB/CDC+1 UART | Identical to UNO |
| 2560 | Mega 2560 | 4 x UART, Serial0 shared with USB | D - (default) using pins on either side of board<br>E1 - using the first row of end connector<br>E2 - using the second row of end connector</td></tr>

Generally speaking, any ESP32 based board should work. When choosing a layout, pay
attention to pins which hae special purpose on the ESP32 as well as pins that are inputs
only.

Including the SN7516x chipset into the interface design will naturally add to the cost,
but has the advantage of providing the full 48mA drive current capacity regardless of
the capability of the board being used, as well as providing proper tri-state
output with Hi-Z when the board is powered down. The latter isolates the Arduino
micro-controller from the GPIB bus when the interface is powered down, preventing GPIB
bus communication problems due to 'parasitic power' from signals present on the GPIB
bus, thereby allowing the interface to be safely powered down while not in use.

An example design for this configuration is [provided here]()

To use the sketch, create a new directory, and then unpack the .zip file into this
location. Open the main sketch, AR488.ino, in the Arduino IDE. This should also load all
of the linked .h and .cpp files. Review Config.h and make any configuration adjustment
required (see the 'Configuration' section of the AR488 manual for details), including
the selcetion of the board layout selection appropriate to the Arduino board that you
are using. Set the target board in Board Manager within the Arduino IDE (Tools =>
Board:), and then compile and upload the sketch. There should be no need to make any
changes to any other files. Once uploaded, the firmware should respond to the ++ver
command with its version information.

Please note that Arduino Micro (and other 32u4 boards, e.g. Leonardo) do not
automatically reset when a connection is made to the serial port. The Arduino IDE takes
care of the programming process via USB which should work normally. Some Micro boards
may not have a reset button, in which case the reset pin need to be briefly shorted to
ground by some other means. When using the Arduino IDE on Linux (Linux Mint and possibly
other Ubuntu derivatives), the modemmanager service must be disabled, otherwise it will
interfere with the programming process and the boards will be rendered inaccessible via
USB. If this curers, then the board can be returned to normal working by uploading a
bootloader to it using an AVR programmer. This issue does not seem to affect Uno, Nano
or Mega 2560 boards.

Unless some form of shield or custom design with integral IEEE488 connector is used,
connecting to an instrument will require a 16 core cable and a suitable IEEE488
connector. This can be salvaged from an old GPIB cable or purchased from various
electronics parts suppliers. Searching for a 'centronics 24-way connector' sometimes
yields better results than searching for 'IEEE 488 connector' or 'GPIB connector'.
Details of interface construction and the mapping of Arduino pins to GPIB control
signals and data bus are explained in the "Building an AR488 GPIB Interface" section of
the <a href="https://github.com/Twilight-Logic/AR488/blob/master/AR488-manual.pdf">AR488
Manual</a>.

Commands generally adhere closely to the Prologix syntax, however there are some minor
differences, additions and enhancements. For example, due to issues with longevity of
the Arduino EEPROM memory, the ++savecfg command has been implemented differently to
save EEPROM wear. Some commands have been enhanced with additional options and a number
of new custom commands have been added to provide new features that are not found in the
standard Prologix implementation. Details of all commands and features can be found in
the Command Reference section of the <a
href="https://github.com/Twilight-Logic/AR488/blob/master/AR488-manual.pdf">AR488
Manual</a>.

Once uploaded, the firmware should respond to the ++ver command with its version
information.

<b><i>Wireless Communication:</i></b>

The 32u4 and mega 2560 boards have additional serial ports which can be used to connect the ESP8266 WiFi add-on or the HC05 bluetooth module. The firmware sketch supports auto-configuration of the Bluetooth HC05 module, the details of which can be found in the <a href="https://github.com/Twilight-Logic/AR488/blob/master/AR488-Bluetooth.pdf">AR488 Bluetooth Support</a> supplement. It is also possible to use a HC06 module, but since this module is capable of operating in slave mode only, automatic configuration is not possible. It will therefore need to be configured manually.

Using these wireless modules in conjunction with the Uno or Nano is not advised as the only available serial UART is also used for USB communication. Serial protocols were not designed to accomodate multiple devices on a single UART. Communication problems may arise when both USB and a serial device on RX0/TX0 are connected and communicating with the MCU at the same time. It is possible instead to use SoftwareSerial (TX = pin 6, RX = pin 13) although at a speed of no more than 57600 baud.

The ESP32 is not supported as yet, but work is progressing to add this to the list of supported boards.

<b><i>Obtaining support:</i></b>

In the event that a problem is found, this can be logged via the Issues feature on the AR488 GitHub page. Please provide at minimum:

- the firmware version number
- the type of board being used
- the make and model of instrument you are trying to control
- a description of the issue including;
- what steps are required to reproduce the issue

Comments and feedback can be provided here:<BR>
https://www.eevblog.com/forum/projects/ar488-arduino-based-gpib-adapter/

<b><i>Acknowledgements:</i></b>
<table>
<tr><td>Emanuelle Girlando</td><td>Original project for the Arduino Uno</td></tr>
<tr><td>Luke Mester</td><td>Testing of original Uno/Nano verions against Prologix</td></tr>
<tr><td>Artag</td><td>Porting to the Arduino Micro (32u4) board</td></tr>
<tr><td>Tom DG8SAQ</td><td>Plotting and printing</td></tr>
</table>

Also, thank you to all the contributors to the AR488 EEVblog thread for their suggestions and support.

The original work by Emanuele Girlando is found here:<BR>
http://egirland.blogspot.com/2014/03/arduino-uno-as-usb-to-gpib-controller.html
