/*
 * Author: <andrea.parenti@xfel.eu>
 *
 * Created on March 18, 2016,  3:29 PM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

// Gotthard channels
#define GOTTHARD_CHANNELS         1280

// Gotthard raw data: unpacking adc/gain bytes
#define GOTTHARD_ADC_MASK       0x3FFF
#define GOTTHARD_GAIN_MASK      0xC000
#define GOTTHARD_GAIN_OFFSET        14

#include "GotthardReceiver.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SlsReceiver, GotthardReceiver)

    void GotthardReceiver::expectedParameters(Schema& expected) {

        Schema displayData;

        NODE_ELEMENT(displayData).key("data")
                .displayedName("Data")
                .commit();

        VECTOR_UINT16_ELEMENT(displayData).key("data.adc")
                .displayedName("ADC")
                .description("The ADC counts.")
                .readOnly()
                .commit();

        VECTOR_UINT8_ELEMENT(displayData).key("data.gain")
                .displayedName("Gain")
                .description("The ADC gain.")
                .readOnly()
                .commit();
        OUTPUT_CHANNEL(expected).key("display")
                .displayedName("Display")
                .dataSchema(displayData)
                .commit();
    }

    GotthardReceiver::GotthardReceiver(const karabo::util::Hash& config) : SlsReceiver(config) {
    }

    GotthardReceiver::~GotthardReceiver() {
    }

    size_t GotthardReceiver::getDetectorSize() {
        return GOTTHARD_CHANNELS;
    }

    std::vector<unsigned long long> GotthardReceiver::getDisplayShape() {
        return {this->getDetectorSize()};
    }

    std::vector<unsigned long long> GotthardReceiver::getDaqShape(unsigned short framesPerTrain) {
        // DAQ first dimension is fastest changing one
        return {this->getDetectorSize(), framesPerTrain};
    }

    void GotthardReceiver::unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned char* gain) {
        const size_t frameSize = this->getDetectorSize();
        size_t offset = sizeof(unsigned short) * idx * frameSize;
        const char* ptr = data + offset; // Base address of the <idx> frame

        for (size_t i = 0; i < frameSize; ++i) {
            adc[i] = (reinterpret_cast<const unsigned short*> (ptr))[i] & GOTTHARD_ADC_MASK;
            gain[i] = ((reinterpret_cast<const unsigned short*> (ptr))[i] & GOTTHARD_GAIN_MASK) >> GOTTHARD_GAIN_OFFSET;
        }
    }

} /* namespace karabo */
