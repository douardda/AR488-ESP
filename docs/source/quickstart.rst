.. _quickstart

Quickstart
==========

The idea of the Prologix serial communication protocol is to use a special
syntax for control commands of the AR488-ESP32 interface. All other
communication stream sent to the AR488-ESP32 will be forwarded (almost) as is
(see below) to the adressed GPIB device, if any.

Any received data stream from the adressed device will be similarly sent the
computer host's serial interface.

The complete list of special `++` commands is described in the next
:ref:commands_ section.

Note that the AR488-ESP32 will not echo characters received from the serial
line. So when using the device in an interactive session, make sure to enable
ECHO on your terminal program.

Using the platformio monitor tool:

.. code-block::

   AR488-ESP32$ pio device monitor -b 115200 --echo
   --- Terminal on /dev/ttyUSB0 | 115200 8-N-1
   --- Available filters and text transformations: colorize, debug, default, direct, hexlify, log2file, nocontrol, printable, send_on_enter, time
   --- More details at https://bit.ly/pio-monitor-filters
   --- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
   > ++ver
   AR488-ESP32 GPIB controller, ver. 0.0.1
   > ++prompt
   Show prompt: ON

In the example above one can see the prompt has been activated, which is useful
for an interactive session (but you probably do not want that for automated
control).

Wifi configuration can be done as well:

.. code-block::

   > ++wifi scan
   scan done
   17 networks found
   1: my-ssid (-55)*
   2: freebox_IDS (-74)*
   3: FreeWifi_secure (-75)*
   4: Livebox-C6A0 (-81)*
   5: SFR_B828 (-82)*
   6: SFR WiFi Mobile (-85)*
   7: SFR_83B0 (-85)*
   8: Bbox-D0AB3948 (-86)*
   9: SFR WiFi FON (-87)
   10: Leo Italie (-90)*
   11: Bbox-A147AE6B (-90)*
   12: SFR_4890 (-91)*
   13: WAAZZZAAAAAAA (-91)*
   14: FreeWifi_secure (-91)*
   15: Bbox-298EAC90 (-92)*
   16: SM (-93)*
   17: FreeWifi_secure (-95)*
   > ++wifi ssid my-ssid
   my-ssid
   > ++wifi passkey my-strong-wifi-password
   > ++wifi connect
   Connecting to Wifi: my-ssid

   WiFi connected IP address: 192.168.1.164
   > ++savecfg
   >


Now let's see available devices on the bus (note that older devices may not be
detected):

.. code-block::

   > ++verbose 1
   Verbose: ON
   > ++addr 4
   Set device primary address to: 4
   > ++spoll
   Got 1 devices to spoll
   Polling 4
   4
   Serial poll completed.
   > *IDN?
   > ++read
   Hewlett-Packard, ESG-D3000B, GB38450564, B.03.86
   > ++verbose 0
   Verbose: OFF
   > ++addr 6
   ++spoll
   0
   > *IDN?
   > ++read
   Agilent Technologies,E3648A,0,1.7-5.0-1.0
   > ++addr 8
   > ++spoll
   0
   > *IDN?
   > ++read
   ADVANTEST,R3465,0,K03
   > ^C
   AR488-ESP32$

You may use Python to write a simple tool to poll the whole bus:

.. code-block:: python

   from serial import Serial
   import time
   cnx = Serial(port='/dev/ttyUSB0', baudrate=115200,
				timeout=0.1)

   cnx.readline()  # just in case
   for i in range(1, 31):
       cnx.write(f"++addr {i}\r\n".encode())
       cnx.readline()
       cnx.write("++spoll\r\n".encode())
       for j in range(10):
           time.sleep(0.05)
           if cnx.readline().decode().strip():
               print("Device at", i, "spoll=", r)
               break

Which would be like:

.. code-block:: bash

   $ python3 detect.py
   Device at 4 spoll= 4
   Device at 6 spoll= 0
   Device at 8 spoll= 0
   Device at 12 spoll= 0
   Device at 24 spoll= 0
   Device at 26 spoll= 0

The ``++auto`` feature is also supported; it atomatically request talk after an
input command that ends with the ``?`` character (note that some devices do not
like it too much):

.. code-block::

   AR488-ESP32$ pio device monitor -b 115200 --echo
   ++prompt 1
   Show prompt: ON
   > ++auto 1
   > ++addr 4
   > *IDN?
   Hewlett-Packard, ESG-D3000B, GB38450564, B.03.86

   > ++addr 26
   > *IDN?
   HP 8904A Opts 02/01/03/05/  /  /  /  /  Firmware Revision 00790A Serial No 06958
   > ^C
   AR488-ESP32$
