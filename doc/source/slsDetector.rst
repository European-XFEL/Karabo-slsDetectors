How to control an SLS detector
==============================

The SlsControl class allows you to control a Gotthard, or another
`Detector <https://www.psi.ch/detectors/users-support>`_ supported by
PSI's slsDetectorPackage. You can set parameters and start/stop an
acquisition.

The data receiver must be running elsewhere. It can be for example the
slsReceiver provided with the slsDetectorPackage, or a SlsReceiver
Karabo device.


How to create a control device based on slsControl
==================================================

If your detector is not covered by an already existing SlsControl
derived class, it can easily added to the slsControl package. You
have to create the source and header files (see
:ref:`slsControl-hh-file` and :ref:`slsControl-cc-file` Sections)
and add them to the Netbeans project. You also have to compile the
project in Netbeans (Debug, Release and Simulation mode) in order to
have the Makefiles updated.

Do not forget to "git add" the new files.


.. _slsControl-expected-parameters:

Expected parameters
-------------------

There are already several common detector parameters defined in the
base class (SlsControl). Check the SlsControl::expectedParameter()
function to see what they are.

In case you need more parameters, they must have the "sls" tag. The
alias have to contain the parameter name, as can be found in the
description of the `command line interface
<https://www.psi.ch/detectors/UsersSupportEN/slsDetectorClientHowTo.pdf>`_.

Parameters can also have the "readOnConnect" tag, in which case the
value will be read from the device upon connection.

Any Karabo type is allowed, also VECTOR types.


.. _slsControl-hh-file:

MyControl.hh file
------------------

This is the minimal MyControl.hh:

.. code-block:: c++

    #ifndef KARABO_MYCONTROL_HH
    #define KARABO_MYCONTROL_HH


    #include <karabo/karabo.hpp>

    #include "SlsControl.hh"

    /**
     * The main Karabo namespace
     */
    namespace karabo {

	class MyControl : public karabo::SlsControl {
	public:

	    KARABO_CLASSINFO(MyControl, "MyControl", "2.2")

	    MyControl(const karabo::util::Hash& config);

	    virtual ~MyControl();

	    static void expectedParameters(karabo::util::Schema& expected);

       private:

            // This function must return the detector calibration
            // as in the calibration.sn file
            const char* getCalibrationString() const;

            // This function must return the detector settings
            // as in the settings.sn file
            const char* getSettingsString() const;

	};

    } /* namespace karabo */

    #endif /* KARABO_MYCONTROL_HH */


You probably don't need anything more than that.


.. _slsControl-cc-file:

MyControl.cc file
------------------

An example of MyControl.cc is the following. In the best case you
will just have to change the detector type (here for the Gotthard) and
add more detector specific expected parameters as described in the
:ref:`slsControl-expected-parameters` Section.


.. code-block:: c++

    #include "MyControl.hh"

    USING_KARABO_NAMESPACES

    namespace karabo {

	KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<CameraFsm>,
            SlsControl, MyControl)

	MyControl::MyControl(const Hash& config) : SlsControl(config) {
	}

	MyControl::~MyControl() {
	}

	void MyControl::expectedParameters(Schema& expected) {

	    OVERWRITE_ELEMENT(expected).key("detectorType") // From base class
		    .setNewDefaultValue("Gotthard+")
		    .setNewOptions("Gotthard+")
		    .commit();

            // Add here more detector specific expected parameters
    
	}

        const char* MyControl::getCalibrationString() const {
    	    return "227 5.6\n"; // This one is for Gotthard
        }

        const char* GotthardControl::getSettingsString() const {
            return "Vcasc 1320\nVcascN 650\nVcascP 1480\nVib_test 2001\nVin 1350\nVout 1520\nVref 660\nVref_comp 887\n"; // This one is for Gotthard
        }

    } /* namespace karabo */


Simulation Mode
===============

To compile slsControl in simulation mode, just run

.. code-block:: bash

    make CONF=Simulation

This way the package will be linked against the simulation,
instead of the libSlsDetector.
