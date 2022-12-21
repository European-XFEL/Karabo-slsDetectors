/*
 * $Id: JungfrauReceiver.cc 12523 2014-01-27 15:15:19Z parenti $
 *
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on March 21, 2016, 11:37 AM
 *
 * Copyright (c) 2010-2016 European XFEL GmbH Hamburg. All rights reserved.
 */

#define JUNGFRAU_PIXEL_X            (4 * 256)
#define JUNGFRAU_PIXEL_Y            (2 * 256)

// Junfrau raw data: unpacking adc/gain bytes
#define JUNGFRAU_ADC_MASK            0x3FFF
#define JUNGFRAU_GAIN_MASK           0xC000
#define JUNGFRAU_GAIN_OFFSET             14

#include "JungfrauReceiver.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SlsReceiver, JungfrauReceiver)

    void JungfrauReceiver::expectedParameters(Schema& expected) {

        Schema displayData;

        NODE_ELEMENT(displayData).key("data")
                .displayedName("Data")
                .commit();

        std::vector<unsigned long long> shape = {JUNGFRAU_PIXEL_Y, JUNGFRAU_PIXEL_X};
        std::string dims = karabo::util::toString(shape);

        IMAGEDATA_ELEMENT(displayData).key("data.adc")
                .displayedName("ADC")
                .description("The ADC counts.")
                .setType(karabo::util::Types::UINT16)
                .setDimensions(dims)
                .commit();

        IMAGEDATA_ELEMENT(displayData).key("data.gain")
                .displayedName("Gain")
                .description("The ADC gain.")
                .setType(karabo::util::Types::UINT8)
                .setDimensions(dims)
                .commit();

        OUTPUT_CHANNEL(expected).key("display")
                .displayedName("Display")
                .dataSchema(displayData)
                .commit();

        BOOL_ELEMENT(expected).key("burstMode")
                .displayedName("Burst Mode")
                .description("The Jungfrau is operated in \"burst mode\", "
                "namely with external trigger and more than one memory cell.")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .allowedStates(State::PASSIVE)
                .commit();

        INT16_ELEMENT(expected).key("storageCellStart")
                .displayedName("Storage Cell Start")
                .description("First storage cell used by the Jungfrau. It is usually 15.")
                .assignmentOptional().defaultValue(15)
                .minInc(0).maxInc(15)
                .reconfigurable()
                .allowedStates(State::PASSIVE)
                .commit();

    }

    JungfrauReceiver::JungfrauReceiver(const karabo::util::Hash& config) : SlsReceiver(config) {
    }

    JungfrauReceiver::~JungfrauReceiver() {
    }

    bool JungfrauReceiver::isNewTrain(const karabo::util::Hash& meta) {
        const auto trainId = meta.get<unsigned long long>("trainId");
        const auto lastTrainId = meta.get<unsigned long long>("lastTrainId");

        if (this->get<bool>("burstMode")) {
            const auto storageCellStart = this->get<short>("storageCellStart");
            const auto memoryCell = meta.get<unsigned char>("memoryCell");
            if (memoryCell == storageCellStart) {
                return true;
            } else {
                return false;
            }
        }

        // "Standard" mode
        if (trainId > lastTrainId) {
            return true;
        } else {
            return false;
        }
    }

    unsigned char JungfrauReceiver::getMemoryCell(const slsDetectorDefs::sls_detector_header& detectorHeader) {
        // "For firmware ID #181206, the number of the storage cell used to store
        // the image is encoded in the bits 11-8 of the debug field."
        unsigned char memoryCell = (detectorHeader.debug >> 8) & 0xF;
        return memoryCell;
    }

    size_t JungfrauReceiver::getDetectorSize() {
        return JUNGFRAU_PIXEL_X * JUNGFRAU_PIXEL_Y;
    }


    std::vector<unsigned long long> JungfrauReceiver::getDisplayShape() {
        return {JUNGFRAU_PIXEL_Y, JUNGFRAU_PIXEL_X};
    }

    std::vector<unsigned long long> JungfrauReceiver::getDaqShape(unsigned short framesPerTrain) {
        // DAQ first dimension is fastest changing one
        return {JUNGFRAU_PIXEL_X, JUNGFRAU_PIXEL_Y, framesPerTrain};
    }

    void JungfrauReceiver::unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned char* gain) {
        const size_t frameSize = this->getDetectorSize();
        size_t offset = sizeof(unsigned short) * idx * frameSize;
        const char* ptr = data + offset; // Base address of the <idx> frame

        for (size_t i = 0; i < frameSize; ++i) {
            adc[i] = (reinterpret_cast<const unsigned short*> (ptr))[i] & JUNGFRAU_ADC_MASK;
            gain[i] = ((reinterpret_cast<const unsigned short*> (ptr))[i] & JUNGFRAU_GAIN_MASK) >> JUNGFRAU_GAIN_OFFSET;
        }
    }

} /* namespace karabo */
