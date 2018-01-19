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

