/*
 * $Id: SlsReceiver_App.cc 12304 2013-12-19 11:01:35Z parenti $
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on December, 2013, 12:01 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/karabo.hpp>

using namespace std;
using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::log;
using namespace karabo::core;


int main(int argc, char** argv) {

    // Activate the Logger (the threshold priority can be changed to INFO, WARN or ERROR)
    Logger::configure(Hash("priority", "DEBUG"));

    // Create an instance of the device
    BaseDevice::Pointer d = BaseDevice::create("SlsReceiver", Hash("deviceId", "Test_SlsReceiver_0"));

    // Run the device
    d->run(); // Will block

    return (EXIT_SUCCESS);
}
