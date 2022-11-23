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

#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include <karabo/karabo.hpp>

#ifndef SLS_SIMULATION
#include <sls/sls_detector_defs.h>
#include <sls/Receiver.h>
#else
#include <slssimulation/Receiver.h>
#endif

#include "version.hh"  // provides SLSRECEIVER_PACKAGE_VERSION

/**
 * The main Karabo namespace
 */
namespace karabo {

    // Detector data (accumulated per train)
    struct DetectorData {
        DetectorData() : mutex(1), accumulatedFrames(0), size(0), adc(0), gain(0) {};

        ~DetectorData() {
            this->free();
        }

        // Semaphore to protect and synchronize access
        boost::interprocess::interprocess_semaphore mutex;

        karabo::util::Timestamp lastTimestamp;
        unsigned short accumulatedFrames;
        size_t size;
        unsigned short* adc;
        unsigned char* gain;
        std::vector<unsigned char> memoryCell;
        std::vector<unsigned long long> frameNumber;
        std::vector<unsigned long long> bunchId;
        std::vector<double> timestamp;

        void free() {
            if (adc != NULL) {
                delete [] adc;
            }
            if (gain != NULL) {
                delete [] gain;
            }
            size = 0;
        }

        void resize(size_t detectorSize, unsigned short framesPerTrain) {
            this->free();

            size = detectorSize * framesPerTrain;
            adc = new unsigned short [size];
            gain = new unsigned char [size];

            memoryCell.resize(framesPerTrain);
            frameNumber.resize(framesPerTrain);
            bunchId.resize(framesPerTrain);
            timestamp.resize(framesPerTrain);
        }

        void reset() {
            accumulatedFrames = 0;
            std::memset(adc, 0, size * sizeof(unsigned short));
            std::memset(gain, 0, size * sizeof(unsigned char));
            std::memset(memoryCell.data(), 255, memoryCell.size() * sizeof(unsigned char));
            std::memset(frameNumber.data(), 0, frameNumber.size() * sizeof(unsigned long long));
            std::memset(bunchId.data(), 0, frameNumber.size() * sizeof(unsigned long long));
            std::memset(timestamp.data(), 0, timestamp.size() * sizeof(double));
        }

        void resetTimestamp(const karabo::util::Timestamp& actualTimestamp) {
            lastTimestamp = actualTimestamp;
        }
    };

    class SlsReceiver : public karabo::core::Device<> {
    public:

        // Add reflection and version information to this class
        KARABO_CLASSINFO(SlsReceiver, "SlsReceiver", SLSRECEIVER_PACKAGE_VERSION)

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration) override;

        /**
         * Constructor providing the initial configuration in form of a Hash object.
         * If this class is constructed using the configuration system the Hash object will
         * already be validated using the information of the expectedParameters function.
         * The configuration is provided in a key/value fashion. 
         */
        explicit SlsReceiver(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~SlsReceiver();

    private: // State-machine call-backs (override)

        void reset();

        void initialize();

    private: // Functions

        static int startAcquisitionCallBack(std::string filePath, std::string fileName, uint64_t fileIndex, uint32_t bufferSize, void* context);

        static void acquisitionFinishedCallBack(uint64_t totalFramesCaught, void* context);

        static void rawDataReadyCallBack(char* metadata, char* dataPointer, uint32_t dataSize, void* context);

        /**
         * The base implementation returns true if meta("trainId") is incremented w.r.t. meta("lastTrainId").
         * May be overridden in derived classes for specific behavior.
         * 
         * @param meta
         * @return true if a new trainId is arrived
         */
        virtual bool isNewTrain(const karabo::util::Hash& meta);

        virtual unsigned char getMemoryCell(const slsDetectorDefs::sls_detector_header& detectorHeader) {
            return 255;
        }

        void logWarning(const std::string& message);

        // Make output schema fit for DAQ
        void updateOutputSchema(unsigned short framesPerTrain);

        // Send End-of-Stream signal
        void signalEndOfStreams();

        // Write to OUTPUT channels
        void writeToOutputs(unsigned char idx, const karabo::util::Timestamp& actualTimestamp);

    private: // Raw data unpacking
        virtual size_t getDetectorSize() = 0;
        virtual std::vector<unsigned long long> getDisplayShape() = 0;
        virtual std::vector<unsigned long long> getDaqShape(unsigned short framesPerTrain) = 0;

        virtual void unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned char* gain) = 0;

    private: // Members

        // SLS receiver class
        std::shared_ptr<sls::Receiver> m_receiver;

        // For frame rate calculation
        unsigned long long m_lastFrameNum;
        double m_lastRateTime;

        // Detector data (accumulated per train)
        // Use one for receiving data, the other one for writing to output channels
        unsigned char m_detectorDataIdx;
        DetectorData m_detectorData[2];

        // Strand to guarantee that the writing order of DetectorData elements is preserved
        karabo::net::Strand::Pointer m_strand;

        // For rate calculation
        long long m_frameCount;

        const unsigned short m_maxWarnPerAcq;
        unsigned short m_warnCounter;

    };

} /* namespace karabo */

#endif /* KARABO_SLSRECEIVER_HH */
