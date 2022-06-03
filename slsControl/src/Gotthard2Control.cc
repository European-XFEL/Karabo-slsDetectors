/*
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on Januar 12, 2021, 11:40 AM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifdef SLS_SIMULATION
#include <slssimulation/sls_simulation_defs.h>
#endif

#include "Gotthard2Control.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SlsControl, Gotthard2Control)

    Gotthard2Control::Gotthard2Control(const Hash& config) : SlsControl(config) {
#ifdef SLS_SIMULATION
        m_detectorType = static_cast<int>(detectorType::GOTTHARD2);
#endif
    }

    Gotthard2Control::~Gotthard2Control() {
        if (m_SLS) {
            m_SLS->setPowerChip(false, m_positions); // power off
        }
    }

    void Gotthard2Control::expectedParameters(Schema& expected) {
        OVERWRITE_ELEMENT(expected).key("settings")
                .setNewDefaultValue("dynamicgain")
                .setNewOptions({"dynamicgain", "fixgain1", "fixgain2"})
                .commit();

        OVERWRITE_ELEMENT(expected).key("highVoltage")
                .setNewDescription("High voltage to the sensor. "
                "Options: 0|90|110|120|150|180|200 V.")
                .commit();

        OVERWRITE_ELEMENT(expected).key("exposureTime")
                .setNewDisplayedName("Additional Exposure Time")
                .setNewDescription("The additional exposure time. Setting it to 0, will result in "
                "an effective exposure time of ~110 ns.")
                .setNewMinInc(0.)
                .setNewDefaultValue(0.)
                .commit();

        OVERWRITE_ELEMENT(expected).key("exposurePeriod")
                .setNewDisplayedName("Additional Exposure Period")
                .setNewDescription("The additional period between frames. Setting it to 0, will "
                "result in an effective frame period of 222 ns.")
                .setNewMinInc(0.)
                .setNewDefaultValue(0.)
                .commit();

        const std::vector<std::string> timingOptions = {"auto", "trigger"};
        OVERWRITE_ELEMENT(expected).key("timing")
                .setNewOptions(timingOptions)
                .commit();

        std::vector<std::string> timingSourceOptions = {"internal", "external"};
        STRING_ELEMENT(expected).key("timingSource")
                .alias("timingsource")
                .tags("sls")
                .displayedName("Timing Source")
                .description("The timing source. Internal is crystal and "
                "external is system timing.")
                .assignmentOptional().defaultValue("internal")
                .options(timingSourceOptions)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        STRING_ELEMENT(expected).key("burstMode")
                .alias("burstmode")
                .tags("sls")
                .displayedName("Burst Mode")
                .assignmentOptional().defaultValue("burst_internal")
                .options({"burst_internal", "burst_external", "cw_internal", "cw_external"})
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("burstPeriod")
                .alias("burstperiod")
                .tags("sls")
                .displayedName("Burst Period")
                .description("Period between 2 bursts. Only in burst mode and "
                "auto timing mode.")
                .assignmentOptional().defaultValue(0.)
                .minInc(0.)
                .unit(Unit::SECOND)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT64_ELEMENT(expected).key("numberOfBursts")
                .alias("bursts")
                .tags("sls")
                .displayedName("Number of Bursts")
                .description("Number of bursts per aquisition. Only in auto "
                "timing mode and burst mode.")
                .assignmentOptional().defaultValue(1)
                .minInc(1)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // XXX: clkdiv, clkfreq, clkphase


        // TODO badchannels: create bad channel file on the fly in /dev/shm or /tmp?
        STRING_ELEMENT(expected).key("badChannels")
                .alias("badchannels")
                .tags("sls")
                .displayedName("Bad Channels")
                .description("Sets the bad channel filename. Bad channels will be "
                "omitted in the data file. Load an empty file to unset all bad channels.")
                .assignmentOptional().defaultValue("badchannels.txt")
                .reconfigurable()
                .allowedStates(State::ON)
               .commit();

    }

    void Gotthard2Control::powerOn() {
        m_SLS->setPowerChip(true, m_positions); // power on
    }

} /* namespace karabo */
