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
                .commit();

        IMAGEDATA_ELEMENT(displayData).key("data.gain")
                .displayedName("Gain")
                .description("The ADC gain.")
                .commit();

        OUTPUT_CHANNEL(expected).key("display")
                .displayedName("Display")
                .dataSchema(displayData)
                .commit();
    }

    JungfrauReceiver::JungfrauReceiver(const karabo::util::Hash& config) : SlsReceiver(config) {
    }

    JungfrauReceiver::~JungfrauReceiver() {
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

    void JungfrauReceiver::unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned short* gain) {
	const size_t frameSize = this->getDetectorSize();
        size_t offset = sizeof(unsigned short) * idx * frameSize;
        const char* ptr = data + offset; // Base address of the <idx> frame

        for (size_t i = 0; i < frameSize; ++i) {
            adc[i] = (reinterpret_cast<const unsigned short*> (ptr))[i] & JUNGFRAU_ADC_MASK;
            gain[i] = ((reinterpret_cast<const unsigned short*> (ptr))[i] & JUNGFRAU_GAIN_MASK) >> JUNGFRAU_GAIN_OFFSET;
        }
    }

} /* namespace karabo */
