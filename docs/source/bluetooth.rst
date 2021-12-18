.. _Remote Connection:

===================
 Remote Connection
===================

Bluetooth
=========

ESP32
-----

When using an ESP32 board (but not an ESP32S2), you can build the firmware with suport
for Bluetooth.

In this case, you can use both the main serial interface and the serial over Bluetooth
to connect to the AR488.

Configuration
+++++++++++++

To enable support for Bluetooth, you only need to define the `AR488_BT_ENABLE`
compilation flag. You can either define it in the `AR488_Config.h` file, or, if using
platformio to build the firmware, simply set it in the `platformio.ini` file in the
board's section like:

.. code-block:: ini

   [env:ttgo-t8-161]
   extends = esp32
   board = esp32dev
   build_flags =
       ${esp32.build_flags}
       -D AR488_BT_ENABLE
       -D BOARD_HAS_PSRAM
       -D SN7516X -D SN7516X_TE=4 -D SN7516X_DC=36
       -D DIO1=34 -D DIO2=35 -D DIO3=32  -D DIO4=33
       -D DIO5=25 -D DIO6=26 -D DIO7=27  -D DIO8=14
       -D REN=12  -D IFC=13  -D NDAC=22  -D NRFD=19
       -D DAV=23  -D EOI=18  -D ATN=5    -D SRQ=2

Desktop Configuration
+++++++++++++++++++++

On a Windows machine, XXX

On Linux, once paired with the AR488, you may need to attach it as a `rfcomm` serial
device. For example:

.. code-block:: bash

   $ bluetoothctl
   [bluetooth]# scan on
   Discovery started
   [CHG] Controller A0:C5:89:B5:94:78 Discovering: yes
   [CHG] Device 7C:9E:BD:F8:1B:36 RSSI: -44
   [bluetooth]# info 7C:9E:BD:F8:1B:36
   Device 7C:9E:BD:F8:1B:36 (public)
      Name: AR488-BT
	  Alias: AR488-BT
	  Class: 0x0002c110
	  Icon: computer
	  Paired: yes
	  Trusted: no
	  Blocked: no
	  Connected: no
	  LegacyPairing: no
	  UUID: Serial Port               (00001101-0000-1000-8000-00805f9b34fb)
	  RSSI: -44
   [bluetooth]# ^D
   $ sudo rfcomm bind hci0 7C:9E:BD:F8:1B:36
   7C:9E:BD:F8:1B:36

From there, ou have a serial console on `/dev/rfcomm0` you can use to connect to the
AR488. For example using platformio's `pio device monitor` command:

.. code-block:: bash

   $ pio device monitor -p /dev/rfcomm0 -b 115200 --echo
   legacy Click
   --- Available filters and text transformations: colorize, debug, default, direct,
   hexlify, log2file, nocontrol, printable, send_on_enter, time
   --- More details at http://bit.ly/pio-monitor-filters
   --- Miniterm on /dev/rfcomm0  115200,8,N,1 ---
   --- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
   > ++addr
   1
   > ++addr 9
   Set device primary address to: 9
   > ++eor 1
   Set EOR to: 1
   > ++auto 3
   Auto mode: 3 (continuous)
   WARNING: automode ON can cause some devices to generate
            'addressed to talk but nothing to say' errors
   > *idn?
   HEWLETT-PACKARD,34970A,0,8-1-2
   > ^C
   $



Arduino
-------

If using a classic Arduino board, you need an HC05 serial-to-BT adapter. For basic
Arduino boards (having a single UART port) this will be connected to the main (hardware)
serial port, in which case ou will not be able to have both the Bluetooth connection and
the direct serial one (it's still possible to configure the AR488 to use a software
serial port for the HC05 module but this comes with limitations, espectially in transfer
speed).

If your Arduino at least two hardware serial ports, you can have both the HC05 and the
main (USB) serial connection active.

Setup
-----

XXX



Wifi
====

ESP32
-----

ESP32 boards come with support for wifi.


Configuration
+++++++++++++

To enable support for Wifi, you only need to define the `AR488_WIFI_ENABLE`
compilation flag. You can either define it in the `AR488_Config.h` file, or, if using
platformio to build the firmware, simply set it in the `platformio.ini` file in the
board's section like:

.. code-block:: ini

   [env:ttgo-t8-161]
   extends = esp32
   board = esp32dev
   build_flags =
       ${esp32.build_flags}
       -D AR488_WIFI_ENABLE
       -D BOARD_HAS_PSRAM
       -D SN7516X -D SN7516X_TE=4 -D SN7516X_DC=36
       -D DIO1=34 -D DIO2=35 -D DIO3=32  -D DIO4=33
       -D DIO5=25 -D DIO6=26 -D DIO7=27  -D DIO8=14
       -D REN=12  -D IFC=13  -D NDAC=22  -D NRFD=19
       -D DAV=23  -D EOI=18  -D ATN=5    -D SRQ=2

The configuration of the Wifi connection credentials is made using the `++wifi` commands
from the main serial connection:


.. code-block:: bash

   $ pio device monitor -p /dev/ttyUSB0 -b 115200 --echo
   > ++wifi scan
   scan done
   1 networks found
   1: My-AP (-57)*

   > ++wifi ssid My-AP
   > ++wifi passkey mypassphrase
   > ++wifi connect
   Connecting to Wifi: My-AP
   [E][WiFiMulti.cpp:187] run(): [WIFI] Connecting Failed (6).
   10
   9
   8

   WiFi connected IP address: 192.168.1.22
   >

Once connected, you can access the AR488 using a simple telnet connection on the port
23:

.. code-block:: bash

   $ telnet 192.168.1.22 23
   Trying 192.168.1.22...
   Connected to 192.168.1.22.
   Escape character is '^]'.
   > ++addr
   9
   > *idn?
   HEWLETT-PACKARD,34970A,0,8-1-2


You can save te wifi connection credentials in the EEPROM using the `++savecfg` command.

Note that the AR488 will not connect by default to the wifi on startup. If you want the
AR488 to automatically connect to the wifi, add the `++wifi connect` command in the
`Macro 0` (see :ref:`macros` for more details).

Serial over IP
==============

Using raw socket
----------------

If the AR488 is connected to the wifi, you test the connection using directly `pio
device monitor`:

.. code-block:: bash

   $ pio device monitor --echo -p socket://192.168.2.151:23

Using raw socket from a linux machine
-------------------------------------

Use `ser2net` on the remote machine side:

.. code-block:: bash

   $ ser2net -n -C 3333:raw:0:/dev/ttyUSB0:115200 8DATABITS NONE 1STOPBIT

Use `socat` on the user side:

.. code-block:: bash

   $ socat pty,link=$HOME/tty,waitslave tcp:<REMOTE.MACHINE>:3333

Using RFC2217
-------------

.. code-block:: bash

   $ ser2net -n -C '3333:telnet:0:/dev/ttyUSB0:115200 8DATABITS NONE 1STOPBIT remctl'

and

.. code-block:: bash

   $ pio device monitor --echo -p rfc2217://beaglebone.lan:3333?ign_set_control -b 115200
