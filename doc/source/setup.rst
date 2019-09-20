.. _slsDetectors-setup:

Receiver Setup
==============

The following parameter is needed if multiple receivers are run on the same
control server:

* `rxTcpPort`: the port on which the receiver will wait for reconfiguration
  from the control device. It corresponds to the `--rx_tcpport` or `-t`
  option in the `slsReceiver` from PSI. It must be unique on the control
  server.

Control Device Setup
====================

The following are mandatory parameters for the control device.
As one single device can control several detectors, all the parameters
are vectors.
Also, in parenthesis I will write the name of the corresponding parameter
in PSI's tools:

* `detectorHostName`: the hostname or IP address of the detector module to
  be controlled (`hostname`);
* `detectorIp`: data produced by the detector will be "marked" with this
  source IP (`detectorip`).
  In some sense this is the IP address of the data interface of the detector;
* `rxHostname`: the hostname or IP of the server receiving data
  (`rx_hostname`);
* `rxTcpPort`: the port on which the receiver for the detector has been
  started (`rx_tcpport`);
* `rxUdpIp`: data produced by the detector will be "marked" with this
  destination IP (`rx_udpip`);
* `rxUdpPort`: the receiver will wait for data on this port; it must be unique
  if multiple receivers are run on the same server (`rx_udpport`).
