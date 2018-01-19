/*
 * $Id: GotthardDetector.hh 12381 2014-01-15 16:27:07Z parenti $
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on March 21, 2016,  3:15 PM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_GOTTHARDCONTROL_HH
#define KARABO_GOTTHARDCONTROL_HH


#include <karabo/karabo.hpp>

#include "SlsControl.hh"

/**
 * The main Karabo namespace
 */
namespace karabo {

    class GotthardControl : public karabo::SlsControl {
    public:

        KARABO_CLASSINFO(GotthardControl, "GotthardControl", "2.2")

        GotthardControl(const karabo::util::Hash& config);

        virtual ~GotthardControl();

        static void expectedParameters(karabo::util::Schema& expected);
     
     private:

    	const char* getCalibrationString() const;

   	const char* getSettingsString() const;

    };

} /* namespace karabo */

#endif /* KARABO_GOTTHARDCONTROL_HH */
