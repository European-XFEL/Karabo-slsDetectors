.. _slsDetectors-deployment:

Deployment Guidelines
=====================

`slsDetectors` will automatically install its dependency
`slsDetectorsPackage`.

For debugging purposes it can be useful to have ``tshark`` installed on the
control server, and ``xctrl`` user added to the ``wireshark`` group.

The control server should have a 10 GbE network interface for sending images
to the DAQ, and possibly one for the GUI server.


Special settings for Jungfrau
-----------------------------

In order to have the receiving thread executed with real time priority,
the line

.. code-block:: bash

    username rtprio 99

shall be added to the file ``/etc/security/limits.conf`` on the control
server (may differ depending on the Linux distribution).

The RX socket buffer size shall be set to 1000 MB

.. code-block:: bash

    sysctl -w net.core.rmem_max=1048576000

The maximum socket input packet queue shall be set to 250000

.. code-block:: bash

    sysctl -w net.core.netdev_max_backlog=250000


The MTU on the network interface used for Jungfrau shall be set to 9000,
also known as "jumbo frames".
