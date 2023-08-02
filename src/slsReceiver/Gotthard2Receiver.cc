/*
 * Author: <andrea.parenti@xfel.eu>
 * 
 * Created on August 20, 2021, 5:16 PM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

// Gotthard2 channels
#define GOTTHARD2_CHANNELS         1280

// Gotthard2 raw data: unpacking adc/gain bytes
#define GOTTHARD2_ADC_MASK       0x0FFF
#define GOTTHARD2_GAIN_MASK      0x3000
#define GOTTHARD2_GAIN_OFFSET        12

#include "Gotthard2Receiver.hh"

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SlsReceiver, Gotthard2Receiver)

    void Gotthard2Receiver::expectedParameters(Schema& expected) {

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

    Gotthard2Receiver::Gotthard2Receiver(const karabo::util::Hash& config) : SlsReceiver(config) {
    }

    Gotthard2Receiver::~Gotthard2Receiver() {
    }

    size_t Gotthard2Receiver::getDetectorSize() {
        return GOTTHARD2_CHANNELS;
    }

    std::vector<unsigned long long> Gotthard2Receiver::getDisplayShape() {
        return {this->getDetectorSize()};
    }

    std::vector<unsigned long long> Gotthard2Receiver::getDaqShape(unsigned short framesPerTrain) {
        // DAQ first dimension is fastest changing one
        return {this->getDetectorSize(), framesPerTrain};
    }

    void Gotthard2Receiver::unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned char* gain) {
        const size_t frameSize = this->getDetectorSize();
        size_t offset = sizeof(unsigned short) * idx * frameSize;
        const char* ptr = data + offset; // Base address of the <idx> frame

        for (size_t i = 0; i < frameSize; ++i) {
            adc[i] = (reinterpret_cast<const unsigned short*> (ptr))[i] & GOTTHARD2_ADC_MASK;
            gain[i] = ((reinterpret_cast<const unsigned short*> (ptr))[i] & GOTTHARD2_GAIN_MASK) >> GOTTHARD2_GAIN_OFFSET;
        }
    }

} /* namespace karabo */
