/*
 * $Id: SlsReceiver.hh 12523 2014-01-27 15:15:19Z parenti $
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on December 19, 2013, 12:01 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_SLSRECEIVER_HH
#define KARABO_SLSRECEIVER_HH

#include <karabo/karabo.hpp>

#ifndef SLS_SIMULATION
#include <slsdetectors/sls_detector_defs.h>
#include <slsdetectors/slsReceiverUsers.h>
#else
#include <slssimulation/slsReceiverUsers.h>
#endif



/**
 * The main Karabo namespace
 */
namespace karabo {

    class SlsReceiver : public karabo::core::Device<> {
    public:

        // Add reflection and version information to this class
        KARABO_CLASSINFO(SlsReceiver, "SlsReceiver", "2.0")

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);

        /**
         * Constructor providing the initial configuration in form of a Hash object.
         * If this class is constructed using the configuration system the Hash object will
         * already be validated using the information of the expectedParameters function.
         * The configuration is provided in a key/value fashion. 
         */
        SlsReceiver(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~SlsReceiver();

    private: // State-machine call-backs (override)

        void reset();

        void initialize();

    private: // Functions

        static int startAcquisitionCallBack(char* filePath, char* fileName, size_t fileIndex, unsigned int bufferSize, void* context);

        static void acquisitionFinishedCallBack(size_t totalFramesCaught, void* context);

        static void rawDataReadyCallBack(char* metadata, char* dataPointer, uint32_t dataSize, void* context);

        void logWarning(const std::string& message);
	
	// Make output schema fit for DAQ
        void updateOutputSchema(unsigned short framesPerTrain);


    private: // Raw data unpacking
        virtual size_t getDetectorSize() = 0;
        virtual std::vector<unsigned long long> getDisplayShape() = 0;
        virtual std::vector<unsigned long long> getDaqShape(unsigned short framesPerTrain) = 0;

        virtual void unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned char* gain) = 0;

    private: // Members

        // SLS receiver class
        slsReceiverUsers* m_receiver;

        // For frame rate calculation
        unsigned long long m_lastFrameNum;
        double m_lastRateTime;

        // Detector data (accumulated per train)
        karabo::util::Timestamp m_lastTimestamp;
        unsigned short m_accumulatedFrames;
        unsigned short* m_adc;
        unsigned char* m_gain;
        std::vector<unsigned char> m_memoryCell;
        std::vector<unsigned long long> m_frameNumber;
        std::vector<double> m_timestamp;

        // For rate calculation
        long long m_frameCount;

        const unsigned short m_maxWarnPerAcq;
        unsigned short m_warnCounter;
    };

} /* namespace karabo */

#endif /* KARABO_SLSRECEIVER_HH */
