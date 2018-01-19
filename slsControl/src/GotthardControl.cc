
/*
 * $Id: GotthardDetector.cc 12381 2014-01-15 16:27:07Z parenti $
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on March 21, 2016,  3:15 PM
 *
 * Copyright (c) 2010-2016 European XFEL GmbH Hamburg. All rights reserved.
 */

#include "GotthardControl.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<CameraFsm>, SlsControl, GotthardControl)

    GotthardControl::GotthardControl(const Hash& config) : SlsControl(config) {
    }

    GotthardControl::~GotthardControl() {
    }

    void GotthardControl::expectedParameters(Schema& expected) {

        OVERWRITE_ELEMENT(expected).key("detectorType") // From base class
                .setNewDefaultValue("Gotthard+")
                .setNewOptions("Gotthard+")
                .commit();

        // Arbitrary, but must be not in use in the same subnet
        STRING_ELEMENT(expected).key("detectorMac")
                .alias("detectormac")
                .tags("sls")
                .displayedName("detectorMac")
                .description("Detector MAC. Arbitrary, but must be not in use in the same subnet.")
                .assignmentOptional().defaultValue("00:aa:bb:cc:dd:ee")
                .commit();

        STRING_ELEMENT(expected).key("rxUdpIp")
                .alias("rx_udpip")
                // No "sls" tag... will be processed differently
                .displayedName("rxUdpIp")
                .description("Receiver UDP IP")
                .assignmentMandatory()
                .commit();

        UINT16_ELEMENT(expected).key("rxUdpPort")
                .alias("rx_udpport")
                .tags("sls")
                .displayedName("rxUdpPort")
                .description("Receiver UDP Port")
                .assignmentOptional().defaultValue(50001)
                .commit();

        UINT16_ELEMENT(expected).key("rxTcpPort")
                .alias("rx_tcpport")
                .tags("sls")
                .displayedName("rxTcpPort")
                .description("Receiver TCP Port")
                .assignmentOptional().defaultValue(1954)
                .commit();

        OVERWRITE_ELEMENT(expected).key("settings") // From base class
                .setNewDefaultValue("dynamicgain")
                .setNewOptions("dynamicgain lowgain mediumgain highgain veryhighgain")
                .commit();

        INT16_ELEMENT(expected).key("rOnline")
                .alias("r_online")
                .tags("sls")
                .displayedName("rOnline")
                .description("rOnline")
                .assignmentOptional().defaultValue(1)
                .options("0 1")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // TODO?
        //        INT16_ELEMENT(expected).key("rLock")
        //                .alias("r_lock")
        //                .tags("sls")
        //                .displayedName("rLock")
        //                .description("rLock")
        //                .assignmentOptional().defaultValue(0)
        //                .options("0 1")
        //                .reconfigurable()
        //                .allowedStates(State::ON)
        //                .commit();
        //        
        //        STRING_ELEMENT(expected).key("receiver")
        //                .alias("receiver")
        //                .tags("sls")
        //                .displayedName("receiver")
        //                .description("receiver")
        //                .assignmentOptional().defaultValue("stop")
        //                .options("stop start")
        //                .reconfigurable()
        //                .allowedStates(State::ON)
        //                .commit();

    }

    const char* GotthardControl::getCalibrationString() const {
    	return "227 5.6\n";
    }

    const char* GotthardControl::getSettingsString() const {
       return "Vcasc 1320\nVcascN 650\nVcascP 1480\nVib_test 2001\nVin 1350\nVout 1520\nVref 660\nVref_comp 887\n";
    }

} /* namespace karabo */
