/*
 * Author: <andrea.parenti@xfel.eu>
 *
 * Created on December 19, 2013, 12:01 PM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "SlsReceiver.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    void SlsReceiver::expectedParameters(Schema& expected) {
        OVERWRITE_ELEMENT(expected)
              .key("state")
              .setNewOptions(State::UNKNOWN, State::PASSIVE, State::ACTIVE, State::ERROR)
              .commit();

        SLOT_ELEMENT(expected).key("reset").displayedName("Reset").allowedStates(State::ERROR).commit();

        UINT16_ELEMENT(expected)
              .key("rxTcpPort")
              .tags("sls")
              .alias("--rx_tcpport")
              .displayedName("rxTcpPort")
              .description("Receiver TCP Port")
              .assignmentOptional()
              .defaultValue(1954)
              .init()
              .commit();

        UINT16_ELEMENT(expected)
              .key("framesPerTrain")
              .displayedName("Frames per Train")
              .description(
                    "How many frames will be sent to DAQ for each train."
                    "If more are received from detector, they will be discarded.")
              .assignmentOptional()
              .defaultValue(1)
              .minInc(1)
              .reconfigurable()
              .allowedStates(State::PASSIVE)
              .commit();

        FLOAT_ELEMENT(expected)
              .key("frameRateIn")
              .displayedName("Frame Rate In")
              .description("Frame rate - incoming data from detector.")
              .unit(Unit::HERTZ)
              .readOnly()
              .commit();

        FLOAT_ELEMENT(expected)
              .key("frameRateOut")
              .displayedName("Frame Rate Out")
              .description("Frame rate - decoded data to output channels.")
              .unit(Unit::HERTZ)
              .readOnly()
              .commit();

        Schema outputData;

        NODE_ELEMENT(outputData).key("data").displayedName("Data").setDaqDataType(DaqDataType::TRAIN).commit();

        NDARRAY_ELEMENT(outputData)
              .key("data.adc")
              .displayedName("ADC")
              .description("The ADC counts.")
              .dtype(karabo::util::Types::UINT16)
              .readOnly()
              .commit();

        NDARRAY_ELEMENT(outputData)
              .key("data.gain")
              .displayedName("Gain")
              .description("The ADC gains.")
              .dtype(karabo::util::Types::UINT8)
              .readOnly()
              .commit();

        VECTOR_UINT8_ELEMENT(outputData)
              .key("data.memoryCell")
              .displayedName("Memory Cell")
              .description("The number of the memory cell used to store the image (only for Jungfrau).")
              .readOnly()
              .commit();

        VECTOR_UINT64_ELEMENT(outputData)
              .key("data.frameNumber")
              .displayedName("Frame Number")
              .description("The frame number.")
              .readOnly()
              .commit();

        VECTOR_UINT64_ELEMENT(outputData)
              .key("data.bunchId")
              .displayedName("Bunch ID")
              .description("The bunch ID from the beamline, if available.")
              .readOnly()
              .commit();

        VECTOR_DOUBLE_ELEMENT(outputData)
              .key("data.timestamp")
              .displayedName("Timestamp")
              .description("The data timestamp.")
              .readOnly()
              .commit();

        OUTPUT_CHANNEL(expected).key("output").displayedName("PP Output").dataSchema(outputData).commit();

        // Second output channel for the DAQ
        OUTPUT_CHANNEL(expected).key("daqOutput").displayedName("DAQ Output").dataSchema(outputData).commit();

        BOOL_ELEMENT(expected)
              .key("onlineDisplayEnable")
              .displayedName("Online Display Enable")
              .description("Enable online display of detector data.")
              .assignmentOptional()
              .defaultValue(false)
              .reconfigurable()
              .commit();

        UINT16_ELEMENT(expected)
              .key("frameToDisplay")
              .displayedName("Frame To Display")
              .description("The index of the frame to be displayed in the train, starting from 0.")
              .assignmentOptional()
              .defaultValue(0)
              .reconfigurable()
              .commit();
    }

    void SlsReceiver::preReconfigure(Hash& incomingReconfiguration) {
        if (incomingReconfiguration.has("framesPerTrain")) {
            // Update schema
            this->updateOutputSchema(incomingReconfiguration.get<unsigned short>("framesPerTrain"));
        }
    }

    SlsReceiver::SlsReceiver(const karabo::util::Hash& config)
        : Device<>(config),
          m_receiver(nullptr),
          m_lastFrameNum(0),
          m_lastRateTime(0.),
          m_detectorDataIdx(0),
          m_strand(boost::make_shared<karabo::net::Strand>(karabo::net::EventLoop::getIOService())),
          m_frameCount(0),
          m_maxWarnPerAcq(10),
          m_warnCounter(0) {
        KARABO_INITIAL_FUNCTION(initialize);
        KARABO_SLOT(reset);
    }

    SlsReceiver::~SlsReceiver() {}

    void SlsReceiver::reset() {
        if (m_receiver == nullptr) {
            // m_receiver needs to be initialized
            this->initialize();
        }
    }

    void SlsReceiver::initialize() {
        std::string rxTcpPort = "1954"; // Default port
        std::stringstream status;

        // Get slsReceiver parameters from current configuration
        std::vector<std::string> __argv__;
        __argv__.push_back("ignored"); // First parameter will be ignored
        const karabo::util::Hash config = this->getCurrentConfiguration("sls");
        for (karabo::util::Hash::const_iterator it = config.begin(); it != config.end(); ++it) {
            try {
                const std::string key = it->getKey();
                const std::string value = config.getAs<std::string>(key);
                const std::string alias = getAliasFromKey<std::string>(key);
                if (alias == "--rx_tcpport") rxTcpPort = value;
                __argv__.push_back(alias);
                __argv__.push_back(value);
                KARABO_LOG_FRAMEWORK_DEBUG << "Parameter for receiver: key=" << key << " alias=" << alias
                                           << " value=" << value;
            } catch (const karabo::util::Exception& e) {
                status << "Error in initialize: " << e.what();
                this->set("status", status.str());
                KARABO_LOG_WARN << status.str();
            }
        }

        // Create list of arguments for slsReceiverUsers object
        const int argc = __argv__.size();
        char* argv[argc];
        for (int i = 0; i < argc; ++i) argv[i] = (char*)__argv__.at(i).c_str();

        try {
            std::shared_ptr<sls::Receiver> receiver(new sls::Receiver(argc, argv));

            // Register callback functions
            receiver->registerCallBackStartAcquisition(startAcquisitionCallBack, static_cast<void*>(this));
            receiver->registerCallBackAcquisitionFinished(acquisitionFinishedCallBack, static_cast<void*>(this));
            receiver->registerCallBackRawDataReady(rawDataReadyCallBack, static_cast<void*>(this));

            // Update schema
            this->updateOutputSchema(this->get<unsigned short>("framesPerTrain"));

            m_receiver.swap(receiver);

            status << "Receiver started on port: " << rxTcpPort;
            this->set("status", status.str());
            KARABO_LOG_INFO << status.str();

            // All went fine, update state
            this->updateState(State::PASSIVE);

        } catch (const std::exception& e) {
            // This occurs e.g. when another receiver is listening on the same port
            status << "Error in initialize: " << e.what();
            this->set("status", status.str());
            KARABO_LOG_ERROR << status.str();
            this->updateState(State::ERROR);
            return;
        }
    }

    int SlsReceiver::startAcquisitionCallBack(const slsDetectorDefs::startCallbackHeader, void* context) {
        Self* self = static_cast<Self*>(context);

        try {
            self->m_frameCount = 0;
            self->updateState(State::ACTIVE);

            // Set start values
            const karabo::util::Timestamp& actualTimestamp = self->getActualTimestamp();
            self->m_lastRateTime = actualTimestamp.toTimestamp();
            self->m_lastFrameNum = 0;
            self->m_warnCounter = 0;

            // Allocate memory for data
            const unsigned short framesPerTrain = self->get<unsigned short>("framesPerTrain");
            self->m_detectorData[0].resize(self->getDetectorSize(), framesPerTrain);
            self->m_detectorData[1].resize(self->getDetectorSize(), framesPerTrain);

            // Reset detector data
            self->m_detectorDataIdx = 0;
            self->m_detectorData[0].reset();
            self->m_detectorData[1].reset();

        } catch (const std::exception& e) {
            KARABO_LOG_FRAMEWORK_WARN << "startAcquisitionCallBack: " << e.what();
        } catch (...) {
            KARABO_LOG_FRAMEWORK_WARN << "startAcquisitionCallBack: other exception";
        }

        // From "slsReceiverUsers.h": return value is insignificant at the moment, we write depending on file
        // write enable, users get data to write depending on call backs registered
        return 0;
    }

    void SlsReceiver::acquisitionFinishedCallBack(const slsDetectorDefs::endCallbackHeader, void* context) {
        Self* self = static_cast<Self*>(context);

        try {
            if (self->m_frameCount > 0) {
                const karabo::util::Timestamp& actualTimestamp = self->getActualTimestamp();
                const double currentTime = actualTimestamp.toTimestamp();
                const double elapsedTime = currentTime - self->m_lastRateTime;
                const double frameRate = self->m_frameCount / elapsedTime;

                self->set("frameRateIn", frameRate);
                KARABO_LOG_FRAMEWORK_DEBUG << "Frame rate (receiver) " << frameRate;
            }

            // Reset frame rates after acquisition is over
            const Hash h("frameRateIn", 0., "frameRateOut", 0.);
            self->set(h);

            // Signals end of stream
            // This is done in the same strand as writeToOutputs, to preserve order
            self->m_strand->post(karabo::util::bind_weak(&SlsReceiver::signalEndOfStreams, self));

        } catch (const std::exception& e) {
            KARABO_LOG_FRAMEWORK_WARN << "acquisitionFinishedCallBack: " << e.what();
        } catch (...) {
            KARABO_LOG_FRAMEWORK_WARN << "acquisitionFinishedCallBack: other exception";
        }

        self->updateState(State::PASSIVE);
    }

    void SlsReceiver::rawDataReadyCallBack(slsDetectorDefs::sls_receiver_header& header,
                                           const slsDetectorDefs::dataCallbackHeader, char* dataPointer,
                                           size_t& dataSize, void* context) {
        Self* self = static_cast<Self*>(context);
        const slsDetectorDefs::sls_detector_header& detectorHeader = header.detHeader;

        try {
            const unsigned short framesPerTrain = self->get<unsigned short>("framesPerTrain");

            karabo::util::Timestamp actualTimestamp;
            // See https://slsdetectorgroup.github.io/devdoc/udpdetspec.html
            const uint64_t& bunchId = detectorHeader.detSpec1;
            if (bunchId != 0 && bunchId != 0xFFFFFFFFFFFFFFFF) {
                // The firmware is able to provide bunchId: use it, if available.
                actualTimestamp = Timestamp(Epochstamp(), bunchId);
            } else {
                actualTimestamp = self->getActualTimestamp();
            }
            const double currentTime = actualTimestamp.toTimestamp();
            const unsigned long long trainId = actualTimestamp.getTrainId();

            // Detector data, trainId, elapsed time
            DetectorData* detectorData = &(self->m_detectorData[self->m_detectorDataIdx]);
            const unsigned long long lastTrainId = detectorData->lastTimestamp.getTrainId();
            const double elapsedTime = currentTime - self->m_lastRateTime;

            Hash meta;
            meta.set("trainId", trainId);
            meta.set("lastTrainId", lastTrainId);

            const unsigned char memoryCell = self->getMemoryCell(detectorHeader);
            meta.set("memoryCell", memoryCell);

            if ((self->isNewTrain(meta) && detectorData->accumulatedFrames > 0) ||
                (trainId == 0 && detectorData->accumulatedFrames >= framesPerTrain)) {
                // A call to 'writeToOutputs' will be posted if:
                // 1) 'isNewTrain' returns true AND at least one frame has been accumulated;
                // OR
                // 2) 'trainId' is 0 AND 'framesPerTrain' frames are accumulated.
                // If the SlsReceiver receives trainIds from a TimeServer, condition 1) will be satisfied as soon as a
                // new train starts, in case there is no connection to TimeServer 2) will be satisfied when
                // 'detectorData' is full.

                detectorData->mutex.wait(); // "lock", then process detectorData in the event loop
                self->m_strand->post(karabo::util::bind_weak(&SlsReceiver::writeToOutputs, self,
                                                             self->m_detectorDataIdx, actualTimestamp));

                // Use next DetectorData object for receiving data
                self->m_detectorDataIdx = (self->m_detectorDataIdx + 1) % 2;
                detectorData = &(self->m_detectorData[self->m_detectorDataIdx]);
                detectorData->resetTimestamp(actualTimestamp);
            }

            const size_t frameSize = sizeof(unsigned short) * self->getDetectorSize();
            if (dataSize == 0) {
                self->logWarning("rawDataReadyCallBack: received empty buffer. Skip!");
                return;
            } else if (dataSize % frameSize != 0) {
                self->logWarning("rawDataReadyCallBack: data size (" + util::toString(dataSize) +
                                 ") is not multiple of frameSize size (" + util::toString(frameSize) + ")! Skip data.");
                return;
            }

            detectorData->mutex.wait(); // "lock"
            const auto accumulatedFrames = detectorData->accumulatedFrames;
            if (accumulatedFrames >= framesPerTrain) {
                // Already got enough frames for this train -> skip data
                detectorData->mutex.post(); // "unlock"
                return;
            }

            const unsigned int numberOfFrames = dataSize / frameSize;

            for (unsigned int i = 0; i < numberOfFrames; ++i) {
                const size_t offset = self->getDetectorSize() * accumulatedFrames;
                try {
                    self->unpackRawData(dataPointer, i, detectorData->adc + offset, detectorData->gain + offset);
                    detectorData->memoryCell[accumulatedFrames] = memoryCell;
                    detectorData->frameNumber[accumulatedFrames] = detectorHeader.frameNumber;
                    detectorData->bunchId[accumulatedFrames] = bunchId;

                    detectorData->timestamp[accumulatedFrames] = currentTime;
                    detectorData->accumulatedFrames += 1;
                } catch (const std::exception& e) {
                    self->logWarning(e.what());
                }
            }

            detectorData->mutex.post(); // "unlock"

            self->m_frameCount += numberOfFrames;

            if (self->m_lastFrameNum == 0) {
                // First frame received
                self->m_lastRateTime = currentTime;
                self->m_lastFrameNum = detectorHeader.frameNumber;
            } else if (elapsedTime > 1. && detectorHeader.frameNumber > self->m_lastFrameNum) {
                // Log frame rate once per second
                const double frameRateIn =
                      (detectorHeader.frameNumber - self->m_lastFrameNum) / elapsedTime; // Detector rate
                const double frameRateOut = self->m_frameCount / elapsedTime;            // Receiver rate

                const Hash h("frameRateIn", frameRateIn, "frameRateOut", frameRateOut);
                self->set(h);

                KARABO_LOG_FRAMEWORK_DEBUG << "Current Frame: " << detectorHeader.frameNumber
                                           << " Last Frame: " << self->m_lastFrameNum
                                           << " Elapsed time [s]: " << elapsedTime;
                KARABO_LOG_FRAMEWORK_DEBUG << "Frame rate (detector) " << frameRateIn << " Hz";
                KARABO_LOG_FRAMEWORK_DEBUG << "Frame rate (receiver) " << frameRateOut << " Hz";
                KARABO_LOG_FRAMEWORK_DEBUG << "Train ID " << trainId;

                self->m_frameCount = 0;
                self->m_lastRateTime = currentTime;
                self->m_lastFrameNum = detectorHeader.frameNumber;
            }

        } catch (const std::exception& e) {
            KARABO_LOG_FRAMEWORK_WARN << "rawDataReadyCallBack: " << e.what();
        } catch (...) {
            KARABO_LOG_FRAMEWORK_WARN << "rawDataReadyCallBack: other exception";
        }
    }

    bool SlsReceiver::isNewTrain(const karabo::util::Hash& meta) {
        const auto trainId = meta.get<unsigned long long>("trainId");
        const auto lastTrainId = meta.get<unsigned long long>("lastTrainId");

        if (trainId > lastTrainId) {
            return true;
        } else {
            return false;
        }
    }

    void SlsReceiver::logWarning(const std::string& message) {
        if (m_warnCounter < m_maxWarnPerAcq) {
            KARABO_LOG_FRAMEWORK_WARN << message;
            ++m_warnCounter;
        } else if (m_warnCounter == m_maxWarnPerAcq) {
            KARABO_LOG_FRAMEWORK_WARN << "No more messages will be logged for this acquisition.";
            ++m_warnCounter;
        }
    }

    void SlsReceiver::updateOutputSchema(unsigned short framesPerTrain) {
        Schema daqData;
        const auto daqShape = this->getDaqShape(framesPerTrain);

        KARABO_LOG_FRAMEWORK_DEBUG << "Updating output schema";

        NODE_ELEMENT(daqData).key("data").displayedName("Data").setDaqDataType(DaqDataType::TRAIN).commit();

        NDARRAY_ELEMENT(daqData)
              .key("data.adc")
              .displayedName("ADC")
              .description("The ADC counts.")
              .dtype(karabo::util::Types::UINT16)
              .shape(karabo::util::toString(daqShape))
              .readOnly()
              .commit();

        NDARRAY_ELEMENT(daqData)
              .key("data.gain")
              .displayedName("Gain")
              .description("The ADC gains.")
              .dtype(karabo::util::Types::UINT8)
              .shape(karabo::util::toString(daqShape))
              .readOnly()
              .commit();

        VECTOR_UINT8_ELEMENT(daqData)
              .key("data.memoryCell")
              .displayedName("Memory Cell")
              .description("The number of the memory cell used to store the image (only for Jungfrau).")
              .maxSize(framesPerTrain)
              .readOnly()
              .commit();

        VECTOR_UINT64_ELEMENT(daqData)
              .key("data.frameNumber")
              .displayedName("Frame Number")
              .description("The frame number.")
              .maxSize(framesPerTrain)
              .readOnly()
              .commit();

        VECTOR_UINT64_ELEMENT(daqData)
              .key("data.bunchId")
              .displayedName("Bunch ID")
              .description("The bunch ID from the beamline, if available.")
              .maxSize(framesPerTrain)
              .readOnly()
              .commit();

        VECTOR_DOUBLE_ELEMENT(daqData)
              .key("data.timestamp")
              .displayedName("Timestamp")
              .description("The data timestamp.")
              .maxSize(framesPerTrain)
              .readOnly()
              .commit();

        // New schema for output channel
        Schema schema;

        OUTPUT_CHANNEL(schema).key("daqOutput").displayedName("DAQ Output").dataSchema(daqData).commit();

        std::string outputHostname;
        try {
            outputHostname = this->get<std::string>("daqOutput.hostname");
        } catch (const karabo::util::ParameterException& e) {
            // Current configuration does not contain "output.hostname"
        }

        // Update the device schema
        this->updateSchema(schema);

        if (outputHostname != "") {
            // Restore daqOutput.hostname
            this->set("daqOutput.hostname", outputHostname);
        }
    }

    void SlsReceiver::signalEndOfStreams() {
        this->signalEndOfStream("output");
        this->signalEndOfStream("daqOutput");
        this->signalEndOfStream("display");
    }

    void SlsReceiver::writeToOutputs(unsigned char idx, const karabo::util::Timestamp& actualTimestamp) {
        DetectorData* detectorData = &m_detectorData[idx];

        const auto detectorSize = this->getDetectorSize();
        const auto framesPerTrain = this->get<unsigned short>("framesPerTrain");
        const size_t size = detectorSize * framesPerTrain;

        // The Pipeline shape is an array of display shapes
        auto vPPShape = this->getDisplayShape();
        vPPShape.insert(vPPShape.begin(), framesPerTrain);
        const Dims ppShape = vPPShape;
        const Dims daqShape = this->getDaqShape(framesPerTrain);
        NDArray adcTrainData(detectorData->adc, size, NDArray::NullDeleter(), ppShape);   // No-copy constructor
        NDArray gainTrainData(detectorData->gain, size, NDArray::NullDeleter(), ppShape); // No-copy constructor

        // KARABO_LOG_FRAMEWORK_DEBUG << "Ready to output data. trainId=" << trainId <<
        //         " lastTrainId=" << lastTrainId << " accumulatedFrames=" << detectorData.accumulatedFrames;

        // Send unpacked data to output channel - for PP

        Hash output;
        output.set("data.adc", adcTrainData);
        output.set("data.gain", gainTrainData);
        output.set("data.memoryCell", detectorData->memoryCell);
        output.set("data.frameNumber", detectorData->frameNumber);
        output.set("data.bunchId", detectorData->bunchId);
        output.set("data.timestamp", detectorData->timestamp);
        this->writeChannel("output", output, detectorData->lastTimestamp);

        // Reshape ADC/gain arrays and send them to DAQ
        adcTrainData.setShape(daqShape);
        gainTrainData.setShape(daqShape);
        output.set("data.adc", adcTrainData);
        output.set("data.gain", gainTrainData);
        this->writeChannel("daqOutput", output, detectorData->lastTimestamp);

        if (this->get<bool>("onlineDisplayEnable")) {
            // Send unpacked data to output channel - for GUI
            const unsigned short frameToDisplay = this->get<unsigned short>("frameToDisplay");
            if (frameToDisplay < framesPerTrain) {
                const unsigned short* adcOffset = detectorData->adc + frameToDisplay * detectorSize;
                const unsigned char* gainOffset = detectorData->gain + frameToDisplay * detectorSize;
                auto displayShape = this->getDisplayShape();
                Hash display;

                if (displayShape.size() == 1) {
                    // Use simple vectors for accommodating 1-dimensional arrays
                    std::vector<unsigned short> adcData;
                    std::vector<unsigned char> gainData;
                    adcData.assign(adcOffset, adcOffset + detectorSize);
                    gainData.assign(gainOffset, gainOffset + detectorSize);

                    display.set("data.adc", adcData);
                    display.set("data.gain", gainData);
                } else {
                    // Use IMAGEDATA and NDArrays otherwise
                    const Dims shape = displayShape;
                    NDArray imgArray(adcOffset, detectorSize, NDArray::NullDeleter());
                    ImageData adcData(imgArray, shape, karabo::xms::Encoding::GRAY, 14);


                    NDArray gainArray(gainOffset, detectorSize, NDArray::NullDeleter());
                    ImageData gainData(gainArray, shape, karabo::xms::Encoding::GRAY, 2);

                    display.set("data.adc", adcData);
                    display.set("data.gain", gainData);
                }
                this->writeChannel("display", display, detectorData->lastTimestamp);
            }
        }

        detectorData->reset();      // reset detector data
        detectorData->mutex.post(); // "unlock"
    }
} /* namespace karabo */
