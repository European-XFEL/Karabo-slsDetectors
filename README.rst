*****************
SlsControl Device
*****************

Update to SLS Detector Software v5.0.1
======================================

In the SLS detector software, some of the parameters changed from v4.0.1 to
v5.0.1. The Karabo device followed these changes, thus some of the keys have
been renamed:

=================  =================
Old Key Name       New Key Name
=================  =================
bitDepth           dynamicRange
detectorIp         udpSrcIp
detectorMac        udpSrcMac
detectorNumber     serialNumber
detectorVersion    firmwareVersion
numberOfCycles     numberOfTriggers
rxUdpIp            udpDstIp
rxUdpPort          udpDstPort
softwareVersion    detServerVersion
thisVersion        clientVersion
vHighVoltage       highVoltage
vHighVoltageMax    highVoltageMax
=================  =================


The following parameters have been removed, as not available any more, or not
available for the detectors we currently have at XFEL:

+-------------------------+
| Removed Key Name        |
+=========================+
| angConv                 |
+-------------------------+
| angDir                  |
+-------------------------+
| binSize                 |
+-------------------------+
| detectorDeveloper       |
+-------------------------+
| flatFieldCorrectionFile |
+-------------------------+
| globalOff               |
+-------------------------+
| lock                    |
+-------------------------+
| master                  |
+-------------------------+
| maximumDetectorSize     |
+-------------------------+
| moveFlag                |
+-------------------------+
| numberOfGates           |
+-------------------------+
| online                  |
+-------------------------+
| sync                    |
+-------------------------+
| threaded                |
+-------------------------+


For some other properties the unit and type have changed:

=================  ========  ========  ========  ========
Key Name           Old Unit  New Unit  Old Type  New Type
=================  ========  ========  ========  ========
exposureTime       s         ns        FLOAT     INT64
exposurePeriod     s         ns        FLOAT     INT64
delayAfterTrigger  s         ns        FLOAT     INT64
=================  ========  ========  ========  ========

