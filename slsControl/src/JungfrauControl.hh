/*
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on March 23, 2016,  3:39 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_JUNGFRAUCONTROL_HH
#define KARABO_JUNGFRAUCONTROL_HH


#include <karabo/karabo.hpp>

#include "SlsControl.hh"

/**
 * The main Karabo namespace
 */
namespace karabo {

    class JungfrauControl : public karabo::SlsControl {
    public:
        KARABO_CLASSINFO(JungfrauControl, "JungfrauControl", "2.2")

        JungfrauControl(const karabo::util::Hash& config);

        virtual ~JungfrauControl();

        static void expectedParameters(karabo::util::Schema& expected);

    private:
        void powerOn();
        const char* getCalibrationString() const;
        const char* getSettingsString() const;

    };

} /* namespace karabo */

#endif /* KARABO_JUNGFRAUCONTROL_HH */
