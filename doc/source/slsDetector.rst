How to control an SLS detector
==============================

The SlsDetector class allows you to control a Gotthard, or another
`Detector <https://www.psi.ch/detectors/users-support>`_ supported by
PSI's slsDetectorPackage. You can set parameters and start/stop an
acquisition.

The data receiver must be running elsewhere. It can be for example the
slsReceiver provided with the slsDetectorPackage, or a SlsReceiver
Karabo device.


How to create a control device based on slsDetector
===================================================

If your detector is not covered by an already existing SlsDetector
derived class, it can easily added to the slsDetector package. You
have to create the source and header files (see
:ref:`slsDetector-hh-file` and :ref:`slsDetector-cc-file` Sections)
and add them to the Netbeans project. You also have to compile the
project in Netbeans (Debug and Release mode) in order to have the
Makefiles updated.

Do not forget to "git add" the new files.


.. _slsDetector-expected-parameters:

Expected parameters
-------------------

There are already several common detector parameters defined in the
base class (SlsDetector). Check the SlsDetector::expectedParameter()
function to see what they are.

In case you need more parameters, they must have the "sls" tag. The
alias have to contain the parameter name, as can be found in the
description of the `command line interface
<https://www.psi.ch/detectors/UsersSupportEN/slsDetectorClientHowTo.pdf>`_.

Parameters can also have the "readOnConnect" tag, in which case the
value will be read from the device upon connection.

Any Karabo type is allowed, also VECTOR types.


.. _slsDetector-hh-file:

MyDetector.hh file
------------------

This is the minimal MyDetector.hh:

.. code-block:: c++

    #ifndef KARABO_MYDETECTOR_HH
    #define KARABO_MYDETECTOR_HH


    #include <karabo/karabo.hpp>

    #include "SlsDetector.hh"

    /**
     * The main Karabo namespace
     */
    namespace karabo {

	class MyDetector : public karabo::SlsDetector {
	public:

	    KARABO_CLASSINFO(MyDetector, "MyDetector", "2.1")

	    MyDetector(const karabo::util::Hash& config);

	    virtual ~MyDetector();

	    static void expectedParameters(karabo::util::Schema& expected);
	};

    } /* namespace karabo */

    #endif /* KARABO_MYDETECTOR_HH */


You probably don't need anything more than that.


.. _slsDetector-cc-file:

MyDetector.cc file
------------------

An example of MyDetector.cc is the following. In the best case you
will just have to change the detector type (here for the Gotthard) and
add more detector specific expected parameters as described in the
:ref:`slsDetector-expected-parameters` Section.


.. code-block:: c++

    #include "MyDetector.hh"

    USING_KARABO_NAMESPACES

    namespace karabo {

	KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<StartStopFsm>,
            SlsDetector, MyDetector)

	MyDetector::MyDetector(const Hash& config) : SlsDetector(config) {
	}

	MyDetector::~MyDetector() {
	}

	void MyDetector::expectedParameters(Schema& expected) {

	    OVERWRITE_ELEMENT(expected).key("detectorType") // From base class
		    .setNewDefaultValue("Gotthard+")
		    .setNewOptions("Gotthard+")
		    .commit();

            // Add here more detector specific expected parameters
    
	}

    } /* namespace karabo */


Simulation Mode
===============

To compile the slsReceiver in simulation mode, just run

.. code-block:: bash

    make CONF=Simulation

This way the package will be linked against the simulation,
instead of the libSlsReceiver.
