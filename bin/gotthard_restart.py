#!/usr/bin/env python

__author__ = "andrea.parenti@xfel.eu"
__date__ = "September 19, 2017,  1:12 PM"
__copyright__ = "Copyright (c) European XFEL GmbH Hamburg."
"All rights reserved."

import sys
import telnetlib
import time

if len(sys.argv) < 2:
    print("This script will restart the gotthardDetectorServer on the given host.")
    print("Usage: {} <hostname>".format(sys.argv[0]))
    sys.exit(0)

hostname = sys.argv[1]
process = "/gotthardDetectorServer -phaseshift 55"
timeout = 5

tn = telnetlib.Telnet(hostname, 23, timeout)
print("Connected to {}".format(hostname))
tn.write("ps\r\n".encode("ascii"))

while True:
    # Read "ps" output line-by-line
    l = tn.read_until("\r\n".encode("ascii"), 1).decode('ascii')

    if len(l) == 0:
        # No more "ps" lines
        print("Process '{}' not found on {}".format(process, hostname))
        break

    if process not in l:
        # Not the desired process
        continue

    pid = l.split()[0]
    cmnd = "kill {}\r\n".format(pid)
    print("Executing '{}' on {}".format(cmnd.strip(), hostname))
    tn.write(cmnd.format(pid).encode("ascii"))
    time.sleep(1)
    break
