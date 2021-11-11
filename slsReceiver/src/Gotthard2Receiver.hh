/*
 * Author: <andrea.parenti@xfel.eu>
 *
 * Created on August, 2021,  5:16 PM
 * 
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_GOTTHARD2RECEIVER_HH
#define KARABO_GOTTHARD2RECEIVER_HH

#include <karabo/karabo.hpp>

#include "SlsReceiver.hh"
#include "version.hh"  // provides PACKAGE_VERSION

/**
 * The main Karabo namespace
 */
namespace karabo {

    class Gotthard2Receiver : public karabo::SlsReceiver {
    public:

        // Add reflection and version information to this class
        KARABO_CLASSINFO(Gotthard2Receiver, "Gotthard2Receiver", PACKAGE_VERSION)

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
        Gotthard2Receiver(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~Gotthard2Receiver();

    private: // State-machine call-backs (override)

    private: // Functions

    private: // Raw data unpacking

        size_t getDetectorSize();
        std::vector<unsigned long long> getDisplayShape();
        std::vector<unsigned long long> getDaqShape(unsigned short framesperTrain);

        void unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned char* gain);

    private: // Members

    };

} /* namespace karabo */

#endif /* KARABO_GOTTHAR2DRECEIVER_HH */
