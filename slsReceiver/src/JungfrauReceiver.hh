/*
 * $Id: JungfrauReceiver.hh 12523 2014-01-27 15:15:19Z parenti $
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on March 21, 2016, 11:37 AM
 *
 * Copyright (c) 2010-2016 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_JUNGFRAURECEIVER_HH
#define KARABO_JUNGFRAURECEIVER_HH

#include <karabo/karabo.hpp>

#include "SlsReceiver.hh"
#include "version.hh"  // provides PACKAGE_VERSION

/**
 * The main Karabo namespace
 */
namespace karabo {

    class JungfrauReceiver : public karabo::SlsReceiver {
    public:

        // Add reflection and version information to this class
        KARABO_CLASSINFO(JungfrauReceiver, "JungfrauReceiver", PACKAGE_VERSION)

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        /**
         * Constructor providing the initial configuration in form of a Hash object.
         * If this class is constructed using the configuration system the Hash object will
         * already be validated using the information of the expectedParameters function.
         * The configuration is provided in a key/value fashion. 
         */
        explicit JungfrauReceiver(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~JungfrauReceiver();

    private: // State-machine call-backs (override)

    private: // Functions
        virtual bool isNewTrain(const karabo::util::Hash& meta) override;
        virtual unsigned char getMemoryCell(const slsDetectorDefs::sls_detector_header& detectorHeader) override;

    private: // Raw data unpacking

        size_t getDetectorSize() override;
        std::vector<unsigned long long> getDisplayShape() override;
        std::vector<unsigned long long> getDaqShape(unsigned short framesPerTrain) override;

        void unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned char* gain) override;

    private: // Members

    };

} /* namespace karabo */

#endif /* KARABO_JUNGFRAURECEIVER_HH */
