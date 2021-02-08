.. _slsReceiver-setup:

Receiver Device Setup
=====================

The following parameter is needed if multiple receivers are run on the same
control host:

* `rxTcpPort`: the port on which the receiver will wait for reconfiguration
  from the control device. It corresponds to the `--rx_tcpport` or `-t`
  option in the `slsReceiver` from PSI. It must be unique on the control
  host.


.. _slsControl-setup:

Control Device Setup
====================

The following are mandatory parameters for the control device.
As one single device can control several detectors, all the parameters
are vectors.
Also, in parenthesis the corresponding parameter name in PSI's tools is
provided:

* `detectorHostName` (`hostname`): the hostnames or IP addresses of the
  detector modules to be controlled;
* `udpSrcIp` (`udp_srcip`): the IP addresses of the detector (source) UDP
  interfaces; must be in the same subnet as the destination UDP/IP; used to be
  named `detectorIp` (`detectorip`);
* `rxHostname` (`rx_hostname`): the hostnames or IP addresses of the hosts
  where the receiver devices are running;
* `rxTcpPort` (`rx_tcpport`): the port on which the receiver for the detector
  has been started; see the `rxTcpPort` paramter of the Karabo receiver.
* `udpDstIp` (`udp_dstip`): the IP addresses of the network interfaces on the
  receiver's host, which is receiving data from the detector; used to be named
  `rxUdpIp` (`rx_udpip`);
* `udpDstPort` (`udp_dstport`): the port numbers of the receiver (destination)
  UDP interfaces; default is 50001; it must be unique if multiple receivers are
  run on the same host; used to be named `rxUdpPort` (`rx_udpport`).
