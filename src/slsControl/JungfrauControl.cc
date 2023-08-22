/*
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Created on March 23, 2016,  3:39 PM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifdef SLS_SIMULATION
#include "../slsDetectorsSimulation/sls_simulation_defs.h"
#endif

#include "JungfrauControl.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SlsControl, JungfrauControl)


    JungfrauControl::JungfrauControl(const Hash& config) : SlsControl(config) {
        KARABO_SLOT(resetTempEvent);

#ifdef SLS_SIMULATION
        m_detectorType = slsDetectorDefs::detectorType::JUNGFRAU;
#endif
    }


    JungfrauControl::~JungfrauControl() {
        if (m_SLS && !m_SLS->empty()) {
            m_SLS->setPowerChip(false, m_positions); // power off
        }
    }


    void JungfrauControl::expectedParameters(Schema& expected) {
        std::vector<std::string> settingsOptions = {"gain0", "highgain0"};
        OVERWRITE_ELEMENT(expected).key("settings") // From base class
                .setNewDefaultValue("gain0")
                .setNewOptions(settingsOptions)
                .commit();

        std::vector<std::string> gainModeOptions = {"dynamic", "forceswitchg1", "forceswitchg2", "fixg1", "fixg2"};
        // fixg0 shall not be used, it can damage the detector!
        STRING_ELEMENT(expected).key("gainMode")
                .alias("gainmode")
                .tags("sls")
                .displayedName("Gain Mode")
                .description("The Jungfrau gain mode.")
                .assignmentOptional().defaultValue("dynamic")
                .options(gainModeOptions)
                .reconfigurable()
                .commit();

        OVERWRITE_ELEMENT(expected).key("highVoltage")
                .setNewDescription("High voltage to the sensor. "
                "Options: 0|60-200 V.")
                .commit();

        OVERWRITE_ELEMENT(expected).key("exposureTime")
                .setNewMinExc(0.)
                .setNewMaxInc(0.001) // 1 ms
                .commit();

        OVERWRITE_ELEMENT(expected).key("exposurePeriod")
                .setNewMinExc(0.)
                .commit();

        std::vector<std::string> timingOptions = {"auto", "trigger"};
        OVERWRITE_ELEMENT(expected).key("timing")
                .setNewOptions(timingOptions)
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

        VECTOR_INT32_ELEMENT(expected).key("tempAdc")
                .displayedName("ADC Temperature")
                .unit(Unit::DEGREE_CELSIUS)
                .readOnly()
                .commit();

        VECTOR_INT32_ELEMENT(expected).key("tempFpga")
                .displayedName("FPGA Temperature")
                .unit(Unit::DEGREE_CELSIUS)
                .readOnly()
                .commit();

        VECTOR_INT32_ELEMENT(expected).key("tempThreshold")
                .alias("temp_threshold")
                .tags("sls")
                .displayedName("Temperature Threshold")
                .description("The threshold temperature in degrees. If the temperature crosses "
                "the threshold temperature and temperature control is enabled (default is "
                "disabled), the power to chip will be switched off and the temperature event "
                "will be set. "
                "To power on the chip again, the temperature has to be less than the threshold "
                "temperature and the temperature event has to be reset.")
                .assignmentOptional().defaultValue({65})
                .unit(Unit::DEGREE_CELSIUS)
                .reconfigurable()
                .expertAccess()
                .allowedStates(State::DISABLED, State::ON)
                .commit();

        VECTOR_INT32_ELEMENT(expected).key("tempControl")
                .alias("temp_control")
                .tags("sls")
                .displayedName("Temperature Control")
                .description("Set to 1 to enable the temperature control.")
                .assignmentOptional().defaultValue({0})
                .reconfigurable()
                .expertAccess()
                .allowedStates(State::DISABLED, State::ON)
                .commit();

        VECTOR_INT32_ELEMENT(expected).key("tempEventVector")
                .displayedName("Temperature Event (Modules)")
                .description("The temperature event will be 1 if temperature crosses the "
                "threshold and the control is enabled. This vector has one entry per module. "
                "Fix the issue before resetting.")
                .readOnly()
                .expertAccess()
                .commit();

        // Aggregates 'tempEventVector' in a single boolean
        BOOL_ELEMENT(expected).key("tempEvent")
                .displayedName("Temperature Event (Summary)")
                .description("The temperature event will be set to 'true' if temperature crosses "
                "the threshold and the control is enabled. Fix the issue before resetting.")
                .readOnly()
                .alarmHigh(false).info("One of the JF modules exceeded the threshold temperature").needsAcknowledging(false)
                .commit();

        SLOT_ELEMENT(expected).key("resetTempEvent")
                .displayedName("Reset Temperature Event")
                .allowedStates(State::DISABLED)
                .commit();
    }


    void JungfrauControl::resetTempEvent() {
        m_SLS->resetTemperatureEvent(m_positions);
        this->updateState(State::ON);
        this->set("status", "Temperature event reset");
    }


    void JungfrauControl::powerOn() {
        m_SLS->setPowerChip(true, m_positions); // power on
        m_SLS->writeRegister(0x4d, 0x108000, m_positions); // set additional read-out timeout (bit 15)
    }


    void JungfrauControl::pollDetectorSpecific(karabo::util::Hash& h) {
        const std::vector<int> tempAdc = m_SLS->getTemperature(slsDetectorDefs::dacIndex::TEMPERATURE_ADC, m_positions);
        h.set("tempAdc", tempAdc);

        const std::vector<int> tempFpga = m_SLS->getTemperature(slsDetectorDefs::dacIndex::TEMPERATURE_FPGA, m_positions);
        h.set("tempFpga", tempFpga);

        const std::vector<int> tempEventVector = m_SLS->getTemperatureEvent(m_positions);
        h.set("tempEventVector", tempEventVector);
        bool tempEvent = false;
        for (const int evt : tempEventVector) {
            if (evt != 0) {
                tempEvent = true;
                break;
            }
        }
        h.set("tempEvent", tempEvent);

        if (tempEvent && this->getState() != State::DISABLED) {
            this->updateState(State::DISABLED);
            h.set("status", "An over-temperature event occurred");
            KARABO_LOG_ERROR << "An over-temperature event occurred";
        }
    }


    void JungfrauControl::configureDetectorSpecific(const karabo::util::Hash& configHash) {
        if (configHash.has("exposureTimeout")) {
            // Must set bits 16-31 of 0x7F (ASIC_CTRL) register
            const unsigned int exposureTimeout = configHash.get<unsigned int>("exposureTimeout");
            const unsigned short exposureTimer = std::lround(exposureTimeout/25) - 1;
            this->set("exposureTimer", exposureTimer); // set read-only device parameter

            const uint32_t addr = 0x7F; // ASIC_CTRL register address
            const std::vector<uint32_t> asicCtrlVector = m_SLS->readRegister(addr);

            int i = 0;
            for (uint32_t asicCtrl : asicCtrlVector) {
                // Replace bits 16-31
                asicCtrl = (asicCtrl & 0xFFFF) | (exposureTimer << 16);

                m_SLS->writeRegister(addr, asicCtrl, {i++});
            }
        }
    }

} /* namespace karabo */
