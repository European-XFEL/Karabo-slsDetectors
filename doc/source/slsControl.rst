How to control an SLS detector
==============================

The SlsControl class allows you to control a Gotthard, or another
`Detector <https://www.psi.ch/en/detectors/projects>`_ supported by
PSI's slsDetectorPackage. You can set parameters, start and stop an
acquisition.

The data receiver must be also running. It can be for example the
slsReceiver provided with the slsDetectorPackage, or a SlsReceiver
Karabo device.


Mandatory parameters
--------------------

The mandatory parameters are described in :ref:`slsControl-setup`.


Optional parameters
-------------------

The SlsControl device has several optional parameters; please be aware that:

* simple (i.e. `scalar`) reconfigurable parameters tagged as `sls` are sent 
  to all the modules;
* vector reconfigurable parameters tagged as `sls` must have zero, one, or as
  many elements as the number of modules. If the vector has only one element, 
  this value will be sent to all modules;


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

In case you need to set more parameters, they must have the "sls" tag.
The alias have to contain the parameter name, as can be found in the
description of the `command line interface
<https://slsdetectorgroup.github.io/devdoc/commandline.html>`_.

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
    #include "version.hh"  // provides PACKAGE_VERSION

    /**
     * The main Karabo namespace
     */
    namespace karabo {

        class MyControl : public karabo::SlsControl {
        public:

            KARABO_CLASSINFO(MyControl, "MyControl", PACKAGE_VERSION)

            MyControl(const karabo::util::Hash& config);

            virtual ~MyControl();

            static void expectedParameters(karabo::util::Schema& expected);

        private:

            // Optional. Send the commands needed to power-up and initialize
            // the detector
            void powerOn();

            // Optional. Place here the code needed to regularly poll detector
            // parameters, e.g. temperatures, and set them in the input Hash.
            void pollDetectorSpecific(karabo::util::Hash& h);

            // Optional. Place here the code needed to execute detector specific
            // actions, when a reconfiguration is received.
            void configureDetectorSpecific(const karabo::util::Hash& configHash);

            // Optional. Place here the code needed to create the calibration
            // and settings files, if needed by the detector.
            void createCalibrationAndSettings(const std::string& settings);
        };

    } /* namespace karabo */

    #endif /* KARABO_MYCONTROL_HH */


You probably don't need anything more than that.


.. _slsControl-cc-file:

MyControl.cc file
------------------

An example of MyControl.cc is the following. In the best case you will just
have to add detector specific expected parameters as described in the
:ref:`slsControl-expected-parameters` Section.


.. code-block:: c++

    #include "MyControl.hh"

    USING_KARABO_NAMESPACES

    namespace karabo {

	KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SlsControl,
            MyControl)

	MyControl::MyControl(const Hash& config) : SlsControl(config) {
	}

	MyControl::~MyControl() {
	}

	void MyControl::expectedParameters(Schema& expected) {
            // Add here more detector specific expected parameters, for
            // example:

            VECTOR_INT32_ELEMENT(expected).key("tempAdc")
                .displayedName("ADC Temperature")
                .unit(Unit::DEGREE_CELSIUS)
                .readOnly()
                .commit();
	}

        void MyControl::powerOn() {
            // Send the commands needed to power-up and initialize the
            // detector, for example:
            sendConfiguration("powerchip", "1");
        }

        void MyControl::pollDetectorSpecific(karabo::util::Hash& h) {
            // Poll detector specifica properties, for example temperatures:
            const std::vector<int> tempAdc = m_SLS->getTemperature(slsDetectorDefs::dacIndex::TEMPERATURE_ADC, m_positions);
            h.set("tempAdc", tempAdc);
        }

        void MyControl::configureDetectorSpecific(const karabo::util::Hash& configHash) {
            // Execute detector specific actions, when a reconfiguration is
            // received.
        }


        void MyControl::createCalibrationAndSettings(const std::string& settings) {
            // Place here the code needed to create the calibration and
            // settings files, if needed by the detector. For example:

            const std::string calibrationDir = m_tmpDir + "/" + settings;

            if (!fs::exists(calibrationDir)) {
                // Create calibration and settings directory
                fs::create_directory(calibrationDir);

            KARABO_LOG_FRAMEWORK_DEBUG << "Created calibration dir" << calibrationDir;
            }

            const std::string fname = calibrationDir + "/calibration.sn";
            if (!fs::exists(fname)) {
                // Create calibration file
                std::ofstream fstr;
                fstr.open(fname.c_str());

                if (fstr.is_open()) {
                   fstr << "227 5.6\n"
                   fstr.close();

                } else {
                    throw KARABO_RECONFIGURE_EXCEPTION("Could not open file " + fname + "for writing");
                }
            }
        }

    } /* namespace karabo */


Simulation Mode
---------------

To compile slsControl in simulation mode, just run

.. code-block:: bash

    make CONF=Simulation

This way the package will be linked against the simulation,
instead of libSlsDetector.

For more details on how the simulation is implemented, see
:ref:`slsDetectorSimulation` Section.

