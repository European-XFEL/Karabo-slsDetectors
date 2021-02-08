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
        m_detectorType = slsDetectorDefs::detectorType::JUNGFRAU;
#endif
    }


    JungfrauControl::~JungfrauControl() {
        if (m_SLS != nullptr) {
            m_SLS->setPowerChip(false, m_positions); // power off
        }
    }


    void JungfrauControl::expectedParameters(Schema& expected) {
        OVERWRITE_ELEMENT(expected).key("settings") // From base class
                .setNewDefaultValue("dynamicgain")
                .setNewOptions("dynamicgain,dynamichg0,fixgain1,fixgain2,forceswitchg1,forceswitchg2")
                .commit();

        OVERWRITE_ELEMENT(expected).key("highVoltage")
                .setNewDescription("High voltage to the sensor in Voltage. "
                "Options: 0|60-200.")
                .commit();

        OVERWRITE_ELEMENT(expected).key("exposureTime")
                .setNewMaxInc(0.001) // 1 ms
                .commit();

        OVERWRITE_ELEMENT(expected).key("timing")
                .setNewOptions("auto,trigger")
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
