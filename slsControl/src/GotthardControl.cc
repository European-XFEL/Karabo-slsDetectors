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
        m_detectorType = static_cast<int>(detectorType::GOTTHARD);
#endif
    }

    GotthardControl::~GotthardControl() {
    }

    void GotthardControl::expectedParameters(Schema& expected) {
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

        STRING_ELEMENT(expected).key("extSig0")
                .alias("extsig:0")
                .tags("sls")
                .displayedName("extSig0")
                .description("Ext Sig 0")
                .assignmentOptional().defaultValue("off")
                .options("off gate_in_active_high gate_in_active_low trigger_in_rising_edge "
                "trigger_in_falling_edge ro_trigger_in_rising_edge ro_trigger_in_falling_edge "
                "gate_out_active_high gate_out_active_low trigger_out_rising_edge "
                "trigger_out_falling_edge ro_trigger_out_rising_edge ro_trigger_out_falling_edge sync")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT64_ELEMENT(expected).key("numberOfGates")
                .alias("gates")
                .tags("sls")
                .displayedName("NumberOfGates")
                .description("Number Of Gates")
                .assignmentOptional().defaultValue(0)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();
    }

    const char* GotthardControl::getCalibrationString() const {
        return "227 5.6\n";
    }

    const char* GotthardControl::getSettingsString() const {
        return "Vcasc 1320\nVcascN 650\nVcascP 1480\nVib_test 2001\nVin 1350\nVout 1520\nVref 660\nVref_comp 887\n";
    }

} /* namespace karabo */
