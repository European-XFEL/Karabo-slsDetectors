/*
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on March 21, 2016,  3:15 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifdef SLS_SIMULATION
#include <slssimulation/sls_simulation_defs.h>
#endif

#include "GotthardControl.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SlsControl, GotthardControl)


    GotthardControl::GotthardControl(const Hash& config) : SlsControl(config) {
#ifdef SLS_SIMULATION
        m_detectorType = slsDetectorDefs::detectorType::GOTTHARD;
#endif
    }


    GotthardControl::~GotthardControl() {
    }


    void GotthardControl::expectedParameters(Schema& expected) {
        OVERWRITE_ELEMENT(expected).key("settings")
                .setNewDefaultValue("dynamicgain")
                .setNewOptions({"dynamicgain", "lowgain", "mediumgain", "highgain", "veryhighgain"})
                .commit();

        OVERWRITE_ELEMENT(expected).key("highVoltage")
                .setNewDescription("High voltage to the sensor. "
                "Options: 0|90|110|120|150|180|200 V.")
                .commit();

        const std::vector<std::string> timingOptions = {"auto", "trigger"};
        OVERWRITE_ELEMENT(expected).key("timing")
                .setNewOptions(timingOptions)
                .commit();

        OVERWRITE_ELEMENT(expected).key("exposureTime")
                .setNewMinExc(0.)
                .commit();

        OVERWRITE_ELEMENT(expected).key("exposurePeriod")
                .setNewMinExc(0.)
                .commit();

        // Only "extsig 0" is used in gotthard
        std::vector<std::string> extSig0Options = {"trigger_in_rising_edge", "trigger_in_falling_edge"};
        STRING_ELEMENT(expected).key("extSig0")
                .alias("extsig 0")
                .tags("sls")
                .displayedName("extSig0")
                .description("Ext Sig 0")
                .assignmentOptional().defaultValue("trigger_in_rising_edge")
                .options(extSig0Options)
                .reconfigurable()
                .allowedStates(State::ON)
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


    void GotthardControl::pollDetectorSpecific(karabo::util::Hash& h) {
        const std::vector<int> tempAdc = m_SLS->getTemperature(slsDetectorDefs::dacIndex::TEMPERATURE_ADC, m_positions);
        h.set("tempAdc", tempAdc);

        const std::vector<int> tempFpga = m_SLS->getTemperature(slsDetectorDefs::dacIndex::TEMPERATURE_FPGA, m_positions);
        h.set("tempFpga", tempFpga);
    }

} /* namespace karabo */
