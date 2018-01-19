/*
 * $Id: JungfrauDetector.cc 12381 2014-01-15 16:27:07Z parenti $
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on March 23, 2016,  3:39 PM
 *
 * Copyright (c) 2010-2016 European XFEL GmbH Hamburg. All rights reserved.
 */

#include "JungfrauControl.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<CameraFsm>, SlsControl, JungfrauControl)

    JungfrauControl::JungfrauControl(const Hash& config) : SlsControl(config) {
    }

    JungfrauControl::~JungfrauControl() {
    }

    void JungfrauControl::expectedParameters(Schema& expected) {

        OVERWRITE_ELEMENT(expected).key("detectorType") // From base class
                .setNewDefaultValue("Jungfrau+")
                .setNewOptions("Jungfrau+")
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

    }
    
    void JungfrauControl::powerOn() {
	sendConfiguration("powerchip", "1");
    }

    const char* JungfrauControl::getCalibrationString() const {
    	return "227 5.6\n";
    }

    const char* JungfrauControl::getSettingsString() const {
       return "VDAC0 1220\nVDAC1 3000\nVDAC2 1053\nVDAC3 1450\nVDAC4 750\nVDAC5 1000\nVDAC6 480\nVDAC7 420\n";
    }

} /* namespace karabo */
