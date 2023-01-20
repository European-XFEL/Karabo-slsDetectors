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
        if (m_SLS  && !m_SLS->empty()) {
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
                .setNewDefaultValue(0)
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

        BOOL_ELEMENT(expected).key("singlePhoton")
                .displayedName("Optimize for Single-Photon")
                .description("Optimize the detector for single-photon operation; this will "
                "increase the amplification of the CDS stage, and change the Vref of the analog "
                "storage cells to make better use of the range of the ADC where it is most "
                "linear.")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        STRING_ELEMENT(expected).key("acquisitionRate")
                .displayedName("Acquisition Rate")
                .description("Switch between the 4.5 MHz operation mode (default), and a slower "
                "sampling and readout rate that allows operation at 1.1 MHz.")
                .assignmentOptional().defaultValue("4.5")
                .options("1.1,4.5")
                .unit(Unit::HERTZ).metricPrefix(MetricPrefix::MEGA)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

    }

    void Gotthard2Control::powerOn() {
        m_SLS->setPowerChip(true, m_positions); // power on
    }

    void Gotthard2Control::configureDetectorSpecific(const karabo::util::Hash& configHash) {
        if (configHash.has("singlePhoton")) {
            const bool& singlePhoton = configHash.get<bool>("singlePhoton");
            if (singlePhoton) {
                // Change to 'single photon' settings
                this->sendConfiguration("dac", "vref_rstore 0"); // shift the Vref of the storage cells
                this->sendConfiguration("cdsgain", "1"); // increase the CDS gain
            } else { // Restore default conditions
                this->sendConfiguration("dac", "vref_rstore 150"); // shift back the Vref
                this->sendConfiguration("cdsgain", "0"); // lower the CDS gain
            }
        }

        if (configHash.has("acquisitionRate")) {
            const std::string& acquisitionRate = configHash.get<std::string>("acquisitionRate");
            if (acquisitionRate == "1.1") { // Change the frame rate to 1.1 MHz
                // Reduce by a factor 4 the running clock
                this->sendConfiguration("clkdiv", "2 20");
                this->sendConfiguration("clkdiv", "3 40");
                this->sendConfiguration("clkdiv", "2 20");
                // Reduce the data output sampling
                this->sendConfiguration("reg", "0x120 0x00000000");
                this->sendConfiguration("clkdiv", "0 16");
                this->sendConfiguration("clkdiv", "1 16");
                this->sendConfiguration("clkphase", "1 270 deg");

            } else { // Restore the 4.5 MHz default
                // Increase the running clock settings
                this->sendConfiguration("clkdiv", "2 5");
                this->sendConfiguration("clkdiv", "3 10");
                this->sendConfiguration("clkdiv", "2 5");
                // Restore the readout clock speed
                this->sendConfiguration("readoutspeed", "108");
            }
        }

    }

} /* namespace karabo */
