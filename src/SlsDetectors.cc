/*
 * Author: <parenti>
 *
 * Created on January, 2018, 03:20 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "SlsDetectors.hh"

using namespace std;

USING_KARABO_NAMESPACES;

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SlsDetectors)

    void SlsDetectors::expectedParameters(Schema& expected) {
    }


    SlsDetectors::SlsDetectors(const karabo::util::Hash& config) : Device<>(config) {
    }


    SlsDetectors::~SlsDetectors() {
    }


    void SlsDetectors::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
    }


    void SlsDetectors::postReconfigure() {
    }
}
