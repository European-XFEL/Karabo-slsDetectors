/*
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Created on March 21, 2016,  3:15 PM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_GOTTHARDCONTROL_HH
#define KARABO_GOTTHARDCONTROL_HH

#include <karabo/karabo.hpp>

#include "../common/version.hh" // provides SLSDETECTORS_PACKAGE_VERSION
#include "SlsControl.hh"

/**
 * The main Karabo namespace
 */
namespace karabo {

    class GotthardControl : public karabo::SlsControl {
       public:
        KARABO_CLASSINFO(GotthardControl, "GotthardControl", SLSDETECTORS_PACKAGE_VERSION)

        explicit GotthardControl(const karabo::util::Hash& config);

        virtual ~GotthardControl();

        static void expectedParameters(karabo::util::Schema& expected);

       private: // functions
        void pollDetectorSpecific(karabo::util::Hash& h) override;
        void createCalibrationAndSettings(const std::string& settings) {
            // nothing to be done
        }
    };

} /* namespace karabo */

#endif /* KARABO_GOTTHARDCONTROL_HH */
