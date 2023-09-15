/*
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Created on Januar 12, 2021, 11:38 AM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_GOTTHARD2CONTROL_HH
#define KARABO_GOTTHARD2CONTROL_HH

#include <karabo/karabo.hpp>

#include "../common/version.hh" // provides SLSDETECTORS_PACKAGE_VERSION
#include "SlsControl.hh"

/**
 * The main Karabo namespace
 */
namespace karabo {

    class Gotthard2Control : public karabo::SlsControl {
       public:
        KARABO_CLASSINFO(Gotthard2Control, "Gotthard2Control", SLSDETECTORS_PACKAGE_VERSION)

        explicit Gotthard2Control(const karabo::util::Hash& config);

        virtual ~Gotthard2Control();

        static void expectedParameters(karabo::util::Schema& expected);

       private: // functions
        void powerOn() override;
        void pollDetectorSpecific(karabo::util::Hash& h) override;
        void configureDetectorSpecific(const karabo::util::Hash& configHash) override;
        void createCalibrationAndSettings(const std::string& settings) {
            // nothing to be done
        }
    };

} /* namespace karabo */

#endif /* KARABO_GOTTHARD2CONTROL_HH */
