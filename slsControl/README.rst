******************************
SlsControl Device (C++)
******************************

Gotthard troubleshooting
========================

It happens sometimes that Gotthard starts sending data and cannot be stopped.
If you click `stop` the Karabo device will be stuck in "CHANGING".
Usually this problem occurs with short exposure time and period.

In this case, it is usually enough to execute the script `gotthard_restart.py`
and then `reset` the Karabo "detector" device.

If it does not help, you may need to:

* Shutdown or `karabo-kill -k` the cppServer
* run the `sls_detector_get free` command on the control server
* `telnet` on the Gotthard and `reboot` it
* restart Karabo servers and devices

Compiling
=========

1. Source activate the Karabo installation against which the device will be
   developed:

    ``cd <Karabo installation root directory>``
    ``source ./activate``

2. Goto the device source root directory and generate its build files with cmake:

     ``cd $KARABO/devices/slsControl``
     ``mkdir build``
     ``cd build``
     ``cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$KARABO/extern ..``

   CMAKE_BUILD_TYPE can also be set to "Release".

3. Build the device:

     ``cd $KARABO/devices/slsControl``
     ``cmake --build . ``

   ``make`` can also be used as long as the Makefile generator is used by cmake.

Testing
=======

After a successfull build, a shared library is generated here:

``dist/<configuration>/<system>/libslsControl.so``

And a soft-link to the ``libslsControl.so`` file is created in the
``$KARABO/plugins`` folder.

To run the tests, go to the tests directory in your build tree and use ``ctest``:

    ``cd $KARABO/devices/slsControl/build/slsControl``
    ``ctest -VV``

Running
=======

If you want to manually start a server using this device, simply type:

``karabo-cppserver serverId=cppServer/1 deviceClasses=SlsControl``

Or just use (a properly configured):

``karabo-start``
