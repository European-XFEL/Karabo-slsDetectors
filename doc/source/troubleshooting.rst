.. _slsDetectors-troubleshooting:

Troubleshooting
===============

The control device is in UNKNOWN state
--------------------------------------

This means that the Karabo device cannot connect to the detector.
You should check that

* the detector is connected to the network,
* and it is powered.

A possible way to verify that the detector is online is to login
to the control server and use the ``ping`` command [#]_.

Detector power can often be controlled via a Beckhoff digital output
device, with the same domain name as the detector, but different type and
member.
For example to the detector ``SA1_XTD9_HIREX/DET/GOTTHARD`` correspond
the Beckhoff devices ``SA1_XTD9_HIREX/DCTRL/GOTTHARD_MASTER_POWER`` and
``SA1_XTD9_HIREX/DCTRL/GOTTHARD_SLAVE_POWER``, which can be used to power on
and off the Gotthard's master and slave.

If the detector is online but still in UNKNOWN state, it can be that the
server software on the detector is not running. In this you can can try to
reboot the detectors micro-controller as explained in the `next
<slsDetectors-reboot_>`_ Section.

.. _slsDetectors-reboot:

Rebooting the detector
----------------------

If the detector is online but not working properly, it can be restarted by
connecting to it. For example, if ``192.168.194.82`` is the IP of module
to be restarted:

.. code-block:: bash

    telnet 192.168.194.82

and then execute:

.. code-block:: bash

    reboot

This command will restart the micro-controller, without the need of a complete
power cycle of the detector.

The control device is in ERROR state
------------------------------------

The control device will go to ERROR state if the receiver device(s) is (are)
not running. This can happen because the instantiation sequence was not
correct, or because the receiver device(s) went down.

The correct starting sequence is

* start receiver device(s) first;
* then start the control device.

To recover for the ERROR, please make sure that the receiver device(s) are
online, then `reset` the control device.


The control device is in INIT state
-----------------------------------

This usually means that the detector is online but cannot be configured.
Try to reboot is as described in `this <slsDetectors-reboot_>`_ Section.

The receiver device is in ERROR state
-------------------------------------

This usually means that the RX TCP port used by this Karabo device, is
already in use.

The control device is ACQUIRING but receiver's `Frame Rate In` is 0
-------------------------------------------------------------------

If you are in external trigger mode (`Timing Mode` = `trigger`), it is
possible that the detector receives no trigger signal.
You can test it by setting the trigger mode to internal (`auto`).

`Frame Rate Out` is not 0, but no images are visible in the GUI
---------------------------------------------------------------

First check that the flag `onlineDisplayEnable` on the receiver device is
enabled.

If the flag is set to ``True``, then it could be that the GUI server is
malfunctioning.
In case there is a second GUI server available for the topic, try to switch
to that one.

`Frame Rate Out` is not 0, but the DAQ does not save any data
-------------------------------------------------------------

Check that in the `DAQ Output` node the `hostname` is set to the
IP address of the 10 GbE interface dedicated to the DAQ.

The receiver device prints out TCP socket errors
------------------------------------------------

If the receiver device logs messages like

.. code-block::

  - 17:32:48.020 ERROR: TCP socket read 0 bytes instead of 4 bytes
  - 17:32:48.020 ERROR: TCP socket sent 0 bytes instead of 1000 bytes
  - 17:32:48.020 ERROR: Accept failed

they can be safely ignored.

This is because the control device, in order to check that the receiver is
online, opens a TCP connection to it. The receiver complains as no data is
exchanged before the connection is closed.

This is a periodic check repeated every 20 s.

If nothing else helps...
------------------------

In order to cleanly restart the system, follow these steps:

* shutdown the Karabo devices and the servers;
* power off the detector;
* optionally execute ``sls_detector_get free`` on the control server
  (this step should not be needed, as this "free" action is done
  by the Karabo control device);
* power up the detector;
* instantiate the receiver device(s);
* instantiate the control device.

.. [#] https://linux.die.net/man/8/ping
