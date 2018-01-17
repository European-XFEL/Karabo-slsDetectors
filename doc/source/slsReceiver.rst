How to receive data from an SLS detector
========================================

The SlsReceiver class allows you to receive data from a Gotthard, or
another `Detector <https://www.psi.ch/detectors/users-support>`_
supported by PSI's slsDetectorPackage.

The detector configuration, and the acquisition command, must be
executed somewhere else, for example by the `command line interface
<https://www.psi.ch/detectors/UsersSupportEN/slsDetectorClientHowTo.pdf>`_
provided by PSI, or by a SlsDetector Karabo device.


How to create a receiver device based on slsReceiver
====================================================

If your detector is not covered by an already existing SlsReceiver
derived class, it can easily added to the slsReceiver package. You
have to create the source and header files (see
:ref:`slsReceiver-hh-file` and :ref:`slsReceiver-cc-file` Sections)
and add them to the Netbeans project. You also have to compile the
project in Netbeans (Debug and Release mode) in order to have the
Makefiles updated.

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

	private: // State-machine call-backs (override)

	private: // Functions

	private: // Raw data unpacking

	    unsigned short getPacketSize();
	    unsigned short getFrameNumber(const char* data, size_t idx);
	    unsigned short getPacketNumber(const char* data, size_t idx);
	    void getAdcAndGain(const char* data, size_t idx,
                               std::vector<unsigned short>& adc,
                               std::vector<unsigned short>& gain);

	private: // Members

	};

    } /* namespace karabo */

    #endif /* KARABO_MYRECEIVER_HH */


You probably don't need anything more than that.

Some functions are pure virtual in SlsReceiver and must be defined in
the derived class:

* getPacketSize

* getFrameNumber

* getPacketNumber

* getAdcAndGain


.. _slsReceiver-cc-file:

MyReceiver.cc file
------------------

The pure virtual functions which must be defined in the derived class
are:

.. function:: unsigned short getPacketSize()

   return the byte size of a data packet.

.. function:: unsigned short getFrameNumber(const char* data, size_t idx)

   return the frame number of the packet <idx> in given <data>.

.. function:: unsigned short getPacketNumber(const char* data, size_t idx)

   return the packet number of the packet <idx> in given <data>.

.. function:: void getAdcAndGain(const char* data, size_t idx, std::vector<unsigned short>& adc, std::vector<unsigned short>& gain)

   fill-up the <adc> and <gain> vectors with the ADC and gain values
   contained in <data> for the packet <idx>.


An example of MyReceiver.cc is the following. In the best case you
will just have to change the constants (here for the Gotthard) to
match the raw data format of the detector:

.. code-block:: c++

    #include "MyReceiver.hh"

    USING_KARABO_NAMESPACES

    // e.g. Gotthard channels
    #define MY_CHANNELS 640

    // e.g. Gotthard raw data
    #define MY_OFFSET              2
    #define MY_HEADER_LENGTH       2
    #define MY_UNUSED_LENGTH       4
    #define MY_PACKET_SIZE      1286 // = MY_HEADER_LENGTH + 2*MY_CHANNELS \
        + MY_UNUSED_LENGTH

    // Gotthard raw data: unpacking frame/packet bytes
    #define MY_FRAME_MASK     0xFFFE
    #define MY_FRAME_OFFSET        1
    #define MY_PACKET_MASK    0x0001

    // Gotthard raw data: unpacking adc/gain bytes
    #define MY_ADC_MASK       0x3FFF
    #define MY_GAIN_MASK      0xC000

    namespace karabo {

	KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>,
            SlsReceiver, MyReceiver)

	void MyReceiver::expectedParameters(Schema& expected) {
	}

	MyReceiver::MyReceiver(const karabo::util::Hash& config) :
                SlsReceiver(config) {
	}

	MyReceiver::~MyReceiver() {
	}

	unsigned short MyReceiver::getPacketSize() {
	    return MY_PACKET_SIZE;
	}

	// Get frame number for packet at position <idx> in <data>
	unsigned short MyReceiver::getFrameNumber(const char* data,
                size_t idx) {

            // Base address of the <idx> packet
	    const char* packet = data + idx*MY_PACKET_SIZE + MY_OFFSET;

	    unsigned short frameNumber = 
                (reinterpret_cast<const unsigned short*>(packet))[0];
	    frameNumber = (frameNumber & MY_FRAME_MASK) >
                MY_FRAME_OFFSET;

	    return frameNumber;
	}

	// Get packet number for packet at position <idx> in <data>
	unsigned short MyReceiver::getPacketNumber(const char* data, 
                size_t idx) {

            // Base address of the <idx> packet
	    const char* packet = data + idx*MY_PACKET_SIZE + MY_OFFSET;

	    unsigned short packetNumber = 
                (reinterpret_cast<const unsigned short*>(packet))[0];
	    packetNumber = packetNumber & MY_PACKET_MASK;

	    return packetNumber;
	}

	// Get ADC counts and gain for packet at position <idx> in <data>
	void MyReceiver::getAdcAndGain(const char* data, size_t idx, 
                std::vector<unsigned short>& adc, 
                std::vector<unsigned short>& gain) {

            // Base address of the <idx> packet
	    const char* packet = data + idx*MY_PACKET_SIZE + MY_OFFSET;
	    packet += MY_HEADER_LENGTH; // base address for adc/gain

	    // Resize vectors
	    adc.resize(MY_CHANNELS);
	    gain.resize(MY_CHANNELS);

	    for (size_t i=0; i<MY_CHANNELS; ++i) {
		adc.at(i) = (reinterpret_cast<const unsigned 
                    short*>(packet))[i] & MY_ADC_MASK;
		gain.at(i) = (reinterpret_cast<const 
                    unsigned short*>(packet))[i] & MY_GAIN_MASK;
	    }

	}

    } /* namespace karabo */


Simulation Mode
===============

To compile the slsReceiver in simulation mode, just run

.. code-block:: bash

    make CONF=Simulation

This way the package will be linked against the simulation,
instead of the libSlsReceiver.

