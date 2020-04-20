/*
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on March 23, 2016,  3:39 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifdef SLS_SIMULATION
#include <slssimulation/sls_simulation_defs.h>
#endif

#include "JungfrauControl.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SlsControl, JungfrauControl)

    JungfrauControl::JungfrauControl(const Hash& config) : SlsControl(config) {
#ifdef SLS_SIMULATION
        m_detectorType = static_cast<int>(detectorType::JUNGFRAU);
#endif
    }

    JungfrauControl::~JungfrauControl() {
    }

    void JungfrauControl::expectedParameters(Schema& expected) {

        // Arbitrary, but must be not in use in the same subnet
        VECTOR_STRING_ELEMENT(expected).key("detectorMac")
                .alias("detectormac")
                .tags("sls")
                .displayedName("detectorMac")
                .description("Detector MAC. Arbitrary (e.g. 00:aa:bb:cc:dd:ee), but must be not in use in the same subnet.")
                .assignmentOptional().defaultValue({})
                .commit();

        OVERWRITE_ELEMENT(expected).key("settings") // From base class
                .setNewDefaultValue("dynamicgain")
                .setNewOptions("dynamicgain dynamichg0 fixgain1 fixgain2 forceswitchg1 forceswitchg2")
                .commit();

        OVERWRITE_ELEMENT(expected).key("exposureTime") // From base class
                .setNewMaxInc(0.001)
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

        INT16_ELEMENT(expected).key("storageCells")
                .alias("storagecells")
                .tags("sls")
                .displayedName("Additional Storage Cells")
                .description("Number of additional storage cells. "
                "For very advanced users only!")
                .assignmentOptional().defaultValue(0)
                .minInc(0).maxInc(15)
                .reconfigurable()
                .expertAccess()
                .allowedStates(State::ON)
                .commit();

        INT16_ELEMENT(expected).key("storageCellStart")
                .alias("storagecell_start")
                .tags("sls")
                .displayedName("Storage Cell Start")
                .description("First storage cell to be used. "
                "For very advanced users only!")
                .assignmentOptional().defaultValue(15)
                .minInc(0).maxInc(15)
                .reconfigurable()
                .expertAccess()
                .allowedStates(State::ON)
                .commit();

       UINT32_ELEMENT(expected).key("rxUdpSocketSize")
                .alias("rx_udpsocksize")
                .tags("sls")
                .displayedName("Receiver(s) UDP Socket Size")
                .description("For very advanced users only!")
                .assignmentOptional().defaultValue(1048576000)
                .minInc(104857600).maxInc(1048576000)
                .reconfigurable()
                .expertAccess()
                .allowedStates(State::ON)
                .commit();
    }

    void JungfrauControl::powerOn() {
        sendConfiguration("powerchip", "1");
        sendConfiguration("reg", "4d 0x108000"); // set additional read-out timeout (bit 15)
    }

    const char* JungfrauControl::getCalibrationString() const {
        return "227 5.6\n";
    }

    const char* JungfrauControl::getSettingsString() const {
        return "VDAC0 1220\nVDAC1 3000\nVDAC2 1053\nVDAC3 1450\nVDAC4 750\nVDAC5 1000\nVDAC6 480\nVDAC7 420\n";
    }

} /* namespace karabo */
