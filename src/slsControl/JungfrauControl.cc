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

    KARABO_REGISTER_FOR_CONFIGURATION(Device, SlsControl, JungfrauControl)


    JungfrauControl::JungfrauControl(const Hash& config) : SlsControl(config) {
        KARABO_SLOT(resetTempEvent);
        KARABO_SLOT(enableCurrentSource);
        KARABO_SLOT(disableCurrentSource);

#ifdef SLS_SIMULATION
        m_detectorType = slsDetectorDefs::detectorType::JUNGFRAU;
#endif
    }


    void JungfrauControl::expectedParameters(Schema& expected) {
        std::vector<std::string> settingsOptions = {"gain0", "highgain0"};
        OVERWRITE_ELEMENT(expected)
              .key("settings") // From base class
              .setNewDefaultValue("gain0")
              .setNewOptions(settingsOptions)
              .commit();

        std::vector<std::string> gainModeOptions = {"dynamic", "forceswitchg1", "forceswitchg2", "fixg1", "fixg2"};
        // fixg0 shall not be used, it can damage the detector!
        STRING_ELEMENT(expected)
              .key("gainMode")
              .alias("gainmode")
              .tags("sls")
              .displayedName("Gain Mode")
              .description("The Jungfrau gain mode.")
              .assignmentOptional()
              .defaultValue("dynamic")
              .options(gainModeOptions)
              .reconfigurable()
              .allowedStates(State::ON)
              .commit();

        OVERWRITE_ELEMENT(expected)
              .key("highVoltage")
              .setNewDescription(
                    "High voltage to the sensor. "
                    "Options: 0|60-200 V.")
              .commit();

        std::vector<std::string> timingOptions = {"auto", "trigger"};
        OVERWRITE_ELEMENT(expected).key("timing").setNewOptions(timingOptions).commit();

        OVERWRITE_ELEMENT(expected)
              .key("exposureTime")
              .setNewDefaultValue(1.e-5) // 10 us
              .setNewMinExc(0.)
              .setNewMaxInc(0.001) // 1 ms
              .commit();

        OVERWRITE_ELEMENT(expected)
              .key("exposurePeriod")
              .setNewMinExc(0.)
              .setNewDefaultValue(0.1) // 100 ms
              .commit();

        INT16_ELEMENT(expected)
              .key("storageCells")
              .alias("extrastoragecells") // was previously "storagecells"
              .tags("sls")
              .displayedName("Additional Storage Cells")
              .description("Number of additional storage cells.")
              .assignmentOptional()
              .defaultValue(0)
              .minInc(0)
              .maxInc(15)
              .reconfigurable()
              .allowedStates(State::ON)
              .commit();

        INT16_ELEMENT(expected)
              .key("storageCellStart")
              .alias("storagecell_start")
              .tags("sls")
              .displayedName("Storage Cell Start")
              .description("First storage cell to be used.")
              .assignmentOptional()
              .defaultValue(15)
              .minInc(0)
              .maxInc(15)
              .reconfigurable()
              .allowedStates(State::ON)
              .commit();

        UINT32_ELEMENT(expected)
              .key("rxUdpSocketSize")
              .alias("rx_udpsocksize")
              .tags("sls")
              .displayedName("Receiver(s) UDP Socket Size")
              .description("For very advanced users only!")
              .assignmentOptional()
              .defaultValue(1048576000)
              .minInc(104857600)
              .maxInc(1048576000)
              .reconfigurable()
              .expertAccess()
              .allowedStates(State::ON)
              .commit();

        UINT32_ELEMENT(expected)
              .key("exposureTimeout")
              .displayedName("Exposure Timeout")
              .description(
                    "In burst acquisition mode, the time interval between two consecutive "
                    "exposures can be tuned with this timeout. "
                    "Note that the time interval between consecutive exposures is also determined by the "
                    "operation of the ASIC control FSM, as well as by the pre-charger and DS timeouts. "
                    "The exposure timeout t_{ET} increases the range of the timeout betweeen two "
                    "consecutive storage cells.")
              .assignmentOptional()
              .defaultValue(25)
              .minInc(25)
              .maxInc(1000000)
              .unit(Unit::SECOND)
              .metricPrefix(MetricPrefix::NANO)
              .reconfigurable()
              .expertAccess()
              .allowedStates(State::ON)
              .commit();

        UINT16_ELEMENT(expected)
              .key("exposureTimer")
              .displayedName("Exposure Timer")
              .description("This value is obtained from the exposure timeout as: ET = t_ET / 25 ns - 1.")
              .readOnly()
              .expertAccess()
              .commit();

        VECTOR_DOUBLE_ELEMENT(expected)
              .key("chipVersion")
              .displayedName("Chip Version")
              .description("The chip version. It can be 1.0 or 1.1.")
              .readOnly()
              .commit();

        VECTOR_INT32_ELEMENT(expected)
              .key("tempAdc")
              .displayedName("ADC Temperature")
              .unit(Unit::DEGREE_CELSIUS)
              .readOnly()
              .commit();

        VECTOR_INT32_ELEMENT(expected)
              .key("tempFpga")
              .displayedName("FPGA Temperature")
              .unit(Unit::DEGREE_CELSIUS)
              .readOnly()
              .commit();

        VECTOR_INT32_ELEMENT(expected)
              .key("tempThreshold")
              .alias("temp_threshold")
              .tags("sls")
              .displayedName("Temperature Threshold")
              .description(
                    "The threshold temperature in degrees. If the temperature crosses "
                    "the threshold temperature and temperature control is enabled (default is "
                    "disabled), the power to chip will be switched off and the temperature event "
                    "will be set. "
                    "To power on the chip again, the temperature has to be less than the threshold "
                    "temperature and the temperature event has to be reset.")
              .assignmentOptional()
              .defaultValue({65})
              .unit(Unit::DEGREE_CELSIUS)
              .reconfigurable()
              .expertAccess()
              .allowedStates(State::DISABLED, State::ON)
              .commit();

        VECTOR_INT32_ELEMENT(expected)
              .key("tempControl")
              .alias("temp_control")
              .tags("sls")
              .displayedName("Temperature Control")
              .description("Set to 1 to enable the temperature control.")
              .assignmentOptional()
              .defaultValue({0})
              .reconfigurable()
              .expertAccess()
              .allowedStates(State::DISABLED, State::ON)
              .commit();

        VECTOR_INT32_ELEMENT(expected)
              .key("tempEventVector")
              .displayedName("Temperature Event (Modules)")
              .description(
                    "The temperature event will be 1 if temperature crosses the "
                    "threshold and the control is enabled. This vector has one entry per module. "
                    "Fix the issue before resetting.")
              .readOnly()
              .expertAccess()
              .commit();

        // Aggregates 'tempEventVector' in a single boolean
        BOOL_ELEMENT(expected)
              .key("tempEvent")
              .displayedName("Temperature Event (Summary)")
              .description(
                    "The temperature event will be set to 'true' if temperature crosses "
                    "the threshold and the control is enabled. Fix the issue before resetting.")
              .readOnly()
              .commit();

        SLOT_ELEMENT(expected)
              .key("resetTempEvent")
              .displayedName("Reset Temperature Event")
              .allowedStates(State::DISABLED)
              .commit();

        BOOL_ELEMENT(expected)
              .key("currentSourceEnabled")
              .displayedName("Current Source Enabled")
              .readOnly()
              .expertAccess()
              .defaultValue(false)
              .commit();

        NODE_ELEMENT(expected)
              .key("currentSourceSettings")
              .displayedName("Current Source Settings")
              .expertAccess()
              .commit();

        BOOL_ELEMENT(expected)
              .key("currentSourceSettings.fixCurrent")
              .displayedName("Fix Current")
              .assignmentOptional()
              .defaultValue(false)
              .reconfigurable()
              .expertAccess()
              .allowedStates(State::ON)
              .commit();

        UINT64_ELEMENT(expected)
              .key("currentSourceSettings.selectCurrent")
              .displayedName("Select Current")
              .description("The source is 0-63 for chip v1.0 and a 64 bit mask for v1.1.")
              .assignmentOptional()
              .defaultValue(0)
              .reconfigurable()
              .expertAccess()
              .allowedStates(State::ON)
              .commit();

        BOOL_ELEMENT(expected)
              .key("currentSourceSettings.normalCurrent")
              .displayedName("Normal Current")
              .description("Only available on chip v1.1. True: normal current; False: low current.")
              .assignmentOptional()
              .defaultValue(true)
              .reconfigurable()
              .expertAccess()
              .allowedStates(State::ON)
              .commit();

        SLOT_ELEMENT(expected)
              .key("enableCurrentSource")
              .displayedName("Enable Current Source")
              .expertAccess()
              .allowedStates(State::ON)
              .commit();

        SLOT_ELEMENT(expected)
              .key("disableCurrentSource")
              .displayedName("Disable Current Source")
              .expertAccess()
              .allowedStates(State::ON)
              .commit();
    }


    void JungfrauControl::enableCurrentSource() {
        const bool fixCurrent = this->get<bool>("currentSourceSettings.fixCurrent");
        const uint64_t selectCurrent = this->get<unsigned long long>("currentSourceSettings.selectCurrent");
        const bool normalCurrent = this->get<bool>("currentSourceSettings.normalCurrent");
        const slsDetectorDefs::currentSrcParameters par_v1_0(fixCurrent, selectCurrent);                // chipv1.0
        const slsDetectorDefs::currentSrcParameters par_v1_1(fixCurrent, selectCurrent, normalCurrent); // chipv1.1
        if (m_SLS && !m_SLS->empty()) {
            const std::vector<double> chipVersion = this->get<std::vector<double>>("chipVersion");
            for (int idx : m_positions) {
                if (std::abs(chipVersion[idx] - 1.0) < 0.01) { // v1.0
                    m_SLS->setCurrentSource(par_v1_0, {idx});
                } else if (std::abs(chipVersion[idx] - 1.1) < 0.01) { // v1.1
                    m_SLS->setCurrentSource(par_v1_1, {idx});
                }
            }
            this->set("currentSourceEnabled", true);
        }
    }


    void JungfrauControl::disableCurrentSource() {
        const slsDetectorDefs::currentSrcParameters par;
        if (m_SLS && !m_SLS->empty()) {
            m_SLS->setCurrentSource(par, m_positions); // Disable current source
            this->set("currentSourceEnabled", false);
        }
    }


    void JungfrauControl::resetTempEvent() {
        m_SLS->resetTemperatureEvent(m_positions);
        this->updateState(State::ON);
        this->set("status", "Temperature event reset");
    }


    void JungfrauControl::powerOn() {
        m_SLS->setPowerChip(true, m_positions);                   // power on
        m_SLS->writeRegister(0x4d, 0x108000, false, m_positions); // set additional read-out timeout (bit 15)
        this->disableCurrentSource();
    }

    void JungfrauControl::powerOff() {
        if (m_SLS && !m_SLS->empty()) {
            m_SLS->setPowerChip(false, m_positions); // power off
        }
    }

    void JungfrauControl::pollDetectorSpecific(karabo::data::Hash& h) {
        const std::vector<int> tempAdc = m_SLS->getTemperature(slsDetectorDefs::dacIndex::TEMPERATURE_ADC, m_positions);
        h.set("tempAdc", tempAdc);

        const std::vector<int> tempFpga =
              m_SLS->getTemperature(slsDetectorDefs::dacIndex::TEMPERATURE_FPGA, m_positions);
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


    void JungfrauControl::configureDetectorSpecific(const karabo::data::Hash& configHash) {
        if (configHash.has("exposureTimeout")) {
            // Must set bits 16-31 of 0x7F (ASIC_CTRL) register
            const unsigned int exposureTimeout = configHash.get<unsigned int>("exposureTimeout");
            const unsigned short exposureTimer = std::lround(exposureTimeout / 25) - 1;
            this->set("exposureTimer", exposureTimer); // set read-only device parameter

            const uint32_t addr = 0x7F; // ASIC_CTRL register address
            const std::vector<uint32_t> asicCtrlVector = m_SLS->readRegister(addr);

            int i = 0;
            for (uint32_t asicCtrl : asicCtrlVector) {
                // Replace bits 16-31
                asicCtrl = (asicCtrl & 0xFFFF) | (exposureTimer << 16);

                m_SLS->writeRegister(addr, asicCtrl, false, {i++});
            }
        }
    }

} /* namespace karabo */
