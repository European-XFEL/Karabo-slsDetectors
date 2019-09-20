How to receive data from an SLS detector
========================================

The SlsReceiver class allows you to receive data from a Gotthard, or
another `Detector <https://www.psi.ch/detectors/users-support>`_
supported by PSI's slsDetectorPackage.

The detector configuration, and the acquisition command, must be
executed somewhere else, for example by the `command line interface
<https://www.psi.ch/detectors/UsersSupportEN/slsDetectorClientHowTo.pdf>`_
provided by PSI, or by a SlsControl Karabo device.


How to create a receiver device based on slsReceiver
====================================================

If your detector is not covered by an already existing SlsReceiver
derived class, it can easily added to the slsReceiver package. You
have to create the source and header files (see
:ref:`slsReceiver-hh-file` and :ref:`slsReceiver-cc-file` Sections)
and add them to the Netbeans project. You also have to compile the
project in Netbeans (Debug, Release and Simulation mode) in order to
have the Makefiles updated.

Do not forget to "git add" the new files.


Expected parameters
-------------------

The receiver TCP port number (rx_tcpport) is already one of the
expected parameters of the base class (SlsReceiver):

.. code-block:: c++
 
        UINT16_ELEMENT(expected).key("rxTcpPort")
                .tags("sls")
                .alias("--rx_tcpport")
                .displayedName("rxTcpPort")
                .description("Receiver TCP Port")
                .assignmentOptional().defaultValue(1954)
                .init()
                .commit();

If you need to pass more options to the slsReceiverUsers object when
it is instantiated in Karabo, you can do it by adding more expected
parameters to your MyReceiver class. As done for the above one
(rx_tcpport), you will need to tag them as "sls", and to give in their
alias the parameter name.


.. _slsReceiver-hh-file:

MyReceiver.hh file
------------------

This is the minimal MyReceiver.hh:

.. code-block:: c++

    #ifndef KARABO_MYRECEIVER_HH
    #define KARABO_MYRECEIVER_HH

    #include <karabo/karabo.hpp>

    #include "SlsReceiver.hh"

    /**
     * The main Karabo namespace
     */
    namespace karabo {

	class MyReceiver : public karabo::SlsReceiver {

	public:

	    KARABO_CLASSINFO(MyReceiver, "MyReceiver", "2.1")
	    static void expectedParameters(karabo::util::Schema& expected);

	    MyReceiver(const karabo::util::Hash& config);

	    virtual ~MyReceiver();

	private: // Raw data unpacking

            size_t getDetectorSize();
            std::vector<unsigned long long> getDisplayShape();
            std::vector<unsigned long long> getDaqShape(unsigned short framesperTrain);
            void unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned short* gain);

	};

    } /* namespace karabo */

    #endif /* KARABO_MYRECEIVER_HH */


You probably don't need anything more than that.

Some functions are pure virtual in SlsReceiver and must be defined in
the derived class:

* getDetectorSize

* getDisplayShape

* getDaqShape

* unpackRawData


.. _slsReceiver-cc-file:

MyReceiver.cc file
------------------

The pure virtual functions which must be defined in the derived class
are:

.. function:: size_t getDetectorSize() 

   returns the size of the detector (i.e. the number of channels, or pixels).

.. function:: std::vector<unsigned long long> getDisplayShape()

   returns the shape of one frame (can be 1- or 2-d).

.. function:: std::vector<unsigned long long> getDaqShape(unsigned short framesPerTrain)

   returns the shape of the data as needed by the DAQ (frames are grouped per
   train before being sent to the DAQ, therefore it is 2- or 3-d;
   moreover the DAQ wants the first dimension to be the fastest changing,
   the last dimension the slowest).

.. function:: void unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned short* gain)

   fill-up the <adc> and <gain> buffers with the ADC and gain values
   contained in <data> for the packet <idx>.


An example of MyReceiver.cc is the following. In the best case you
will just have to change the constants (here for the Gotthard) to
match the raw data format of the detector:

.. code-block:: c++

    #include "MyReceiver.hh"

    USING_KARABO_NAMESPACES

    // e.g. Gotthard channels
    #define MY_CHANNELS 1280

    // e.g. Gotthard raw data: unpacking adc/gain bytes
    #define MY_ADC_MASK       0x3FFF
    #define MY_GAIN_MASK      0xC000
    #define MY_GAIN_OFFSET        14

    namespace karabo {

	KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>,
            SlsReceiver, MyReceiver)

	void MyReceiver::expectedParameters(Schema& expected) {
            Schema displayData;

            // This is the schema for data display in the GUI,
            // in this example for 1-d data
            NODE_ELEMENT(displayData).key("data")
                    .displayedName("Data")
                    .commit();

            VECTOR_UINT16_ELEMENT(displayData).key("data.adc")
                    .displayedName("ADC")
                    .description("The ADC counts.")
                    .readOnly()
                    .commit();

            VECTOR_UINT16_ELEMENT(displayData).key("data.gain")
                    .displayedName("Gain")
                    .description("The ADC gain.")
                    .readOnly()
                    .commit();

            OUTPUT_CHANNEL(expected).key("display")
                    .displayedName("Display")
                    .dataSchema(displayData)
                    .commit();
	}

	MyReceiver::MyReceiver(const karabo::util::Hash& config) :
                SlsReceiver(config) {
	}

	MyReceiver::~MyReceiver() {
	}

	unsigned short MyReceiver::getDetectorSize() {
	    return MY_CHANNELS;
	}

        std::vector<unsigned long long> MyReceiver::getDisplayShape() {
            return {this->getDetectorSize()};
        }

        std::vector<unsigned long long> MyReceiver::getDaqShape(unsigned short framesPerTrain) {
            // DAQ first dimension is fastest changing one
            return {this->getDetectorSize(), framesPerTrain};
        }

        void MyReceiver::unpackRawData(const char* data, size_t idx, unsigned short* adc, unsigned short* gain) {
            // e.g. For Gotthard:
            const size_t frameSize = this->getDetectorSize();
            size_t offset = sizeof(unsigned short) * idx * frameSize;
            const char* ptr = data + offset; // Base address of the <idx> frame

            for (size_t i = 0; i < frameSize; ++i) {
                adc[i] = (reinterpret_cast<const unsigned short*> (ptr))[i] & MY_ADC_MASK;
                gain[i] = ((reinterpret_cast<const unsigned short*> (ptr))[i] & MY_GAIN_MASK) >> MY_GAIN_OFFSET;
            }
        }

    } /* namespace karabo */


Simulation Mode
---------------

To compile the slsReceiver in simulation mode, just run

.. code-block:: bash

    make CONF=Simulation

This way the package will be linked against the simulation,
instead of the libSlsReceiver.

