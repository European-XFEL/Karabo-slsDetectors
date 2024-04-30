/*
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Created on March 23, 2016,  3:39 PM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_JUNGFRAUCONTROL_HH
#define KARABO_JUNGFRAUCONTROL_HH


#include <karabo/karabo.hpp>

#include "../common/version.hh" // provides SLSDETECTORS_PACKAGE_VERSION
#include "SlsControl.hh"

/**
 * The main Karabo namespace
 */
namespace karabo {

    class JungfrauControl : public karabo::SlsControl {
       public:
        KARABO_CLASSINFO(JungfrauControl, "JungfrauControl", SLSDETECTORS_PACKAGE_VERSION)

        explicit JungfrauControl(const karabo::util::Hash& config);


        static void expectedParameters(karabo::util::Schema& expected);

       private:
        void resetTempEvent();
        void powerOn() override;
        void powerOff() override;

        void pollDetectorSpecific(karabo::util::Hash& h) override;
        void configureDetectorSpecific(const karabo::util::Hash& configHash) override;
        void createCalibrationAndSettings(const std::string& settings) {
            // nothing to be done
        }
    };

} /* namespace karabo */

#endif /* KARABO_JUNGFRAUCONTROL_HH */
