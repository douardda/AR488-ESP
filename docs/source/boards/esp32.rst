=======
 ESP32
=======


Connection
==========

TODO

Bluetooth
=========

On linux, you may have to use cli tools.

Pairing using `bluetoothctl`:

.. code-block:: bash

   $ sudo bluetoothctl
   Agent registered
   [bluetooth]# devices
   Device 10:52:1C:5B:1F:DE AR488-BT
   [bluetooth]# pair 10:52:1C:5B:1F:DE
   [bluetooth]# trust 10:52:1C:5B:1F:DE
   [bluetooth]# info 10:52:1C:5B:1F:DE
   Device 10:52:1C:5B:1F:DE (public)
       Name: AR488-BT
	   Alias: AR488-BT
	   Class: 0x0002c110
	   Icon: computer
	   Paired: yes
	   Trusted: yes
	   Blocked: no
	   Connected: no
	   LegacyPairing: no
	   UUID: Serial Port               (00001101-0000-1000-8000-001111111111)


Then you have to bind a tty to the BT controller:

.. code-block:: bash

   $ sudo rfcomm bind hci0 10:52:1C:5B:1F:DE
