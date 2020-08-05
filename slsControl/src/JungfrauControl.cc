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

       UINT32_ELEMENT(expected).key("exposureTimeout")
                .displayedName("Exposure Timeout")
                .description("In burst acquisition mode, the time interval between two consecutive "
                "exposures can be tuned with this timeout. "
                "Note that the time interval between consecutive exposures is also determined by the "
                "operation of the ASIC control FSM, as well as by the pre-charger and DS timeouts. "
                "The exposure timeout t_{ET} increases the range of the timeout betweeen two "
                "consecutive storage cells.")
                .assignmentOptional().defaultValue(25)
                .minInc(25).maxInc(1000000)
                .unit(Unit::SECOND).metricPrefix(MetricPrefix::NANO)
                .reconfigurable()
                .expertAccess()
                .allowedStates(State::ON)
                .commit();

       UINT16_ELEMENT(expected).key("exposureTimer")
                .displayedName("Exposure Timer")
                .description("This value is obtained from the exposure timeout as: ET = t_ET / 25 ns - 1.")
                .readOnly()
                .expertAccess()
                .commit();
    }

    void JungfrauControl::powerOn() {
        sendConfiguration("powerchip", "1");
        sendConfiguration("reg", "4d 0x108000"); // set additional read-out timeout (bit 15)
    }

    void JungfrauControl::configureDetectorSpecific(const karabo::util::Hash& configHash) {
        if (configHash.has("exposureTimeout")) {
            // Must set bits 16-31 of 0x7F (ASIC_CTRL) register
            const unsigned int exposureTimeout = configHash.get<unsigned int>("exposureTimeout");
            const unsigned short exposureTimer = std::lround(exposureTimeout/25) - 1;
            this->set("exposureTimer", exposureTimer); // set read-only device parameter

            char* args[3];
            args[0] = const_cast<char*>("reg");
            args[1] = const_cast<char*>("7F");

            for (size_t i = 0; i < m_numberOfModules; ++i) {
                // Get current value of ASIC_CTRL
                std::string reply = m_SLS->getCommand(2, args, i);
                unsigned int asicCtrl = std::stoul(reply, nullptr, 16);

                // Replace bits 16-31
                asicCtrl = (asicCtrl & 0xFFFF) | (exposureTimer << 16);

                // Convert to string
                std::stringstream sstream;
                sstream << std::hex << asicCtrl;
                std::string asicCtrlStr(sstream.str());
                args[2] = const_cast<char*>(asicCtrlStr.c_str());

                // Set the value back
                m_SLS->putCommand(3, args, i);
            }
        }
    }

    const char* JungfrauControl::getCalibrationString() const {
        return "227 5.6\n";
    }

    const char* JungfrauControl::getSettingsString() const {
        return "VDAC0 1220\nVDAC1 3000\nVDAC2 1053\nVDAC3 1450\nVDAC4 750\nVDAC5 1000\nVDAC6 480\nVDAC7 420\n";
    }

} /* namespace karabo */
