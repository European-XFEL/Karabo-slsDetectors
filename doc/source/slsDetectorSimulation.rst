.. _slsDetectorSimulation:

The slsDetectorSimulation
=========================

In this package a subset of the slsDetector package have been reimplemented,
in a simulation mode.


The Receiver Class
------------------

It reimplements part of the functionalities of the same class from the
slsDetectorPackage. In order to use it, you need to include
`slssimulation/Receiver.h` and link `libSlsSimulation.so`.

The `Receiver` class runs a TCP/IP server on the port specified on start-up
by the argument `--rx_tcpport` or `-t`.

The commands known to the `Receiver` TCP/IP server are:

* `start`: to start generating simulated data (till the `stop` command is
  received);
* `stop`: to stop generating data;
* `exptime` [ns]: to set the exposure time parameter;
* `delay` [ns]: to set the delay after trigger parameter;
* `period` [ns]: to set the period parameter;
* `detectortype`: to set the detector type; an integer value must be used; the
  options are defined in `slssimulation/sls_simulation_defs.h`;
* `fpath`: to set the output path;
* `fname`: to set the root name of the output file;
* `findex`: to set the start index of the output file;
* `fwrite`: to enable or disable the output file write;
* `settings`: to select the settings; an integer value must be used; the
  options are defined in `slssimulation/sls_simulation_defs.h`.


The Detector Class
------------------

It reimplements part of the functionalities of the same class from the
slsDetectorPackage. In order to use it, you need to include
`slssimulation/Detector.h` and link `libSlsSimulation.so`.

It currently has the limitation that it can only control one simulated
detector.

The `Detector::start()` function will connect to the `Receiver` TCP/IP
server, configure it and start an acquisition.

The `Detector::stop()` function will stop the acquisition, if still ongoing.

